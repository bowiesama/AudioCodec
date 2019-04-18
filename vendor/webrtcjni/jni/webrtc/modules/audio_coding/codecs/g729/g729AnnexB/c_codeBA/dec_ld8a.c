/* ITU-T G.729 Software Package Release 2 (November 2006) */
/*
   ITU-T G.729A Speech Coder with Annex B    ANSI-C Source Code
   Version 1.5    Last modified: October 2006 

   Copyright (c) 1996,
   AT&T, France Telecom, NTT, Universite de Sherbrooke, Lucent Technologies,
   Rockwell International
   All rights reserved.
*/

/*-----------------------------------------------------------------*
 *   Functions Init_Decod_ld8a  and Decod_ld8a                     *
 *-----------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include "typedef.h"
#include "basic_op.h"
#include "ld8a.h"

#include "dtx.h"
#include "sid.h"
#include "tab_ld8a.h"

/*---------------------------------------------------------------*
 *   Decoder constant parameters (defined in "ld8a.h")           *
 *---------------------------------------------------------------*
 *   L_FRAME     : Frame size.                                   *
 *   L_SUBFR     : Sub-frame size.                               *
 *   M           : LPC order.                                    *
 *   MP1         : LPC order+1                                   *
 *   PIT_MIN     : Minimum pitch lag.                            *
 *   PIT_MAX     : Maximum pitch lag.                            *
 *   L_INTERPOL  : Length of filter for interpolation            *
 *   PRM_SIZE    : Size of vector containing analysis parameters *
 *---------------------------------------------------------------*/

/*-----------------------------------------------------------------*
 *   Function Init_Decod_ld8a                                      *
 *            ~~~~~~~~~~~~~~~                                      *
 *                                                                 *
 *   ->Initialization of variables for the decoder section.        *
 *                                                                 *
 *-----------------------------------------------------------------*/

void
Init_Decod_ld8a(DecState *decoder)
{
  /* Initialize pointers */

  decoder->exc = decoder->old_exc + PIT_MAX + L_INTERPOL;

  /* Static vectors to zero */

  Set_zero(decoder->old_exc, PIT_MAX+L_INTERPOL);
  Set_zero(decoder->mem_syn, M);

  Copy(lsp_old_init, decoder->lsp_old, M);
  decoder->sharp  = SHARPMIN;
  decoder->old_T0 = 60;
  decoder->gain_code = 0;
  decoder->gain_pitch = 0;

  Lsp_decw_reset(decoder);

  decoder->bad_lsf = 0;
  Set_zero(decoder->Az_dec, MP1 * 2);
  Set_zero(decoder->T2, 2);
  Set_zero(decoder->synth_buf, L_FRAME + M);
  decoder->synth = decoder->synth_buf + M;

  Copy(past_qua_en_init, decoder->past_qua_en, 4);

  /* for G.729B */
  decoder->seed_fer = 21845;
  decoder->past_ftyp = 1;
  decoder->seed = INIT_SEED;
  decoder->sid_sav = 0;
  decoder->sh_sid_sav = 1;
  Init_lsfq_noise(decoder);
}

/*-----------------------------------------------------------------*
 *   Function Decod_ld8a                                           *
 *           ~~~~~~~~~~                                            *
 *   ->Main decoder routine.                                       *
 *                                                                 *
 *-----------------------------------------------------------------*/

void Decod_ld8a(
  DecState *decoder,
  Word16  parm[],      /* (i)   : vector of synthesis parameters
                                  parm[0] = bad frame indicator (bfi)  */
  Word16  *Vad         /* (o)   : frame type                           */
)
{
  Word16  *synth = decoder->synth; /* (o)   : synthesis speech                     */
  Word16  *A_t = decoder->Az_dec;  /* (o)   : decoded LP filter in 2 subframes     */
  Word16  *T2 = decoder->T2;	   /* (o)   : decoded pitch lag in 2 subframes     */

  Word16  *Az;                  /* Pointer on A_t   */
  Word16  lsp_new[M];           /* LSPs             */
  Word16  code[L_SUBFR];        /* ACELP codevector */

  /* Scalars */
  Word16  i, j, i_subfr;
  Word16  T0, T0_frac, index;
  Word16  bfi;
  Word32  L_temp;

  Word16 bad_pitch;             /* bad pitch indicator */

  /* for G.729B */
  Word16 ftyp;
  Word16 lsfq_mem[MA_NP][M];
  Flag   Overflow = 0;

  /* Test bad frame indicator (bfi) */

  bfi = *parm++;
  /* for G.729B */
  ftyp = *parm;

  if(bfi == 1) {
    if(decoder->past_ftyp == 1) {
      ftyp = 1;
      parm[4] = 1;    /* G.729 maintenance */
    }
    else ftyp = 0;
    *parm = ftyp;  /* modification introduced in version V1.3 */
  }
  
  *Vad = ftyp;

  /* Processing non active frames (SID & not transmitted) */
  if(ftyp != 1) {
    
    Get_decfreq_prev(decoder, lsfq_mem);
	Dec_cng(decoder, parm, A_t, lsfq_mem);
    Update_decfreq_prev(decoder, lsfq_mem);

    Az = A_t;
    for (i_subfr = 0; i_subfr < L_FRAME; i_subfr += L_SUBFR) {
      Overflow = 0;
	  Syn_filt(Az, &decoder->exc[i_subfr], &synth[i_subfr], L_SUBFR, decoder->mem_syn, 0);
      if(Overflow != 0) {
        /* In case of overflow in the synthesis          */
        /* -> Scale down vector exc[] and redo synthesis */
        
        for(i=0; i<PIT_MAX+L_INTERPOL+L_FRAME; i++)
			decoder->old_exc[i] = shr(decoder->old_exc[i], 2, &Overflow);
        
		Syn_filt(Az, &decoder->exc[i_subfr], &synth[i_subfr], L_SUBFR, decoder->mem_syn, 1);
      }
      else
        Copy(&synth[i_subfr+L_SUBFR-M], decoder->mem_syn, M);
      
      Az += MP1;

	  *T2++ = decoder->old_T0;
    }
	decoder->sharp = SHARPMIN;
    
  }
  /* Processing active frame */
  else {
    
    decoder->seed = INIT_SEED;
    parm++;

    /* Decode the LSPs */
    
    D_lsp(decoder, parm, lsp_new, add(bfi, decoder->bad_lsf, &Overflow));
    parm += 2;
    
    /*
       Note: "bad_lsf" is introduce in case the standard is used with
       channel protection.
       */
    
    /* Interpolation of LPC for the 2 subframes */
    
    Int_qlpc(decoder->lsp_old, lsp_new, A_t);
    
    /* update the LSFs for the next frame */
    
    Copy(lsp_new, decoder->lsp_old, M);
    
    /*------------------------------------------------------------------------*
     *          Loop for every subframe in the analysis frame                 *
     *------------------------------------------------------------------------*
     * The subframe size is L_SUBFR and the loop is repeated L_FRAME/L_SUBFR  *
     *  times                                                                 *
     *     - decode the pitch delay                                           *
     *     - decode algebraic code                                            *
     *     - decode pitch and codebook gains                                  *
     *     - find the excitation and compute synthesis speech                 *
     *------------------------------------------------------------------------*/
    
    Az = A_t;            /* pointer to interpolated LPC parameters */
    
    for (i_subfr = 0; i_subfr < L_FRAME; i_subfr += L_SUBFR)
      {

        index = *parm++;        /* pitch index */

        if(i_subfr == 0)
          {
            i = *parm++;        /* get parity check result */
            bad_pitch = add(bfi, i, &Overflow);
            if( bad_pitch == 0)
              {
                Dec_lag3(index, PIT_MIN, PIT_MAX, i_subfr, &T0, &T0_frac);
                decoder->old_T0 = T0;
              }
            else                /* Bad frame, or parity error */
              {
                T0  =  decoder->old_T0;
                T0_frac = 0;
                decoder->old_T0 = add( decoder->old_T0, 1, &Overflow);
                if( sub(decoder->old_T0, PIT_MAX, &Overflow) > 0) {
                  decoder->old_T0 = PIT_MAX;
                }
              }
          }
        else                    /* second subframe */
          {
            if( bfi == 0)
              {
                Dec_lag3(index, PIT_MIN, PIT_MAX, i_subfr, &T0, &T0_frac);
                decoder->old_T0 = T0;
              }
            else
              {
                T0  =  decoder->old_T0;
                T0_frac = 0;
                decoder->old_T0 = add( decoder->old_T0, 1, &Overflow);
                if( sub(decoder->old_T0, PIT_MAX, &Overflow) > 0) {
                  decoder->old_T0 = PIT_MAX;
                }
              }
          }
        *T2++ = T0;

        /*-------------------------------------------------*
         * - Find the adaptive codebook vector.            *
         *-------------------------------------------------*/

        Pred_lt_3(&decoder->exc[i_subfr], T0, T0_frac, L_SUBFR);

        /*-------------------------------------------------------*
         * - Decode innovative codebook.                         *
         * - Add the fixed-gain pitch contribution to code[].    *
         *-------------------------------------------------------*/

        if(bfi != 0)            /* Bad frame */
          {

            parm[0] = Random(&decoder->seed_fer) & (Word16)0x1fff; /* 13 bits random */
            parm[1] = Random(&decoder->seed_fer) & (Word16)0x000f; /*  4 bits random */
          }

        Decod_ACELP(parm[1], parm[0], code);
        parm +=2;

        j = shl(decoder->sharp, 1, &Overflow);      /* From Q14 to Q15 */
        if(sub(T0, L_SUBFR, &Overflow) <0 ) {
          for (i = T0; i < L_SUBFR; i++) {
            code[i] = add(code[i], mult(code[i-T0], j, &Overflow), &Overflow);
          }
        }

        /*-------------------------------------------------*
         * - Decode pitch and codebook gains.              *
         *-------------------------------------------------*/

        index = *parm++;        /* index of energy VQ */

        Dec_gain(decoder, index, code, L_SUBFR, bfi, &decoder->gain_pitch, &decoder->gain_code);

        /*-------------------------------------------------------------*
         * - Update pitch sharpening "sharp" with quantized gain_pitch *
         *-------------------------------------------------------------*/

        decoder->sharp = decoder->gain_pitch;
        if (sub(decoder->sharp, SHARPMAX, &Overflow) > 0) { decoder->sharp = SHARPMAX;  }
        if (sub(decoder->sharp, SHARPMIN, &Overflow) < 0) { decoder->sharp = SHARPMIN;  }

        /*-------------------------------------------------------*
         * - Find the total excitation.                          *
         * - Find synthesis speech corresponding to exc[].       *
         *-------------------------------------------------------*/

        for (i = 0; i < L_SUBFR;  i++)
          {
            /* exc[i] = gain_pitch*exc[i] + gain_code*code[i]; */
            /* exc[i]  in Q0   gain_pitch in Q14               */
            /* code[i] in Q13  gain_codeode in Q1              */
            
            L_temp = L_mult(decoder->exc[i+i_subfr], decoder->gain_pitch, &Overflow);
            L_temp = L_mac(L_temp, code[i], decoder->gain_code, &Overflow);
            L_temp = L_shl(L_temp, 1, &Overflow);
            decoder->exc[i+i_subfr] = W_round(L_temp, &Overflow);
          }
        
        Overflow = 0;
        Syn_filt(Az, &decoder->exc[i_subfr], &synth[i_subfr], L_SUBFR, decoder->mem_syn, 0);
        if(Overflow != 0)
          {
            /* In case of overflow in the synthesis          */
            /* -> Scale down vector exc[] and redo synthesis */

            for(i=0; i<PIT_MAX+L_INTERPOL+L_FRAME; i++)
              decoder->old_exc[i] = shr(decoder->old_exc[i], 2, &Overflow);

            Syn_filt(Az, &decoder->exc[i_subfr], &synth[i_subfr], L_SUBFR, decoder->mem_syn, 1);
          }
        else
          Copy(&synth[i_subfr+L_SUBFR-M], decoder->mem_syn, M);

        Az += MP1;              /* interpolated LPC parameters for next subframe */
      }
  }
  
  /*------------*
   *  For G729b
   *-----------*/
  if(bfi == 0) {
    L_temp = 0L;
    for(i=0; i<L_FRAME; i++) {
      L_temp = L_mac(L_temp, decoder->exc[i], decoder->exc[i], &Overflow);
    } /* may overflow => last level of SID quantizer */
	decoder->sh_sid_sav = norm_l(L_temp);
	decoder->sid_sav = W_round(L_shl(L_temp, decoder->sh_sid_sav, &Overflow), &Overflow);
	decoder->sh_sid_sav = sub(16, decoder->sh_sid_sav, &Overflow);
  }

 /*--------------------------------------------------*
  * Update signal for next frame.                    *
  * -> shift to the left by L_FRAME  exc[]           *
  *--------------------------------------------------*/

  Copy(&decoder->old_exc[L_FRAME], &decoder->old_exc[0], PIT_MAX+L_INTERPOL);

  /* for G729b */
  decoder->past_ftyp = ftyp;

  return;
}






