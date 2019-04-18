/* ITU-T G.729 Software Package Release 2 (November 2006) */
/*
   ITU-T G.729A Speech Coder with Annex B    ANSI-C Source Code
   Version 1.3    Last modified: August 1997

   Copyright (c) 1996,
   AT&T, France Telecom, NTT, Universite de Sherbrooke, Lucent Technologies,
   Rockwell International
   All rights reserved.
*/

/*-----------------------------------------------------------------*
 *   Functions Coder_ld8a and Init_Coder_ld8a                      *
 *             ~~~~~~~~~~     ~~~~~~~~~~~~~~~                      *
 *                                                                 *
 *  Init_Coder_ld8a(void);                                         *
 *                                                                 *
 *   ->Initialization of variables for the coder section.          *
 *                                                                 *
 *                                                                 *
 *  Coder_ld8a(Word16 ana[]);                                      *
 *                                                                 *
 *   ->Main coder function.                                        *
 *                                                                 *
 *                                                                 *
 *  Input:                                                         *
 *                                                                 *
 *    80 speech data should have beee copy to vector new_speech[]. *
 *    This vector is global and is declared in this function.      *
 *                                                                 *
 *  Ouputs:                                                        *
 *                                                                 *
 *    ana[]      ->analysis parameters.                            *
 *                                                                 *
 *-----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include "typedef.h"
#include "basic_op.h"
#include "ld8a.h"
#include "vad.h"
#include "dtx.h"
#include "sid.h"
#include "tab_ld8a.h"

/*-----------------------------------------------------------*
 *    Coder constant parameters (defined in "ld8a.h")        *
 *-----------------------------------------------------------*
 *   L_WINDOW    : LPC analysis window size.                 *
 *   L_NEXT      : Samples of next frame needed for autocor. *
 *   L_FRAME     : Frame size.                               *
 *   L_SUBFR     : Sub-frame size.                           *
 *   M           : LPC order.                                *
 *   MP1         : LPC order+1                               *
 *   L_TOTAL     : Total size of speech buffer.              *
 *   PIT_MIN     : Minimum pitch lag.                        *
 *   PIT_MAX     : Maximum pitch lag.                        *
 *   L_INTERPOL  : Length of filter for interpolation        *
 *-----------------------------------------------------------*/

/*-----------------------------------------------------------------*
 *   Function  Init_Coder_ld8a                                     *
 *            ~~~~~~~~~~~~~~~                                      *
 *                                                                 *
 *  Init_Coder_ld8a(void);                                         *
 *                                                                 *
 *   ->Initialization of variables for the coder section.          *
 *       - initialize pointers to speech buffer                    *
 *       - initialize pointers                                     *
 *       - set vectors to zero                                     *
 *                                                                 *
 *-----------------------------------------------------------------*/

void
Init_Coder_ld8a(CodState *coder)
{
	/*----------------------------------------------------------------------*
	*      Initialize pointers to speech vector.                            *
	*                                                                       *
	*                                                                       *
	*   |--------------------|-------------|-------------|------------|     *
	*     previous speech           sf1           sf2         L_NEXT        *
	*                                                                       *
	*   <----------------  Total speech vector (L_TOTAL)   ----------->     *
	*   <----------------  LPC analysis window (L_WINDOW)  ----------->     *
	*   |                   <-- present frame (L_FRAME) -->                 *
	* old_speech            |              <-- new speech (L_FRAME) -->     *
	* p_window              |              |                                *
	*                     speech           |                                *
	*                             new_speech                                *
	*-----------------------------------------------------------------------*/

  coder->new_speech = coder->old_speech + L_TOTAL - L_FRAME;   /* New speech     */
  coder->speech     = coder->new_speech - L_NEXT;              /* Present frame  */
  coder->p_window   = coder->old_speech + L_TOTAL - L_WINDOW;  /* For LPC window */

  /* Initialize pointers */

  coder->wsp    = coder->old_wsp + PIT_MAX;
  coder->exc    = coder->old_exc + PIT_MAX + L_INTERPOL;

  /* Static vectors to zero */

  Set_zero(coder->old_speech, L_TOTAL);
  Set_zero(coder->old_exc, PIT_MAX+L_INTERPOL);
  Set_zero(coder->old_wsp, PIT_MAX);
  Set_zero(coder->mem_w,   M);
  Set_zero(coder->mem_w0,  M);
  Set_zero(coder->mem_zero, M);
  coder->sharp = SHARPMIN;

  /* Initialize lsp_old_q[] */
  Copy(lsp_old_init, coder->lsp_old, M);
  Copy(lsp_old_init, coder->lsp_old_q, M);
  Lsp_encw_reset(coder);
  Init_exc_err(coder);

  Copy(past_qua_en_init, coder->past_qua_en, 4);
  Copy(old_A_init, coder->old_A, M+1);
  Copy(old_rc_init, coder->old_rc, 2);

  /* For G.729B */
  /* Initialize VAD/DTX parameters */
  coder->pastVad = 1;
  coder->ppastVad = 1;
  coder->seed = INIT_SEED;
  vad_init(coder);
  //Init_lsfq_noise(); // DK - This is used by the decoder
}

/*-----------------------------------------------------------------*
 *   Functions Coder_ld8a                                          *
 *            ~~~~~~~~~~                                           *
 *  Coder_ld8a(Word16 ana[]);                                      *
 *                                                                 *
 *   ->Main coder function.                                        *
 *                                                                 *
 *                                                                 *
 *  Input:                                                         *
 *                                                                 *
 *    80 speech data should have beee copy to vector new_speech[]. *
 *    This vector is global and is declared in this function.      *
 *                                                                 *
 *  Ouputs:                                                        *
 *                                                                 *
 *    ana[]      ->analysis parameters.                            *
 *                                                                 *
 *-----------------------------------------------------------------*/

void Coder_ld8a(
     CodState *coder, 
     Word16 ana[],       /* output  : Analysis parameters */
     Word16 frame,       /* input   : frame counter       */
     Word16 vad_enable   /* input   : VAD enable flag     */
)
{

  /* LPC analysis */

  Word16 Aq_t[(MP1)*2];         /* A(z)   quantized for the 2 subframes */
  Word16 Ap_t[(MP1)*2];         /* A(z/gamma)       for the 2 subframes */
  Word16 *Aq, *Ap;              /* Pointer on Aq_t and Ap_t             */

  /* Other vectors */

  Word16 h1[L_SUBFR];            /* Impulse response h1[]              */
  Word16 xn[L_SUBFR];            /* Target vector for pitch search     */
  Word16 xn2[L_SUBFR];           /* Target vector for codebook search  */
  Word16 code[L_SUBFR];          /* Fixed codebook excitation          */
  Word16 y1[L_SUBFR];            /* Filtered adaptive excitation       */
  Word16 y2[L_SUBFR];            /* Filtered fixed codebook excitation */
  Word16 g_coeff[4];             /* Correlations between xn & y1       */

  Word16 g_coeff_cs[5];
  Word16 exp_g_coeff_cs[5];      /* Correlations between xn, y1, & y2
                                     <y1,y1>, -2<xn,y1>,
                                          <y2,y2>, -2<xn,y2>, 2<y1,y2> */

  /* Scalars */

  Word16 i, j, k, i_subfr;
  Word16 T_op, T0, T0_min, T0_max, T0_frac;
  Word16 gain_pit, gain_code, index;
  Word16 temp, taming;
  Word32 L_temp;
  Flag   Overflow = 0;

/*------------------------------------------------------------------------*
 *  - Perform LPC analysis:                                               *
 *       * autocorrelation + lag windowing                                *
 *       * Levinson-durbin algorithm to find a[]                          *
 *       * convert a[] to lsp[]                                           *
 *       * quantize and code the LSPs                                     *
 *       * find the interpolated LSPs and convert to a[] for the 2        *
 *         subframes (both quantized and unquantized)                     *
 *------------------------------------------------------------------------*/
  {
     /* Temporary vectors */
    Word16 r_l[NP+1], r_h[NP+1];     /* Autocorrelations low and hi          */
    Word16 rc[M];                    /* Reflection coefficients.             */
    Word16 lsp_new[M], lsp_new_q[M]; /* LSPs at 2th subframe                 */

    /* For G.729B */
    Word16 rh_nbe[MP1];             
    Word16 lsf_new[M];
    Word16 lsfq_mem[MA_NP][M];
    Word16 exp_R0, Vad;

    /* LP analysis */
    Autocorr(coder->p_window, NP, r_h, r_l, &exp_R0);    /* Autocorrelations */
    Copy(r_h, rh_nbe, MP1);
    Lag_window(NP, r_h, r_l);                            /* Lag windowing    */
    Levinson(coder, r_h, r_l, Ap_t, rc, &temp);          /* Levinson Durbin  */
    Az_lsp(Ap_t, lsp_new, coder->lsp_old);               /* From A(z) to lsp */

    /* For G.729B */
    /* ------ VAD ------- */
    Lsp_lsf(lsp_new, lsf_new, M);
    vad(coder, rc[1], lsf_new, r_h, r_l, exp_R0, coder->p_window, frame, 
        coder->pastVad, coder->ppastVad, &Vad);

    Update_cng(coder, rh_nbe, exp_R0, Vad);
    
    /* ---------------------- */
    /* Case of Inactive frame */
    /* ---------------------- */

    if ((Vad == 0) && (vad_enable == 1)){

      Get_freq_prev(coder, lsfq_mem);
      Cod_cng(coder, Aq_t, ana, lsfq_mem);
      Update_freq_prev(coder, lsfq_mem);
      coder->ppastVad = coder->pastVad;
      coder->pastVad = Vad;

      /* Update wsp, mem_w and mem_w0 */
      Aq = Aq_t;
      for(i_subfr=0; i_subfr < L_FRAME; i_subfr += L_SUBFR) {
        
        /* Residual signal in xn */
        Residu(Aq, &coder->speech[i_subfr], xn, L_SUBFR);
        
        Weight_Az(Aq, GAMMA1, M, Ap_t);
        
        /* Compute wsp and mem_w */
        Ap = Ap_t + MP1;
        Ap[0] = 4096;
        for(i=1; i<=M; i++)    /* Ap[i] = Ap_t[i] - 0.7 * Ap_t[i-1]; */
          Ap[i] = sub(Ap_t[i], mult(Ap_t[i-1], 22938, &Overflow), &Overflow);
        Syn_filt(Ap, xn, &coder->wsp[i_subfr], L_SUBFR,coder-> mem_w, 1);
        
        /* Compute mem_w0 */
        for(i=0; i<L_SUBFR; i++) {
          xn[i] = sub(xn[i], coder->exc[i_subfr+i], &Overflow);  /* residu[] - exc[] */
        }
        Syn_filt(Ap_t, xn, xn, L_SUBFR, coder->mem_w0, 1);
                
        Aq += MP1;
      }
      
      
      coder->sharp = SHARPMIN;
      
      /* Update memories for next frames */
      Copy(&coder->old_speech[L_FRAME], &coder->old_speech[0], L_TOTAL-L_FRAME);
      Copy(&coder->old_wsp[L_FRAME], &coder->old_wsp[0], PIT_MAX);
      Copy(&coder->old_exc[L_FRAME], &coder->old_exc[0], PIT_MAX+L_INTERPOL);
      
      return;
    }  /* End of inactive frame case */
    


    /* -------------------- */
    /* Case of Active frame */
    /* -------------------- */
    
    /* Case of active frame */
    *ana++ = 1;
    coder->seed = INIT_SEED;
    coder->ppastVad = coder->pastVad;
    coder->pastVad = Vad;

    /* LSP quantization */
    Qua_lsp(coder, lsp_new, lsp_new_q, ana);
    ana += 2;                         /* Advance analysis parameters pointer */

    /*--------------------------------------------------------------------*
     * Find interpolated LPC parameters in all subframes                  *
     * The interpolated parameters are in array Aq_t[].                   *
     *--------------------------------------------------------------------*/

    Int_qlpc(coder->lsp_old_q, lsp_new_q, Aq_t);

    /* Compute A(z/gamma) */

    Weight_Az(&Aq_t[0],   GAMMA1, M, &Ap_t[0]);
    Weight_Az(&Aq_t[MP1], GAMMA1, M, &Ap_t[MP1]);

    /* update the LSPs for the next frame */

    Copy(lsp_new,   coder->lsp_old,   M);
    Copy(lsp_new_q, coder->lsp_old_q, M);
  }

 /*----------------------------------------------------------------------*
  * - Find the weighted input speech w_sp[] for the whole speech frame   *
  * - Find the open-loop pitch delay                                     *
  *----------------------------------------------------------------------*/

  Residu(&Aq_t[0], &coder->speech[0], &coder->exc[0], L_SUBFR);
  Residu(&Aq_t[MP1], &coder->speech[L_SUBFR], &coder->exc[L_SUBFR], L_SUBFR);

  {
    Word16 Ap1[MP1];

    Ap = Ap_t;
    Ap1[0] = 4096;
    for(i=1; i<=M; i++)    /* Ap1[i] = Ap[i] - 0.7 * Ap[i-1]; */
       Ap1[i] = sub(Ap[i], mult(Ap[i-1], 22938, &Overflow), &Overflow);
    Syn_filt(Ap1, &coder->exc[0], &coder->wsp[0], L_SUBFR, coder->mem_w, 1);

    Ap += MP1;
    for(i=1; i<=M; i++)    /* Ap1[i] = Ap[i] - 0.7 * Ap[i-1]; */
       Ap1[i] = sub(Ap[i], mult(Ap[i-1], 22938, &Overflow), &Overflow);
    Syn_filt(Ap1, &coder->exc[L_SUBFR], &coder->wsp[L_SUBFR], L_SUBFR, coder->mem_w, 1);
  }

  /* Find open loop pitch lag */

  T_op = Pitch_ol_fast(coder->wsp, PIT_MAX, L_FRAME);

  /* Range for closed loop pitch search in 1st subframe */

  T0_min = sub(T_op, 3, &Overflow);
  if (sub(T0_min,PIT_MIN, &Overflow)<0) {
    T0_min = PIT_MIN;
  }

  T0_max = add(T0_min, 6, &Overflow);
  if (sub(T0_max ,PIT_MAX, &Overflow)>0)
  {
     T0_max = PIT_MAX;
     T0_min = sub(T0_max, 6, &Overflow);
  }


 /*------------------------------------------------------------------------*
  *          Loop for every subframe in the analysis frame                 *
  *------------------------------------------------------------------------*
  *  To find the pitch and innovation parameters. The subframe size is     *
  *  L_SUBFR and the loop is repeated 2 times.                             *
  *     - find the weighted LPC coefficients                               *
  *     - find the LPC residual signal res[]                               *
  *     - compute the target signal for pitch search                       *
  *     - compute impulse response of weighted synthesis filter (h1[])     *
  *     - find the closed-loop pitch parameters                            *
  *     - encode the pitch delay                                           *
  *     - find target vector for codebook search                           *
  *     - codebook search                                                  *
  *     - VQ of pitch and codebook gains                                   *
  *     - update states of weighting filter                                *
  *------------------------------------------------------------------------*/

  Aq = Aq_t;    /* pointer to interpolated quantized LPC parameters */
  Ap = Ap_t;    /* pointer to weighted LPC coefficients             */

  for (i_subfr = 0;  i_subfr < L_FRAME; i_subfr += L_SUBFR)
  {

    /*---------------------------------------------------------------*
     * Compute impulse response, h1[], of weighted synthesis filter  *
     *---------------------------------------------------------------*/

    h1[0] = 4096;
    Set_zero(&h1[1], L_SUBFR-1);
    Syn_filt(Ap, h1, h1, L_SUBFR, &h1[1], 0);

   /*----------------------------------------------------------------------*
    *  Find the target vector for pitch search:                            *
    *----------------------------------------------------------------------*/

    Syn_filt(Ap, &coder->exc[i_subfr], xn, L_SUBFR, coder->mem_w0, 0);

    /*---------------------------------------------------------------------*
     *                 Closed-loop fractional pitch search                 *
     *---------------------------------------------------------------------*/

    T0 = Pitch_fr3_fast(&coder->exc[i_subfr], xn, h1, L_SUBFR, T0_min, T0_max,
                    i_subfr, &T0_frac);

    index = Enc_lag3(T0, T0_frac, &T0_min, &T0_max,PIT_MIN,PIT_MAX,i_subfr);

    *ana++ = index;

    if (i_subfr == 0) {
      *ana++ = Parity_Pitch(index);
    }

   /*-----------------------------------------------------------------*
    *   - find filtered pitch exc                                     *
    *   - compute pitch gain and limit between 0 and 1.2              *
    *   - update target vector for codebook search                    *
    *-----------------------------------------------------------------*/

    Syn_filt(Ap, &coder->exc[i_subfr], y1, L_SUBFR, coder->mem_zero, 0);

    gain_pit = G_pitch(xn, y1, g_coeff, L_SUBFR);

    /* clip pitch gain if taming is necessary */

    taming = test_err(coder, T0, T0_frac);

    if( taming == 1){
      if (sub(gain_pit, GPCLIP, &Overflow) > 0) {
        gain_pit = GPCLIP;
      }
    }

    /* xn2[i]   = xn[i] - y1[i] * gain_pit  */

    for (i = 0; i < L_SUBFR; i++)
    {
      L_temp = L_mult(y1[i], gain_pit, &Overflow);
      L_temp = L_shl(L_temp, 1, &Overflow);               /* gain_pit in Q14 */
      xn2[i] = sub(xn[i], extract_h(L_temp), &Overflow);
    }


   /*-----------------------------------------------------*
    * - Innovative codebook search.                       *
    *-----------------------------------------------------*/

    index = ACELP_Code_A(xn2, h1, T0, coder->sharp, code, y2, &i);

    *ana++ = index;        /* Positions index */
    *ana++ = i;            /* Signs index     */


   /*-----------------------------------------------------*
    * - Quantization of gains.                            *
    *-----------------------------------------------------*/

    g_coeff_cs[0]     = g_coeff[0];            /* <y1,y1> */
    exp_g_coeff_cs[0] = negate(g_coeff[1]);    /* Q-Format:XXX -> JPN */
    g_coeff_cs[1]     = negate(g_coeff[2]);    /* (xn,y1) -> -2<xn,y1> */
    exp_g_coeff_cs[1] = negate(add(g_coeff[3], 1, &Overflow)); /* Q-Format:XXX -> JPN */

    Corr_xy2( xn, y1, y2, g_coeff_cs, exp_g_coeff_cs );  /* Q0 Q0 Q12 ^Qx ^Q0 */
                         /* g_coeff_cs[3]:exp_g_coeff_cs[3] = <y2,y2>   */
                         /* g_coeff_cs[4]:exp_g_coeff_cs[4] = -2<xn,y2> */
                         /* g_coeff_cs[5]:exp_g_coeff_cs[5] = 2<y1,y2>  */

    *ana++ = Qua_gain(coder, code, g_coeff_cs, exp_g_coeff_cs,
                         L_SUBFR, &gain_pit, &gain_code, taming);


   /*------------------------------------------------------------*
    * - Update pitch sharpening "sharp" with quantized gain_pit  *
    *------------------------------------------------------------*/

    coder->sharp = gain_pit;
    if (sub(coder->sharp, SHARPMAX, &Overflow) > 0) { coder->sharp = SHARPMAX;         }
    if (sub(coder->sharp, SHARPMIN, &Overflow) < 0) { coder->sharp = SHARPMIN;         }

   /*------------------------------------------------------*
    * - Find the total excitation                          *
    * - update filters memories for finding the target     *
    *   vector in the next subframe                        *
    *------------------------------------------------------*/

    for (i = 0; i < L_SUBFR;  i++)
    {
      /* exc[i] = gain_pit*exc[i] + gain_code*code[i]; */
      /* exc[i]  in Q0   gain_pit in Q14               */
      /* code[i] in Q13  gain_cod in Q1                */

      L_temp = L_mult(coder->exc[i+i_subfr], gain_pit, &Overflow);
      L_temp = L_mac(L_temp, code[i], gain_code, &Overflow);
      L_temp = L_shl(L_temp, 1, &Overflow);
      coder->exc[i+i_subfr] = W_round(L_temp, &Overflow);
    }

    update_exc_err(gain_pit, T0, coder->L_exc_err);

    for (i = L_SUBFR-M, j = 0; i < L_SUBFR; i++, j++)
    {
      temp       = extract_h(L_shl( L_mult(y1[i], gain_pit, &Overflow), 1, &Overflow));
      k          = extract_h(L_shl( L_mult(y2[i], gain_code, &Overflow), 2, &Overflow));
      coder->mem_w0[j]  = sub(xn[i], add(temp, k, &Overflow), &Overflow);
    }

    Aq += MP1;           /* interpolated LPC parameters for next subframe */
    Ap += MP1;

  }

 /*--------------------------------------------------*
  * Update signal for next frame.                    *
  * -> shift to the left by L_FRAME:                 *
  *     speech[], wsp[] and  exc[]                   *
  *--------------------------------------------------*/

  Copy(&coder->old_speech[L_FRAME], &coder->old_speech[0], L_TOTAL-L_FRAME);
  Copy(&coder->old_wsp[L_FRAME], &coder->old_wsp[0], PIT_MAX);
  Copy(&coder->old_exc[L_FRAME], &coder->old_exc[0], PIT_MAX+L_INTERPOL);

  return;
}
