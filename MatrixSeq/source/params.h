/*
 * params.h
 *
 *  Created on: 12 Mar 2018
 *      Author: jason
 */

#ifndef PARAMS_H_
#define PARAMS_H_

typedef enum {
	P_NONE = 0,

	P_SQL_SEQ_MODE,
	P_SQL_STEP_RATE,
	P_SQL_STEP_DUR,
	P_SQL_MIDI_CHAN,
	P_SQL_MIDI_CC,
	P_SQL_SCALE_TYPE,
	P_SQL_SCALE_ROOT,
	P_SQL_FORCE_SCALE,
	P_SQL_MIDI_VEL_HI,
	P_SQL_MIDI_VEL_MED,
	P_SQL_MIDI_VEL_LO,
	P_SQL_MAX,

	P_CVGATE_VSCALE,
	P_CVGATE_VRANGE,
	P_CVGATE_MAX,

	P_CLOCK_BPM,
	P_CLOCK_SRC,
	P_CLOCK_MAX
} PARAM_ID;


typedef enum:byte {
	V_SQL_SEQ_MODE_CHROMATIC = 0,		// One grid row is one semitone (11 rows per octave)
	V_SQL_SEQ_MODE_SCALE,				// One grid row is one scale note (7 rows per octave)
	V_SQL_SEQ_MODE_MOD,					// full modulation range compressed to 13 rows
	V_SQL_SEQ_MODE_TRANSPOSE,
	V_SQL_SEQ_MODE_MAX
} V_SQL_SEQ_MODE;


typedef enum:byte {
	V_SQL_SCALE_TYPE_IONIAN,
	V_SQL_SCALE_TYPE_DORIAN,
	V_SQL_SCALE_TYPE_PHRYGIAN,
	V_SQL_SCALE_TYPE_LYDIAN,
	V_SQL_SCALE_TYPE_MIXOLYDIAN,
	V_SQL_SCALE_TYPE_AEOLIAN,
	V_SQL_SCALE_TYPE_LOCRIAN,
	V_SQL_SCALE_TYPE_MAX
} V_SQL_SCALE_TYPE;

typedef enum:byte {
	V_SQL_SCALE_ROOT_C = 0,
	V_SQL_SCALE_ROOT_CSHARP,
	V_SQL_SCALE_ROOT_D,
	V_SQL_SCALE_ROOT_DSHARP,
	V_SQL_SCALE_ROOT_E,
	V_SQL_SCALE_ROOT_F,
	V_SQL_SCALE_ROOT_FSHARP,
	V_SQL_SCALE_ROOT_G,
	V_SQL_SCALE_ROOT_GSHARP,
	V_SQL_SCALE_ROOT_A,
	V_SQL_SCALE_ROOT_ASHARP,
	V_SQL_SCALE_ROOT_B
} V_SQL_SCALE_ROOT;

typedef enum:byte {
	V_SQL_FORCE_SCALE_OFF,
	V_SQL_FORCE_SCALE_ON,
	V_SQL_FORCE_SCALE_MAX
} V_SQL_FORCE_SCALE;

typedef enum:byte {
	V_SQL_VEL_MOD_OFF,
	V_SQL_VEL_MOD_LAYER1,
	V_SQL_VEL_MOD_LAYER2,
	V_SQL_VEL_MOD_LAYER3,
	V_SQL_VEL_MOD_LAYER4,
	V_SQL_VEL_MOD_MAX,
} V_SQL_VEL_MOD;

typedef enum:byte {
	V_SQL_TRANSPOSE_MOD_OFF,
	V_SQL_TRANSPOSE_MOD_LAYER1,
	V_SQL_TRANSPOSE_MOD_LAYER2,
	V_SQL_TRANSPOSE_MOD_LAYER3,
	V_SQL_TRANSPOSE_MOD_LAYER4,
	V_SQL_TRANSPOSE_MOD_MAX,
} V_SQL_TRANSPOSE_MOD;

typedef enum:byte {
	V_SQL_MIDI_CHAN_NONE,
	V_SQL_MIDI_CHAN_1,
	V_SQL_MIDI_CHAN_2,
	V_SQL_MIDI_CHAN_3,
	V_SQL_MIDI_CHAN_4,
	V_SQL_MIDI_CHAN_5,
	V_SQL_MIDI_CHAN_6,
	V_SQL_MIDI_CHAN_7,
	V_SQL_MIDI_CHAN_8,
	V_SQL_MIDI_CHAN_9,
	V_SQL_MIDI_CHAN_10,
	V_SQL_MIDI_CHAN_11,
	V_SQL_MIDI_CHAN_12,
	V_SQL_MIDI_CHAN_13,
	V_SQL_MIDI_CHAN_14,
	V_SQL_MIDI_CHAN_15,
	V_SQL_MIDI_CHAN_16
} V_SQL_MIDI_CHAN;


typedef enum:byte {
	V_SQL_STEP_RATE_1 = 0,
	V_SQL_STEP_RATE_2D,
	V_SQL_STEP_RATE_2,
	V_SQL_STEP_RATE_4D,
	V_SQL_STEP_RATE_2T,
	V_SQL_STEP_RATE_4,
	V_SQL_STEP_RATE_8D,
	V_SQL_STEP_RATE_4T,
	V_SQL_STEP_RATE_8,
	V_SQL_STEP_RATE_16D,
	V_SQL_STEP_RATE_8T,
	V_SQL_STEP_RATE_16,
	V_SQL_STEP_RATE_16T,
	V_SQL_STEP_RATE_32,
	V_SQL_STEP_RATE_MAX
} V_SQL_STEP_RATE;

typedef enum:byte {
	V_SQL_STEP_DUR_STEP,	// Play for exactly one sequencer step unless extended by legato step(s)
	V_SQL_STEP_DUR_FULL,	// Play up until the next active sequencer step
	V_SQL_STEP_DUR_NONE,
	V_SQL_STEP_DUR_32,
	V_SQL_STEP_DUR_16T,
	V_SQL_STEP_DUR_16,
	V_SQL_STEP_DUR_8T,
	V_SQL_STEP_DUR_16D,
	V_SQL_STEP_DUR_8,
	V_SQL_STEP_DUR_4T,
	V_SQL_STEP_DUR_8D,
	V_SQL_STEP_DUR_4,
	V_SQL_STEP_DUR_2T,
	V_SQL_STEP_DUR_4D,
	V_SQL_STEP_DUR_2,
	V_SQL_STEP_DUR_2D,
	V_SQL_STEP_DUR_1,
	V_SQL_STEP_DUR_MAX
} V_SQL_STEP_DUR;


typedef enum:byte {
	V_CVGATE_VSCALE_1VOCT = 0,
	V_CVGATE_VSCALE_1_2VOCT,
	V_CVGATE_VSCALE_HZVOLT,
	V_CVGATE_VSCALE_MAX
} V_CVGATE_VSCALE;

typedef enum:byte {
	V_SQL_STEP_TRIG_CLOCK = 0,
	V_SQL_STEP_TRIG_LAYER1,
	V_SQL_STEP_TRIG_LAYER2,
	V_SQL_STEP_TRIG_LAYER3,
	V_SQL_STEP_TRIG_LAYER4,
	V_SQL_STEP_TRIG_MAX
} V_SQL_STEP_TRIG;

typedef enum:byte {
	V_SQL_RESET_TRIG_NONE = 0,
	V_SQL_RESET_TRIG_LAYER1,
	V_SQL_RESET_TRIG_LAYER2,
	V_SQL_RESET_TRIG_LAYER3,
	V_SQL_RESET_TRIG_LAYER4,
	V_SQL_RESET_TRIG_MAX
} V_SQL_RESET_TRIG;

typedef enum:byte {
	V_SQL_TRANSP_SRC_NONE = 0,
	V_SQL_TRANSP_SRC_LAYER1,
	V_SQL_TRANSP_SRC_LAYER2,
	V_SQL_TRANSP_SRC_LAYER3,
	V_SQL_TRANSP_SRC_LAYER4,
	V_SQL_TRANSP_SRC_MAX
} V_SQL_TRANSP_SRC;

typedef enum:byte {
	V_CLOCK_SRC_INTERNAL,
	V_CLOCK_SRC_MIDI
} V_CLOCK_SRC;



extern void set_param(PARAM_ID param, int value);
extern int get_param(PARAM_ID param);
extern int is_valid_param(PARAM_ID param);




#endif /* PARAMS_H_ */
