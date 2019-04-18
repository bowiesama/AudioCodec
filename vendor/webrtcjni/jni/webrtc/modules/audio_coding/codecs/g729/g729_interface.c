/*
 *  Copyright (c) 2014 Broadsoft. All Rights Reserved.
 *
 *  Authors: 
 *           Danail Kirov
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "g729_interface.h"
#include "bit_stream.h"

// webrtc includes
#include "webrtc/typedefs.h"

// g.729 codec includes
#include "typedef.h"
#include "basic_op.h"
#include "ld8a.h"
#include "dtx.h"
#include "octet.h"
#include "tab_ld8a.h"

//
// Macros to enable extra debugging and test functionality.
//

#if defined(_MSC_VER) && defined(_DEBUG)
//
// Extra debugging and tracing is disabled in release mode and on non windows platforms.
//
#include <windows.h>
#include "dk_utils.h"

//
// The "native" WebRTC tracing is "C++" based and can not be used in "C" code modules.
//
#define G729_TRACE
//#define DEBUG_TIMING
//#define STORE_RECV_PCM
//#define FFMPEG_CMP

#ifdef FFMPEG_CMP
#define _ALLOW_KEYWORD_MACROS
#define RC_INVOKED
#define EINVAL          22

#include "libavcodec\get_bits.h"
#include "libavcodec\put_bits.h"
#endif

#endif


typedef struct
{
	unsigned	instance;
	int			initialized;

	Word16 prm[PRM_SIZE + 1];		// Analysis parameters + frame type

	Word16 frame;					// frame counter ?

	// For G.729 Annex B
	Word16 vad_enable;

#ifdef DEBUG_TIMING
	uint64_t msec;
#endif

	CodState coder;

} g729_encode_state_t;

typedef struct
{
	unsigned	instance;
	int			initialized;

	Word16  parm[PRM_SIZE + 2];    // Synthesis parameters

	// For G.729 Annex B
	Word16  Vad;

#ifdef STORE_RECV_PCM
	FILE	*RecvPcmFile;
#endif

#ifdef DEBUG_TIMING
	uint64_t msec;
#endif

	DecState decoder;

} g729_decode_state_t;


// Non protected increments / decrements used for debugging and better tracing only for now. 
// Not sure if webrtc ensures thread protection.
// If needed these could be better lock protected in the future.
static unsigned num_enc_instances = 0;
static unsigned num_dec_instances = 0;
static unsigned enc_counter = 0;
static unsigned dec_counter = 0;

void bits2prm_dk(
	uint8_t  *bits,     /* input : serial bits (80)                       */
	uint16_t nb_bits,
	Word16   prm[]      /* output: decoded parameters (11 parameters)     */
	)
{
	BsrContext bsr;
	int i;

#ifdef FFMPEG_CMP
	GetBitContext gbc;
	Word16 prm_ref[PRM_SIZE + 2];
#endif

	if (nb_bits == RATE_8000) 
	{
		bsr_init(&bsr, bits, 80);

#ifdef FFMPEG_CMP
		init_get_bits(&gbc, bits, 80);
#endif

		prm[1] = 1;
		for (i = 0; i < PRM_SIZE; i++) 
		{
			prm[i + 2] = bsr_get_bits_safe(&bsr, bitsno[i]);

#ifdef FFMPEG_CMP
			prm_ref[i + 2] = get_bits(&gbc, bitsno[i]);
			if (prm[i + 2] != prm_ref[i + 2])
			{
				assert(prm[i + 2] == prm_ref[i + 2]);
			}
#endif
		}
	}
	else
	{
		// the last bit of the SID bit stream under octet mode will be discarded
		if (nb_bits == RATE_SID_OCTET) {
			bsr_init(&bsr, bits, 16);
			prm[1] = 2;
			for (i = 0; i < 4; i++) {
				prm[i + 2] = bsr_get_bits_safe(&bsr, bitsno2[i]);
			}
		}
		else {
			prm[1] = 0;
		}
	}
	
	return;
}

int16_t prm2bits_dk(
	Word16 prm[],    /* input : encoded parameters  (PRM_SIZE parameters)  */
	uint8_t *bits    /* output: serial bits (SERIAL_SIZE )*/
	)
{
	BswContext pb;
	int i;
	int16_t num_octets = 0;

#ifdef FFMPEG_CMP
	PutBitContext pbc;
	uint8_t bits_ref[10];
	memset(bits_ref, 0, 10);
	memset(bits, 0, 10);
#endif

	switch (prm[0])
	{
	// not transmitted
	case 0:
		num_octets = 0;
		break;

	case 1:
		num_octets = 10;
		bsw_init(&pb, bits, 10);
#ifdef FFMPEG_CMP
		init_put_bits(&pbc, bits_ref, 10);
#endif
		for (i = 0; i < PRM_SIZE; ++i)
		{
			bsw_put_bits(&pb, bitsno[i], prm[i + 1]);
#ifdef FFMPEG_CMP
			put_bits(&pbc, bitsno[i], prm[i + 1]);
			if (memcmp(bits_ref, bits, 10))
			{
				assert(0);
			}
#endif
		}

		break;

	case 2:
		num_octets = 2;
		bsw_init(&pb, bits, 2);
		for (i = 0; i < 4; i++) {
			bsw_put_bits(&pb, bitsno2[i], prm[i+1]);
		}
		bsw_put_bits(&pb, 1, 0);
		break;
	}

	if (num_octets)
		bsw_flush(&pb);

#ifdef FFMPEG_CMP
	flush_put_bits(&pbc);
	if (memcmp(bits_ref, bits, 10))
	{
		assert(0);
	}
#endif

#ifdef DEBUG_GARBAGE_ENCODER
	//
	// replace output with random bits i.e. garbage
	// to test and make sure the decoding modules do not crash 
	// when interfacing with malicious or misbehaving endpoints
	//
	{
		int j;
		//
		//random_value = (double) rand() / (RAND_MAX + 1) * (range_max - range_min) + range_min;
		//

		for (j = 0; j < num_octets; j++)
			bits[j] = (uint8_t)((double)rand() / (RAND_MAX + 1) * (255 - 0) + 0);
	}

#endif

#ifdef DEBUG_INVALID_FRAME_SIZE
	//
	// change num_octets to invalid frame size
	// to test and make sure the decoding modules do not crash 
	// when interfacing with malicious or misbehaving endpoints
	
	//
	// change also ACMG729::InternalEncode to allow this invalid size
	//
	num_octets = 7;
#endif

	return num_octets;
}

int16_t WebRtcG729_Version(char *versionStr, short len)
{
	// Get version string
	char version[30] = "2.0.0\n";
	if (strlen(version) < (unsigned int)len)
	{
		strcpy(versionStr, version);
		return 0;
	}
	else
	{
		return -1;
	}
}

int16_t WebRtcG729_CreateEnc(G729_encinst_t **G729enc_inst)
{
	g729_encode_state_t *enc_state = (g729_encode_state_t*)calloc(1, sizeof(g729_encode_state_t));
	
	if (!enc_state)
		return -1;
  
  num_enc_instances++;
	enc_state->instance = enc_counter++;

#ifdef G729_TRACE
	trace_d("%u %s num_enc:%u\n", enc_state->instance, __FUNCTION__, num_enc_instances);
#endif

	*G729enc_inst=(G729_encinst_t*)enc_state;

	return 0;
}

int16_t WebRtcG729_SetMode(G729_encinst_t *G729enc_inst, int16_t mode)
{
	g729_encode_state_t *enc_state = (g729_encode_state_t*)G729enc_inst;

	if (!enc_state)
	{
		assert(enc_state);
		return -1;
	}

#ifdef G729_TRACE
	trace_d("%u %s vad_enable %d --> %d\n", enc_state->instance, __FUNCTION__, enc_state->vad_enable, mode);
#endif

	enc_state->vad_enable = mode ? 1 : 0;

	return 0;
}

int16_t WebRtcG729_EncoderInit(G729_encinst_t *G729enc_inst, int16_t mode)
{
	g729_encode_state_t *enc_state = (g729_encode_state_t*)G729enc_inst;

	if (!enc_state)
	{
		assert(enc_state);
		return -1;
	}

#ifdef G729_TRACE
	trace_d("%u %s vad_enable %d --> %d\n", enc_state->instance, __FUNCTION__, enc_state->vad_enable, mode);
#endif

	Init_Coder_ld8a(&enc_state->coder);
	Init_Pre_Process(&enc_state->coder);
	Set_zero(enc_state->prm, PRM_SIZE + 1);

	// for G.729B
	enc_state->vad_enable = mode ? 1 : 0;
	Init_Cod_cng(&enc_state->coder);

	enc_state->initialized = 1;

	return 0;
}

int16_t WebRtcG729_FreeEnc(G729_encinst_t *G729enc_inst)
{
	g729_encode_state_t *enc_state = (g729_encode_state_t*)G729enc_inst;

	if (!enc_state)
	{
		assert(enc_state);
		return -1;
	}

	num_enc_instances--;

#ifdef G729_TRACE
	trace_d("%u %s num_enc:%u\n", enc_state->instance, __FUNCTION__, num_enc_instances);
#endif
	
	free(enc_state);

	return 0;
}

int16_t WebRtcG729_Encode(G729_encinst_t *G729enc_inst,
                          const int16_t *speechIn,
                          int16_t len,
                          uint8_t *encoded)
{
	g729_encode_state_t *enc_state = (g729_encode_state_t*)G729enc_inst;
	int16_t num_octets;

#ifdef DEBUG_TIMING
	uint64_t microsec = dk_utils_time_in_microseconds();
	uint32_t delta = enc_state->msec ? (microsec / 1000 - enc_state->msec) : 0;
	enc_state->msec = microsec / 1000;
#endif

	if (!enc_state)
	{
		assert(enc_state);
		return -1;
	}

	if (!enc_state->initialized)
	{
		assert(enc_state->initialized);
		WebRtcG729_EncoderInit(G729enc_inst, 0);
	}

	//
	// This code expects single frame only.
	//
	if (len != L_FRAME)
	{
		//assert(len == L_FRAME);
		return 0;
	}
	
	// TODO - DK need to research this more
	if (enc_state->frame == 32767) 
		enc_state->frame = 256;
	else 
		enc_state->frame++;

	//
	// enc_state->coder->new_speech is the input to the encoder
	//
	memcpy(enc_state->coder.new_speech, speechIn, L_FRAME*sizeof(int16_t));

	Pre_Process(&enc_state->coder);
	Coder_ld8a(&enc_state->coder, enc_state->prm, enc_state->frame, enc_state->vad_enable);
	num_octets = prm2bits_dk(enc_state->prm, (uint8_t*)encoded);

#ifdef DEBUG_TIMING
	{
		static uint32_t delta_treshold = 0; // change the value in debugger if needed

		microsec = dk_utils_time_in_microseconds() - microsec;

		if (delta > delta_treshold)
		{
			trace_d("%u %s len:%d took:%u microsecs delta:%ums\n", enc_state->instance, __FUNCTION__, len, (uint32_t)microsec, delta);
		}
	}
#endif

	return num_octets;
}

int16_t WebRtcG729_CreateDec(G729_decinst_t **G729dec_inst)
{
	g729_decode_state_t *dec_state = (g729_decode_state_t*)calloc(1, sizeof(g729_decode_state_t));
	
	if (!dec_state)
		return -1;
  
  num_dec_instances++;
	dec_state->instance = dec_counter++;

#ifdef G729_TRACE
	trace_d("%u %s num_dec:%u\n", dec_state->instance, __FUNCTION__, num_dec_instances);
#endif

	*G729dec_inst=(G729_decinst_t*)dec_state;

	return 0;
}

int16_t WebRtcG729_DecoderInit(G729_decinst_t *G729dec_inst)
{
	g729_decode_state_t *dec_state = (g729_decode_state_t*)G729dec_inst;

	if (!dec_state)
	{
		assert(dec_state);
		return -1;
	}

	Init_Decod_ld8a(&dec_state->decoder);
	Init_Post_Filter(&dec_state->decoder);
	Init_Post_Process(&dec_state->decoder);

	// G.729 Annex B
	Init_Dec_cng(&dec_state->decoder);

	dec_state->initialized = 1;

#ifdef G729_TRACE
	trace_d("%u %s\n", dec_state->instance, __FUNCTION__);
#endif

	return 0;
}

int16_t WebRtcG729_FreeDec(G729_decinst_t *G729dec_inst)
{
	g729_decode_state_t *dec_state = (g729_decode_state_t*)G729dec_inst;

	if (!dec_state)
	{
		assert(dec_state);
		return -1;
	}

	num_dec_instances--;

#ifdef G729_TRACE
	trace_d("%u %s num_dec:%u\n", dec_state->instance, __FUNCTION__, num_dec_instances);
#endif

#ifdef STORE_RECV_PCM
	if (dec_state->RecvPcmFile)
	{
		fclose(dec_state->RecvPcmFile);
	}
#endif

	free(dec_state);

	return 0;
}

int16_t WebRtcG729_Decode(G729_decinst_t *G729dec_inst,
                          int16_t *encoded,
                          int16_t len,
                          int16_t *decoded,
                          int16_t *speechType)
{
	g729_decode_state_t *dec_state = (g729_decode_state_t*)G729dec_inst;

	Word16	Vad = 0;
	Word16	remaining_len = len;
	Word16	decoded_words = 0;

#ifdef DEBUG_TIMING
	uint64_t microsec = dk_utils_time_in_microseconds();
	uint32_t delta = dec_state->msec ? (microsec / 1000 - dec_state->msec) : 0;
	dec_state->msec = microsec / 1000;
#endif

	if (!dec_state)
	{
		assert(dec_state);
		return -1;
	}

	if (!dec_state->initialized)
	{
		assert(dec_state->initialized);
		WebRtcG729_DecoderInit(G729dec_inst);
	}

    *speechType=G729_WEBRTC_SPEECH;

	//
	// This code processes variable number of G.729 Annex A or Annex B encoded frames.
	//
	while (remaining_len)
	{
		//
		// http://tools.ietf.org/pdf/rfc3551.pdf
		// A G729 RTP packet may consist of zero or more G.729 or G.729 Annex A frames, followed by zero
		// or one G.729 Annex B frames.The presence of a comfort noise frame can be deduced from the
		// length of the RTP payload.
		//

		if (remaining_len < 2)
		{
			// invalid lenght for either type of frame
			return -1;
		}

		if (remaining_len >= 10)
		{
			// there is at least one Annex A frame
			remaining_len -= 10;
			bits2prm_dk((uint8_t*)encoded, 80, dec_state->parm);
			encoded += 5;
		}
		else
		{
			// there should be a single Annex B frame only
			// ignore the rest
			remaining_len = 0;
			bits2prm_dk((uint8_t*)encoded, 16, dec_state->parm);
			encoded += 1;
		}

#if 0	// TODO - DK need to research this more
		/* This part was modified for version V1.3 */
		/* for speech and SID frames, the hardware detects frame erasures
		by checking if all bits are set to zero */
		/* for untransmitted frames, the hardware detects frame erasures
		by testing serial[0] */
		dec_state->parm[0] = 0;           /* No frame erasure */
		if (serial[1] != 0) {
			Word16  i;
			for (i = 0; i < serial[1]; i++)
				if (serial[i + 2] == 0)
					dec_state->parm[0] = 1;  /* frame erased     */
		}
		else if (serial[0] != SYNC_WORD){
			dec_state->parm[0] = 1;
		}
#endif

		dec_state->parm[0] = 0;	//No frame erasure
		if (dec_state->parm[1] == 1) 
		{
			// check parity and put 1 in parm[5] if parity error
			dec_state->parm[5] = Check_Parity_Pitch(dec_state->parm[4], dec_state->parm[5]);
		}
	
		Decod_ld8a(&dec_state->decoder, dec_state->parm, &Vad);
		Post_Filter(&dec_state->decoder, Vad);
		Post_Process(&dec_state->decoder);

		//
		// dec_state->decoder.synth is the output from the decoder
		//
		memcpy(&decoded[decoded_words], dec_state->decoder.synth, L_FRAME * sizeof(Word16));

		decoded_words += L_FRAME;
	}

#ifdef DEBUG_TIMING
	{
		static uint32_t delta_treshold = 0; // change the value in debugger if needed

		microsec = dk_utils_time_in_microseconds() - microsec;

		if (delta > delta_treshold)
		{
			trace_d("%u %s len:%d took:%u microsecs delta:%ums\n", dec_state->instance, __FUNCTION__, len, (uint32_t)microsec, delta);
		}
	}
#endif

#ifdef STORE_RECV_PCM
	{
		if (dec_state->RecvPcmFile == NULL)
		{
			SYSTEMTIME	lt;
			char FileName[512];
			GetLocalTime(&lt);

			sprintf_s(FileName, 512, "RecvPCM %4.4u-%2.2u-%2.2u %2.2u-%2.2u-%2.2u.raw",
				lt.wYear,
				lt.wMonth,
				lt.wDay,
				lt.wHour,
				lt.wMinute,
				lt.wSecond);

			if ((dec_state->RecvPcmFile = fopen(FileName, "wb")) == NULL)
			{
				trace_d("%u %s Failed to open file %s\n", dec_state->instance, __FUNCTION__, FileName);
				assert(0);
			}
		}

		if (fwrite(decoded, sizeof(short), decoded_words, dec_state->RecvPcmFile) != (size_t)decoded_words) 
		{
			trace_d("%u %s Cannot write to RecvPCM file\n", dec_state->instance, __FUNCTION__);
			assert(0);
		}
	}
#endif

	return decoded_words;
}

