/* ITU-T G.729 Software Package Release 2 (November 2006) */
/*
   ITU-T G.729A Annex B     ANSI-C Source Code
   Version 1.3    Last modified: August 1997
   Copyright (c) 1996, France Telecom, Rockwell International,
                       Universite de Sherbrooke.
   All rights reserved.
*/

/*--------------------------------------------------------------------------*
 * Prototypes for DTX/CNG                                                   *
 *--------------------------------------------------------------------------*/

/* Encoder DTX/CNG functions */
void Init_Cod_cng(CodState *coder);
void Cod_cng(
  CodState *coder,
  Word16 *Aq,           /* (o)   : set of interpolated LPC coefficients */
  Word16 *ana,          /* (o)   : coded SID parameters                 */
  Word16 freq_prev[MA_NP][M]
                        /* (i/o) : previous LPS for quantization        */
);
void Update_cng(
  CodState *coder,
  Word16 *r_h,      /* (i) :   MSB of frame autocorrelation        */
  Word16 exp_r,     /* (i) :   scaling factor associated           */
  Word16 Vad        /* (i) :   current Vad decision                */
);

/* SID gain Quantization */
void Qua_Sidgain(
  Word16 *ener,     /* (i)   array of energies                   */
  Word16 *sh_ener,  /* (i)   corresponding scaling factors       */
  Word16 nb_ener,   /* (i)   number of energies or               */
  Word16 *enerq,    /* (o)   decoded energies in dB              */
  Word16 *idx       /* (o)   SID gain quantization index         */
);

/* CNG excitation generation */
void Calc_exc_rand(
  Word16 cur_gain,      /* (i)   :   target sample gain                 */
  Word16 *exc,          /* (i/o) :   excitation array                   */
  Word16 *seed,         /* (i)   :   current Vad decision               */
  Word32 *L_exc_err
);

/* SID LSP Quantization */
void Get_freq_prev(CodState *coder, Word16 x[MA_NP][M]);
void Update_freq_prev(CodState *coder, Word16 x[MA_NP][M]);
void Get_decfreq_prev(DecState* decoder, Word16 x[MA_NP][M]);
void Update_decfreq_prev(DecState* decoder, Word16 x[MA_NP][M]);

/* Decoder CNG generation */
void Init_Dec_cng(DecState *decoder);
void Dec_cng(
  DecState *decoder,
  Word16 *parm,         /* (i)   : coded SID parameters                 */
  Word16 *A_t,          /* (o)   : set of interpolated LPC coefficients */
  Word16 freq_prev[MA_NP][M]
                        /* (i/o) : previous LPS for quantization        */
);
Word16 read_frame(FILE *f_serial, Word16 *parm);

