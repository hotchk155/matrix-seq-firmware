///////////////////////////////////////////////////////////////////////////////////
//
//                                  ~~  ~~             ~~
//  ~~~~~~    ~~~~~    ~~~~~    ~~~~~~  ~~     ~~~~~   ~~~~~~    ~~~~~   ~~    ~~
//  ~~   ~~  ~~   ~~  ~~   ~~  ~~   ~~  ~~    ~~   ~~  ~~   ~~  ~~   ~~   ~~  ~~
//  ~~   ~~  ~~   ~~  ~~   ~~  ~~   ~~  ~~    ~~~~~~~  ~~   ~~  ~~   ~~     ~~
//  ~~   ~~  ~~   ~~  ~~   ~~  ~~   ~~  ~~    ~~       ~~   ~~  ~~   ~~   ~~  ~~
//  ~~   ~~   ~~~~~    ~~~~~    ~~~~~~   ~~~   ~~~~~   ~~~~~~    ~~~~~   ~~    ~~
//
//  Serendipity Sequencer                                   CC-NC-BY-SA
//  hotchk155/2018                                          Sixty-four pixels ltd
//
//  GLOBAL DEFINITIONS
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef DEFS_H_
#define DEFS_H_

enum {
	EV_NONE,
	EV_ENCODER,
	EV_KEY_PRESS,
	EV_KEY_RELEASE,
	EV_KEY_CLICK,
	EV_KEY_HOLD,
	EV_SEQ_START,
	EV_SEQ_STOP,
	EV_SEQ_RESTART,
	EV_CLOCK_RESET
};


typedef unsigned char byte;

#define PORTA_BASE 0
#define PORTB_BASE 8
#define PORTC_BASE 16
#define PORTD_BASE 24


#define MK_GPIOA_BIT(port, bit) (((uint32_t)1) << ((port) + (bit)))
#define SET_GPIOA(mask) ((GPIO_Type *)GPIOA_BASE)->PSOR = (mask)
#define CLR_GPIOA(mask) ((GPIO_Type *)GPIOA_BASE)->PCOR = (mask)
#define READ_GPIOA(mask) (((GPIO_Type *)GPIOA_BASE)->PDIR & (mask))

typedef enum:byte {
	P_NONE = 0,

	P_SQL_SEQ_MODE,
	P_SQL_STEP_RATE,
	P_SQL_STEP_DUR,
	P_SQL_MIDI_CHAN,
	P_SQL_MIDI_CC,
	P_SQL_SCALE_TYPE,
	P_SQL_SCALE_ROOT,
	P_SQL_FORCE_SCALE,
	P_SQL_MIDI_VEL_ACCENT,
	P_SQL_MIDI_VEL,
	P_SQL_CVSCALE,
	P_SQL_CVRANGE,
	P_SQL_CVGLIDE,
	P_SQL_TRAN_TRIG,
	P_SQL_TRAN_ACC,
	P_SQL_LOAD_PATTERN,
	P_SQL_SAVE_PATTERN,
	P_SQL_MAX,

	P_CLOCK_BPM,
	P_CLOCK_SRC,
	P_CLOCK_IN_RATE,
	P_CLOCK_OUT_RATE,
	P_CLOCK_MAX
} PARAM_ID;

typedef enum:byte {
	PT_NONE = 0,
	PT_ENUMERATED,
	PT_MIDI_CHANNEL,
	PT_NUMBER_7BIT,
	PT_VOLT_RANGE,
	PT_BPM,
	PT_DURATION,
	PT_PATTERN
} PARAM_TYPE;


typedef enum:byte {
	V_SQL_SEQ_MODE_SCALE = 0,
	V_SQL_SEQ_MODE_CHROMATIC,
	V_SQL_SEQ_MODE_MOD,
	V_SQL_SEQ_MODE_TRANSPOSE,
	V_SQL_SEQ_MODE_VELOCITY,
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

/*
typedef enum:byte {
	V_SQL_VEL_MOD_OFF,
	V_SQL_VEL_MOD_LAYER1,
	V_SQL_VEL_MOD_LAYER2,
	V_SQL_VEL_MOD_LAYER3,
	V_SQL_VEL_MOD_LAYER4,
	V_SQL_VEL_MOD_MAX,
} V_SQL_VEL_MOD;
*/
/*
typedef enum:byte {
	V_SQL_TRANSPOSE_MOD_OFF,
	V_SQL_TRANSPOSE_MOD_LAYER1,
	V_SQL_TRANSPOSE_MOD_LAYER2,
	V_SQL_TRANSPOSE_MOD_LAYER3,
	V_SQL_TRANSPOSE_MOD_LAYER4,
	V_SQL_TRANSPOSE_MOD_MAX,
} V_SQL_TRANSPOSE_MOD;
*/
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
	V_SQL_CVSCALE_1VOCT = 0,
	V_SQL_CVSCALE_1_2VOCT,
	V_SQL_CVSCALE_HZVOLT,
	V_SQL_CVSCALE_MAX
} V_SQL_CVSCALE;

typedef enum:byte {
	V_SQL_CVGLIDE_OFF= 0,
	V_SQL_CVGLIDE_ON
} V_SQL_CVGLIDE;

typedef enum:byte {
	V_SQL_TRAN_TRIG_ORIG,
	V_SQL_TRAN_TRIG_THIS,
	V_SQL_TRAN_TRIG_AND,
	V_SQL_TRAN_TRIG_OR,
	V_SQL_TRAN_TRIG_XOR,
	V_SQL_TRAN_TRIG_MAX
} V_SQL_TRAN_TRIG;

typedef enum:byte {
	V_SQL_TRAN_ACC_ACCENT,
	V_SQL_TRAN_ACC_LOCK
} V_SQL_TRAN_ACC;
/*
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
*/
/*
typedef enum:byte {
	V_SQL_TRANSP_SRC_NONE = 0,
	V_SQL_TRANSP_SRC_LAYER1,
	V_SQL_TRANSP_SRC_LAYER2,
	V_SQL_TRANSP_SRC_LAYER3,
	V_SQL_TRANSP_SRC_LAYER4,
	V_SQL_TRANSP_SRC_MAX
} V_SQL_TRANSP_SRC;
*/
typedef enum:byte {
	V_CLOCK_SRC_INTERNAL = 0,
	V_CLOCK_SRC_MIDI,
	V_CLOCK_SRC_EXTERNAL,
} V_CLOCK_SRC;

typedef enum:byte {
	V_CLOCK_IN_RATE_16 = 0,
	V_CLOCK_IN_RATE_8,
	V_CLOCK_IN_RATE_4,
	V_CLOCK_IN_RATE_24PPQN,
	V_CLOCK_IN_RATE_MAX
} V_CLOCK_IN_RATE;

typedef enum:byte {
	V_CLOCK_OUT_RATE_1 = 0,
	V_CLOCK_OUT_RATE_2D,
	V_CLOCK_OUT_RATE_2,
	V_CLOCK_OUT_RATE_4D,
	V_CLOCK_OUT_RATE_2T,
	V_CLOCK_OUT_RATE_4,
	V_CLOCK_OUT_RATE_8D,
	V_CLOCK_OUT_RATE_4T,
	V_CLOCK_OUT_RATE_8,
	V_CLOCK_OUT_RATE_16D,
	V_CLOCK_OUT_RATE_8T,
	V_CLOCK_OUT_RATE_16,
	V_CLOCK_OUT_RATE_16T,
	V_CLOCK_OUT_RATE_32,
	V_CLOCK_OUT_RATE_24PPQN,
	V_CLOCK_OUT_RATE_MAX
} V_CLOCK_OUT_RATE;

extern void fire_event(int event, uint32_t param);
extern void fire_note(byte midi_note, byte midi_vel);
extern void force_full_repaint();



#endif /* DEFS_H_ */
