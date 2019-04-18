/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AECM_AECM_DEFINES_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AECM_AECM_DEFINES_H_

#define AECM_DYNAMIC_Q                 /* Turn on/off dynamic Q-domain. */

/* Algorithm parameters */
#define FRAME_LEN       80             /* Total frame length, 10 ms. */

//#define FLOATPRO
//need change delay_estimator_wrapper.c too
#ifdef FLOATPRO
#define PART_LEN        256            /* Length of partition. */
#define PART_LEN_SHIFT  9             /* Length of (PART_LEN * 2) in base 2. */
#define MAX_DELAY       25
#else
#define PART_LEN        64            /* Length of partition. */
#define PART_LEN_SHIFT  7             /* Length of (PART_LEN * 2) in base 2. */
#define MAX_DELAY       100
#endif

#define PART_LEN1       (PART_LEN + 1)  /* Unique fft coefficients. */
#define PART_LEN2       (PART_LEN << 1) /* Length of partition * 2. */
#define PART_LEN4       (PART_LEN << 2) /* Length of partition * 4. */
#define FAR_BUF_LEN     PART_LEN4       /* Length of buffers. */


/* Counter parameters */
#define CONV_LEN        512          /* Convergence length used at startup. */
#define CONV_LEN2       (CONV_LEN << 1) /* Used at startup. */

/* Energy parameters */
#define MAX_BUF_LEN     64           /* History length of energy signals. */
#define FAR_ENERGY_MIN  1025         /* Lowest Far energy level: At least 2 */
                                     /* in energy. */
#define FAR_ENERGY_DIFF 929          /* Allowed difference between max */
                                     /* and min. */
#define ENERGY_DEV_OFFSET       0    /* The energy error offset in Q8. */
#define ENERGY_DEV_TOL  400          /* The energy estimation tolerance (Q8). */
#define FAR_ENERGY_VAD_REGION   230  /* Far VAD tolerance region. */

/* Stepsize parameters */
#define MU_MIN          10          /* Min stepsize 2^-MU_MIN (far end energy */
                                    /* dependent). */
#define MU_MAX          1           /* Max stepsize 2^-MU_MAX (far end energy */
                                    /* dependent). */
#define MU_DIFF         9           /* MU_MIN - MU_MAX */

/* Channel parameters */
#define MIN_MSE_COUNT   20 /* Min number of consecutive blocks with enough */
                           /* far end energy to compare channel estimates. */
#define MIN_MSE_DIFF    29 /* The ratio between adapted and stored channel to */
                           /* accept a new storage (0.8 in Q-MSE_RESOLUTION). */
#define MSE_RESOLUTION  5           /* MSE parameter resolution. */
#define MSE_RESOLUTION_FLOAT  32.0  /* LY add for float version. */    
#define RESOLUTION_CHANNEL16    12  /* W16 Channel in Q-RESOLUTION_CHANNEL16. */
#define RESOLUTION_CHANNEL32    28  /* W32 Channel in Q-RESOLUTION_CHANNEL. */
#define CHANNEL_VAD     16          /* Minimum energy in frequency band */
                                    /* to update channel. */

/* Suppression gain parameters: SUPGAIN parameters in Q-(RESOLUTION_SUPGAIN). */
#define RESOLUTION_SUPGAIN      8     /* Channel in Q-(RESOLUTION_SUPGAIN). */
#define SUPGAIN_DEFAULT (1 << RESOLUTION_SUPGAIN)  /* Default. */
#define SUPGAIN_ERROR_PARAM_A   (16 * SUPGAIN_DEFAULT)  /* Estimation error parameter */
                                      /* (Maximum gain) (8 in Q8). */
#define SUPGAIN_ERROR_PARAM_B   (8 * SUPGAIN_DEFAULT)  /* Estimation error parameter */
                                      /* (Gain before going down). */
#define SUPGAIN_ERROR_PARAM_D   (3 * SUPGAIN_DEFAULT) /* Estimation error parameter */
                                /* (Should be the same as Default) (1 in Q8). */
#define SUPGAIN_EPC_DT  200     /* SUPGAIN_ERROR_PARAM_C * ENERGY_DEV_TOL */

/* Defines for "check delay estimation" */
#define CORR_WIDTH      31      /* Number of samples to correlate over. */
#define CORR_MAX        16      /* Maximum correlation offset. */
#define CORR_MAX_BUF    63
#define CORR_DEV        4
#define CORR_MAX_LEVEL  20
#define CORR_MAX_LOW    4
#define CORR_BUF_LEN    (CORR_MAX << 1) + 1
/* Note that CORR_WIDTH + 2*CORR_MAX <= MAX_BUF_LEN. */

#define ONE_Q14         (1 << 14)

/* NLP defines */
#define NLP_COMP_LOW    8192    /* 0.5 in Q14 */
#define NLP_COMP_HIGH   ONE_Q14 /* 1 in Q14 */

/* LY add for float version. */    
#define ONE_Q8_FLOAT 256.0
#define ONE_Q16_FLOAT 65536.0
#define FAR_ENERGY_MIN_FLOAT (FAR_ENERGY_MIN / ONE_Q8_FLOAT)
#define SUPGAIN_DEFAULT_FLOAT 1.0  /* Default. */
#define SUPGAIN_ERROR_PARAM_A_FLOAT   16.0  /* Estimation error parameter */
/* (Maximum gain) (8 in Q8). */
#define SUPGAIN_ERROR_PARAM_B_FLOAT   8.0  /* Estimation error parameter */
/* (Gain before going down). */
#define SUPGAIN_ERROR_PARAM_D_FLOAT   3.0 /* Estimation error parameter */
/* (Should be the same as Default) (1 in Q8). */
#define SUPGAIN_EPC_DT_FLOAT  0.78125     /* SUPGAIN_ERROR_PARAM_C * ENERGY_DEV_TOL */

#endif
