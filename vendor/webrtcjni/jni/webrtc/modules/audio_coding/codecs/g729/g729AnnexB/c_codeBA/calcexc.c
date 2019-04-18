/* ITU-T G.729 Software Package Release 2 (November 2006) */
/*
   ITU-T G.729A Annex B     ANSI-C Source Code
   Version 1.3    Last modified: August 1997
   Copyright (c) 1996, France Telecom, Rockwell International,
                       Universite de Sherbrooke.
   All rights reserved.
*/

/* Computation of Comfort Noise excitation             */

#include <stdio.h>
#include <stdlib.h>

#include "typedef.h"
#include "ld8a.h"
#include "dtx.h"
#include "basic_op.h"
#include "oper_32b.h"


/* Local functions */
static Word16 Gauss(Word16 *seed);
static Word16 Sqrt( Word32 Num);

/*-----------------------------------------------------------*
 * procedure Calc_exc_rand                                   *
 *           ~~~~~~~~~~~~~                                   *
 *   Computes comfort noise excitation                       *
 *   for SID and not-transmitted frames                      *
 *-----------------------------------------------------------*/
void Calc_exc_rand(
  Word16 cur_gain,      /* (i)   :   target sample gain                 */
  Word16 *exc,          /* (i/o) :   excitation array                   */
  Word16 *seed,         /* (i)   :   current Vad decision               */
  Word32 *L_exc_err
)
{
  Word16 i, j, i_subfr;
  Word16 temp1, temp2;
  Word16 pos[4];
  Word16 sign[4];
  Word16 t0, frac;
  Word16 *cur_exc;
  Word16 g, Gp, Gp2;
  Word16 excg[L_SUBFR], excs[L_SUBFR];
  Word32 L_acc, L_ener, L_k;
  Word16 max, hi, lo, inter_exc;
  Word16 sh;
  Word16 x1, x2;
  Flag   Overflow = 0;
  
  if(cur_gain == 0) {

    for(i=0; i<L_FRAME; i++) {
      exc[i] = 0;
    }
    Gp = 0;
    t0 = add(L_SUBFR,1, &Overflow);
    for (i_subfr = 0;  i_subfr < L_FRAME; i_subfr += L_SUBFR) {
      if(L_exc_err) update_exc_err(Gp, t0, L_exc_err);
    }

    return;
  }

  
  
  /* Loop on subframes */
  
  cur_exc = exc;
  
  for (i_subfr = 0;  i_subfr < L_FRAME; i_subfr += L_SUBFR) {

    /* generate random adaptive codebook & fixed codebook parameters */
    /*****************************************************************/
    temp1 = Random(seed);
    frac = sub((temp1 & (Word16)0x0003), 1, &Overflow);
    if(sub(frac, 2, &Overflow) == 0) frac = 0;
    temp1 = shr(temp1, 2, &Overflow);
    t0 = add((temp1 & (Word16)0x003F), 40, &Overflow);
    temp1 = shr(temp1, 6, &Overflow);
    temp2 = temp1 & (Word16)0x0007;
    pos[0] = add(shl(temp2, 2, &Overflow), temp2, &Overflow); /* 5 * temp2 */
    temp1 = shr(temp1, 3, &Overflow);
    sign[0] = temp1 & (Word16)0x0001;
    temp1 = shr(temp1, 1, &Overflow);
    temp2 = temp1 & (Word16)0x0007;
    temp2 = add(shl(temp2, 2, &Overflow), temp2, &Overflow);
    pos[1] = add(temp2, 1, &Overflow);     /* 5 * x + 1 */
    temp1 = shr(temp1, 3, &Overflow);
    sign[1] = temp1 & (Word16)0x0001;
    temp1 = Random(seed);
    temp2 = temp1 & (Word16)0x0007;
    temp2 = add(shl(temp2, 2, &Overflow), temp2, &Overflow);
    pos[2] = add(temp2, 2, &Overflow);     /* 5 * x + 2 */
    temp1 = shr(temp1, 3, &Overflow);
    sign[2] = temp1 & (Word16)0x0001;
    temp1 = shr(temp1, 1, &Overflow);
    temp2 = temp1 & (Word16)0x000F;
    pos[3] = add((temp2 & (Word16)1), 3, &Overflow); /* j+3*/
    temp2 = (shr(temp2, 1, &Overflow)) & (Word16)7;
    temp2 = add(shl(temp2, 2, &Overflow), temp2, &Overflow); /* 5i */
    pos[3] = add(pos[3], temp2, &Overflow);
    temp1 = shr(temp1, 4, &Overflow);
    sign[3] = temp1 & (Word16)0x0001;
    Gp = Random(seed) & (Word16)0x1FFF; /* < 0.5 Q14 */
    Gp2 = shl(Gp, 1, &Overflow);           /* Q15 */


    /* Generate gaussian excitation */
    /********************************/
    L_acc = 0L;
    for(i=0; i<L_SUBFR; i++) {
      temp1 = Gauss(seed);
      L_acc = L_mac(L_acc, temp1, temp1, &Overflow);
      excg[i] = temp1;
    }

/*
    Compute fact = alpha x cur_gain * sqrt(L_SUBFR / Eg)
    with Eg = SUM(i=0->39) excg[i]^2
    and alpha = 0.5
    alpha x sqrt(L_SUBFR)/2 = 1 + FRAC1
*/
    L_acc = Inv_sqrt(L_shr(L_acc,1, &Overflow));  /* Q30 */
    L_Extract(L_acc, &hi, &lo);
    /* cur_gain = cur_gainR << 3 */
    temp1 = mult_r(cur_gain, FRAC1);
    temp1 = add(cur_gain, temp1, &Overflow);
    /* <=> alpha x cur_gainR x 2^2 x sqrt(L_SUBFR) */

    L_acc = Mpy_32_16(hi, lo, temp1);   /* fact << 17 */
    sh = norm_l(L_acc);
    temp1 = extract_h(L_shl(L_acc, sh, &Overflow));  /* fact << (sh+1) */

    sh = sub(sh, 14, &Overflow);
    for(i=0; i<L_SUBFR; i++) {
      temp2 = mult_r(excg[i], temp1);
      temp2 = shr_r(temp2, sh, &Overflow);   /* shl if sh < 0 */
      excg[i] = temp2;
    }

    /* generate random  adaptive excitation */
    /****************************************/
    Pred_lt_3(cur_exc, t0, frac, L_SUBFR);


    /* compute adaptive + gaussian exc -> cur_exc */
    /**********************************************/
    max = 0;
    for(i=0; i<L_SUBFR; i++) {
      temp1 = mult_r(cur_exc[i], Gp2);
      temp1 = add(temp1, excg[i], &Overflow); /* may overflow */
      cur_exc[i] = temp1;
      temp1 = abs_s(temp1);
      if(sub(temp1,max, &Overflow) > 0) max = temp1;
    }

    /* rescale cur_exc -> excs */
    if(max == 0) sh = 0;
    else {
      sh = sub(3, norm_s(max), &Overflow);
      if(sh <= 0) sh = 0;
    }
    for(i=0; i<L_SUBFR; i++) {
      excs[i] = shr(cur_exc[i], sh, &Overflow);
    }

    /* Compute fixed code gain */
    /***************************/

    /**********************************************************/
    /*** Solve EQ(X) = 4 X**2 + 2 b X + c                     */
    /**********************************************************/

    L_ener = 0L;
    for(i=0; i<L_SUBFR; i++) {
      L_ener = L_mac(L_ener, excs[i], excs[i], &Overflow);
    } /* ener x 2^(-2sh + 1) */

    /* inter_exc = b >> sh */
    inter_exc = 0;
    for(i=0; i<4; i++) {
      j = pos[i];
      if(sign[i] == 0) {
        inter_exc = sub(inter_exc, excs[j], &Overflow);
      }
      else {
        inter_exc = add(inter_exc, excs[j], &Overflow);
      }
    }

    /* Compute k = cur_gainR x cur_gainR x L_SUBFR */
    L_acc = L_mult(cur_gain, L_SUBFR, &Overflow);
    L_acc = L_shr(L_acc, 6, &Overflow);
    temp1 = extract_l(L_acc);   /* cur_gainR x L_SUBFR x 2^(-2) */
    L_k   = L_mult(cur_gain, temp1, &Overflow); /* k << 2 */
    temp1 = add(1, shl(sh,1, &Overflow), &Overflow);
    L_acc = L_shr(L_k, temp1, &Overflow);  /* k x 2^(-2sh+1) */

    /* Compute delta = b^2 - 4 c */
    L_acc = L_sub(L_acc, L_ener, &Overflow); /* - 4 c x 2^(-2sh-1) */
    inter_exc = shr(inter_exc, 1, &Overflow);
    L_acc = L_mac(L_acc, inter_exc, inter_exc, &Overflow); /* 2^(-2sh-1) */
    sh = add(sh, 1, &Overflow);
    /* inter_exc = b x 2^(-sh) */
    /* L_acc = delta x 2^(-2sh+1) */

    if(L_acc < 0) {

      /* adaptive excitation = 0 */
      Copy(excg, cur_exc, L_SUBFR);
      temp1 = abs_s(excg[(int)pos[0]]) | abs_s(excg[(int)pos[1]]);
      temp2 = abs_s(excg[(int)pos[2]]) | abs_s(excg[(int)pos[3]]);
      temp1 = temp1 | temp2;
      sh = ((temp1 & (Word16)0x4000) == 0) ? (Word16)1 : (Word16)2;
      inter_exc = 0;
      for(i=0; i<4; i++) {
        temp1 = shr(excg[(int)pos[i]], sh, &Overflow);
        if(sign[i] == 0) {
          inter_exc = sub(inter_exc, temp1, &Overflow);
        }
        else {
          inter_exc = add(inter_exc, temp1, &Overflow);
        }
      } /* inter_exc = b >> sh */
      L_Extract(L_k, &hi, &lo);
      L_acc = Mpy_32_16(hi, lo, K0); /* k x (1- alpha^2) << 2 */
      temp1 = sub(shl(sh, 1, &Overflow), 1, &Overflow); /* temp1 > 0 */
      L_acc = L_shr(L_acc, temp1, &Overflow); /* 4k x (1 - alpha^2) << (-2sh+1) */
      L_acc = L_mac(L_acc, inter_exc, inter_exc, &Overflow); /* delta << (-2sh+1) */
      Gp = 0;
    }

    temp2 = Sqrt(L_acc);        /* >> sh */
    x1 = sub(temp2, inter_exc, &Overflow);
    x2 = negate(add(inter_exc, temp2, &Overflow)); /* x 2^(-sh+2) */
    if(sub(abs_s(x2),abs_s(x1), &Overflow) < 0) x1 = x2;
    temp1 = sub(2, sh, &Overflow);
    g = shr_r(x1, temp1, &Overflow);       /* shl if temp1 < 0 */
    if(g >= 0) {
      if(sub(g, G_MAX, &Overflow) > 0) g = G_MAX;
    }
    else {
      if(add(g, G_MAX, &Overflow) < 0) g = negate(G_MAX);
    }

    /* Update cur_exc with ACELP excitation */
    for(i=0; i<4; i++) {
      j = pos[i];
      if(sign[i] != 0) {
        cur_exc[j] = add(cur_exc[j], g, &Overflow);
      }
      else {
        cur_exc[j] = sub(cur_exc[j], g, &Overflow);
      }
    }

    if(L_exc_err) update_exc_err(Gp, t0, L_exc_err);

    cur_exc += L_SUBFR;


  } /* end of loop on subframes */
  
  return;
}

/*-----------------------------------------------------------*
 *         Local procedures                                  *
 *         ~~~~~~~~~~~~~~~~                                  *
 *-----------------------------------------------------------*/

/* Gaussian generation */
/***********************/
static Word16 Gauss(Word16 *seed)
{

/****  Xi = uniform v.a. in [-32768, 32767]       ****/
/****  Z = SUM(i=1->12) Xi / 2 x 32768 is N(0,1)  ****/
/****  output : Z x 512 < 2^12                    ****/

  Word16 i;
  Word16 temp;
  Word32 L_acc;
  Flag   Overflow = 0;
  
  L_acc = 0L;
  for(i=0; i<12; i++) {
    L_acc = L_add(L_acc, L_deposit_l(Random(seed)), &Overflow);
  }
  L_acc = L_shr(L_acc, 7, &Overflow);
  temp = extract_l(L_acc);
  return(temp);
}

/* Square root function : returns sqrt(Num/2) */
/**********************************************/
static Word16   Sqrt( Word32 Num )
{
  Word16   i  ;
  
  Word16   Rez = (Word16) 0 ;
  Word16   Exp = (Word16) 0x4000 ;
  
  Word32   Acc, L_temp;
  Flag     Overflow = 0;

  for ( i = 0 ; i < 14 ; i ++ ) {
    Acc = L_mult(add(Rez, Exp, &Overflow), add(Rez, Exp, &Overflow) , &Overflow);
    L_temp = L_sub(Num, Acc, &Overflow);
    if(L_temp >= 0L) Rez = add( Rez, Exp, &Overflow);
    Exp = shr( Exp, (Word16) 1 , &Overflow) ;
  }
  return Rez ;
}





 
    





