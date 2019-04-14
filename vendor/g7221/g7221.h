#ifndef __G7221_H__
#define __G7221_H__

#ifdef __cplusplus
extern "C" {
#endif

/* DEFINES */
#define DCT_LENGTH 320
#define HALF_DCT_LENGTH 160

/* ENUMS */
typedef enum {
    ENC_OK = 0,
	ENC_FAILED
}encStatus;

typedef enum {
    DEC_OK = 0,
	DEC_FAILED
}decStatus;

/* STRUCTURES */
typedef struct {
	unsigned int bitrate;			
	short statebuf[DCT_LENGTH];
}encG722_1State;

typedef struct {
	unsigned int bitrate;	
}encG722_1Config;

typedef struct {
    short seed0;
    short seed1;
    short seed2;
    short seed3;
}Rand_Obj;

typedef struct {
	unsigned int bitrate;
	short statebuf[HALF_DCT_LENGTH];
	short fe_buf[HALF_DCT_LENGTH];
	short fe_buf_used;
	Rand_Obj rand_obj;
}decG722_1State;

typedef struct {
	unsigned int bitrate;	
}decG722_1Config;


/* FUNCTIONS */
/** encode **/
encStatus encG7221Open(encG722_1State * state, encG722_1Config * config);
encStatus encG7221Close(encG722_1State * state);
void encG7221Reset(encG722_1State * state);
encStatus encG7221SetConfig(encG722_1State * state, encG722_1Config * config);
encStatus encG7221GetConfig(encG722_1State * state, encG722_1Config * config);
encStatus encG7221Encode(encG722_1State * state,
	short * indata, short * codewords);

/** decode **/
decStatus decG7221Open(decG722_1State * state, decG722_1Config * config);
decStatus decG7221Close(decG722_1State * state);
void decG7221Reset(decG722_1State * state);
decStatus decG7221SetConfig(decG722_1State * state, decG722_1Config * config);
decStatus decG7221GetConfig(decG722_1State * state, decG722_1Config * config);
decStatus decG7221Decode(decG722_1State * state,
	short * indata, short * outdata, short fe_flag);

#ifdef __cplusplus
}
#endif

#endif /* __G7221_H__ */
