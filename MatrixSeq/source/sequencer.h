/*
 * sequence.h
 *
 *  Created on: 4 Mar 2018
 *      Author: jason
 */

#ifndef SEQUENCER_H_
#define SEQUENCER_H_

#include <clock.h>
#include "cv_gate.h"
#include "sequence_layer.h"
#include "midi.h"
class CSequencer {
public:


	enum {
		NUM_LAYERS = 4
	};
	CSequenceLayer m_layer[NUM_LAYERS];
	byte m_midi_velocity = 127;
	CSequencer() {
		m_midi_velocity = 127;
	}








	/////////////////////////////////////////////////////////////////////////////////////////////
	void tick(uint32_t ticks, byte parts_tick) {

		//byte midi_velocity = m_midi_velocity;
		//uint16_t midi_note_action[NUM_LAYERS];
		int note_layer = -1;
		int velocity_layer = -1;
		byte stepped = 0;

		// run the clock on each sequencer layer
		for(int i=0; i<NUM_LAYERS; ++i) {
			if(CSequenceLayer::VELOCITY_SEQUENCE == m_layer[i].m_mode) {
				velocity_layer = i; // remember if there is a velocity layer (only 1 allowed)
			}
			if(CSequenceLayer::NOTE_SEQUENCE == m_layer[i].m_mode) {
				note_layer = i; // remember last note layer
			}
			stepped |= m_layer[i].tick(ticks, parts_tick);
		}
		if(!stepped) {
			// if nothing has changed, no need to go further
			return;
		}

		CCVGate::TRANSACTION txn;
		for(int i=0; i<NUM_LAYERS; ++i) {
			CSequenceLayer& layer = m_layer[i];
			txn.flags[i] = 0;
			if(CSequenceLayer::NOTE_SEQUENCE == m_layer[i].m_mode) {
				note_layer = i; // preceding note layer for a transpose layer
			}

			// has there been a step on this sequence in this millisecond?
			if(layer.m_stepped) {

				// is this a trigger step?
				if(layer.m_value & CSequenceLayer::IS_TRIG) {
					txn.flags[i] |= CCVGate::TXN_TRIG;
				}
				// is this an active (gate remain open) step?
				else if(layer.m_value & CSequenceLayer::IS_ACTIVE) {
					txn.flags[i] |= CCVGate::TXN_GATE;
				}

				// what type of layer?
				switch(layer.m_mode)
				{
					////////////////////////////////////////////////////////////////////
					case CSequenceLayer::NOTE_SEQUENCE:
					case CSequenceLayer::TRANSPOSE_SEQUENCE:
					{
						int midi_note;
						if(layer.m_mode == CSequenceLayer::TRANSPOSE_SEQUENCE) {
							if(note_layer >= 0) {
								// get note from the note layer and transpose it
								midi_note = (byte)m_layer[note_layer].m_value;
								midi_note += ((int)((byte)layer.m_value)-64);
								midi_note = layer.force_to_scale(midi_note);
							}
							else {
								// if there is no note layer then there will be no
								// CV or MIDI from a transpose layer...
								break;
							}
						}
						else {
							midi_note = (byte)layer.m_value;
						}

						// decide MIDI velocity
						byte midi_velocity;
						if(velocity_layer >= 0) {
							midi_velocity = (byte)m_layer[velocity_layer].m_value;
						}
						else {
							midi_velocity = m_midi_velocity;
						}

						// gonna send us some CV...
						txn.flags[i] |= CCVGate::TXN_PITCH;
						txn.cv[i] = ((uint16_t)(midi_note))<<8;

						// Now sort out MIDI...
						// is the current step a RETRIG?
						if(layer.m_value & CSequenceLayer::IS_TRIG) {
							if(layer.m_last_midi_note) {
								// stop previous note
								g_midi.send_note(layer.m_midi_channel, layer.m_last_midi_note, 0);
							}
							// play new note and remember it
							layer.m_last_midi_note = (byte)midi_note;
							g_midi.send_note(layer.m_midi_channel, layer.m_last_midi_note, midi_velocity);
						}
						// is the current step LEGATO?
						else if(layer.m_value & CSequenceLayer::IS_ACTIVE) {
							if(layer.m_last_midi_note != (byte)midi_note) {
								g_midi.send_note(layer.m_midi_channel, (byte)midi_note, midi_velocity);
								if(layer.m_last_midi_note) {
									g_midi.send_note(layer.m_midi_channel, layer.m_last_midi_note, 0);
								}
								layer.m_last_midi_note = (byte)midi_note;
							}
						}
						// current step is MUTED
						else {
							if(layer.m_last_midi_note) {
								g_midi.send_note(layer.m_midi_channel, layer.m_last_midi_note, 0);
								layer.m_last_midi_note = 0;
							}
						}
						break;
					}

					////////////////////////////////////////////////////////////////////
					case CSequenceLayer::VELOCITY_SEQUENCE:
						// A velocity layer sends no MIDI out by itself
						txn.flags[i] |= CCVGate::TXN_CV;
						txn.cv[i] = (layer.m_value & 0x7F)<<8;
						break;

					////////////////////////////////////////////////////////////////////
					case CSequenceLayer::MOD_SEQUENCE:
						txn.flags[i] |= CCVGate::TXN_CV;
						txn.cv[i] = (layer.m_value & 0x7F)<<8;
						break;
				}
			}
		}

		// if there are any actions then
		for(int i=0; i<NUM_LAYERS; ++i) {
			if(txn.flags[i]) {
				//
				g_cv_gate.set(txn);
				break;
			}
		}
	}

};

extern CSequencer g_sequencer;
#ifdef MAIN_INCLUDE
	CSequencer g_sequencer;
#endif

#endif /* SEQUENCER_H_ */
