/*
 *  Copyright (c) 2014 Broadsoft. All Rights Reserved.
 *
 *  Authors: 
 *           Danail Kirov
 */

#ifndef MODULES_AUDIO_CODING_CODECS_G729_MAIN_INTERFACE_G729_INTERFACE_H_
#define MODULES_AUDIO_CODING_CODECS_G729_MAIN_INTERFACE_G729_INTERFACE_H_

#include "webrtc/typedefs.h"

//
// Macros to enable extra debugging and test functionality.
//

#if defined(_MSC_VER) && defined(_DEBUG)
//
// Extra debugging and tracing is disabled in release mode and on non windows platforms.
//

//
// Do not enable the following macros, unless you know what you are doing.
//

//#define DEBUG_GARBAGE_ENCODER
//#define DEBUG_INVALID_FRAME_SIZE

#endif

/*
 * Solution to support multiple instances
 */

typedef struct WebRtcG729EncInst    G729_encinst_t;
typedef struct WebRtcG729DecInst    G729_decinst_t;

/*
 * Comfort noise constants
 */

#define G729_WEBRTC_SPEECH     1
#define G729_WEBRTC_CNG        2

#ifdef __cplusplus
extern "C" {
#endif


/****************************************************************************
 * WebRtcG729_CreateEnc(...)
 *
 * Create memory used for G729 encoder
 *
 * Input:
 *     - G729enc_inst         : G729 instance for encoder
 *
 * Return value               :  0 - Ok
 *                              -1 - Error
 */
int16_t WebRtcG729_CreateEnc(G729_encinst_t **G729enc_inst);


/****************************************************************************
 * WebRtcG729_EncoderInit(...)
 *
 * This function initializes a G729 instance
 *
 * Input:
 *     - G729enc_inst         : G729 instance, i.e. the user that should receive
 *                             be initialized
 *
 * Return value               :  0 - Ok
 *                              -1 - Error
 */

int16_t WebRtcG729_EncoderInit(G729_encinst_t *encInst, int16_t mode);

int16_t WebRtcG729_SetMode(G729_encinst_t *G729enc_inst, int16_t mode);

/****************************************************************************
 * WebRtcG729_FreeEncoder(...)
 *
 * Free the memory used for G729 encoder
 *
 * Input:
 *     - G729enc_inst         : G729 instance for encoder
 *
 * Return value               :  0 - Ok
 *                              -1 - Error
 */
int16_t WebRtcG729_FreeEnc(G729_encinst_t *G729enc_inst);



/****************************************************************************
 * WebRtcG729_Encode(...)
 *
 * This function encodes G729 encoded data.
 *
 * Input:
 *     - G729enc_inst         : G729 instance, i.e. the user that should encode
 *                              a packet
 *     - speechIn             : Input speech vector
 *     - len                  : Samples in speechIn
 *
 * Output:
 *        - encoded           : The encoded data vector
 *
 * Return value               : >0 - Length (in bytes) of coded data
 *                              -1 - Error
 */

int16_t WebRtcG729_Encode(G729_encinst_t *G729enc_inst,
                          const int16_t *speechIn,
                          int16_t len,
                          uint8_t *encoded);


/****************************************************************************
 * WebRtcG729_CreateDecoder(...)
 *
 * Create memory used for G729 encoder
 *
 * Input:
 *     - G729dec_inst         : G729 instance for decoder
 *
 * Return value               :  0 - Ok
 *                              -1 - Error
 */
int16_t WebRtcG729_CreateDec(G729_decinst_t **G729dec_inst);


/****************************************************************************
 * WebRtcG729_DecoderInit(...)
 *
 * This function initializes a G729 instance
 *
 * Input:
 *     - G729_decinst_t    : G729 instance, i.e. the user that should receive
 *                           be initialized
 *
 * Return value            :  0 - Ok
 *                           -1 - Error
 */

int16_t WebRtcG729_DecoderInit(G729_decinst_t *G729dec_inst);


/****************************************************************************
 * WebRtcG729_FreeDecoder(...)
 *
 * Free the memory used for G729 decoder
 *
 * Input:
 *     - G729dec_inst         : G729 instance for decoder
 *
 * Return value               :  0 - Ok
 *                              -1 - Error
 */

int16_t WebRtcG729_FreeDec(G729_decinst_t *G729dec_inst);


/****************************************************************************
 * WebRtcG729_Decode(...)
 *
 * This function decodes a packet with G729 frame(s). Output speech length
 * will be a multiple of 80 samples (80*frames/packet).
 *
 * Input:
 *     - G729dec_inst       : G729 instance, i.e. the user that should decode
 *                            a packet
 *     - encoded            : Encoded G729 frame(s)
 *     - len                : Bytes in encoded vector
 *
 * Output:
 *        - decoded         : The decoded vector
 *      - speechType        : 1 normal, 2 CNG
 *
 * Return value             : >0 - Samples in decoded vector
 *                            -1 - Error
 */

int16_t WebRtcG729_Decode(G729_decinst_t *G729dec_inst,
                          int16_t *encoded,
                          int16_t len,
                          int16_t *decoded,
                          int16_t *speechType);

/****************************************************************************
 * WebRtcG729_Version(...)
 *
 * Get a string with the current version of the codec
 */

int16_t WebRtcG729_Version(char *versionStr, short len);


#ifdef __cplusplus
}
#endif


#endif /* MODULES_AUDIO_CODING_CODECS_G729_MAIN_INTERFACE_G729_INTERFACE_H_ */
