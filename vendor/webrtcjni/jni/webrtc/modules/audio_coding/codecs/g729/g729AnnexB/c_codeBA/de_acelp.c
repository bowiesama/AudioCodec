/* ITU-T G.729 Software Package Release 2 (November 2006) */
/*
   ITU-T G.729A Speech Coder    ANSI-C Source Code
   Version 1.1    Last modified: September 1996

   Copyright (c) 1996,
   AT&T, France Telecom, NTT, Universite de Sherbrooke, Lucent Technologies
   All rights reserved.
*/

/*-----------------------------------------------------------*
 *  Function  Decod_ACELP()                                  *
 *  ~~~~~~~~~~~~~~~~~~~~~~~                                  *
 *   Algebraic codebook decoder.                             *
 *----------------------------------------------------------*/

#include "typedef.h"
#include "basic_op.h"
#include "ld8a.h"

void Decod_ACELP(
  Word16 sign,      /* (i)     : signs of 4 pulses.                       */
  Word16 index,     /* (i)     : Positions of the 4 pulses.               */
  Word16 cod[]      /* (o) Q13 : algebraic (fixed) codebook excitation    */
)
{
  Word16 i, j;
  Word16 pos[4];
  Flag   Overflow = 0;


  /* Decode the positions */

  i      = index & (Word16)7;
  pos[0] = add(i, shl(i, 2, &Overflow), &Overflow);           /* pos0 =i*5 */

  index  = shr(index, 3, &Overflow);
  i      = index & (Word16)7;
  i      = add(i, shl(i, 2, &Overflow), &Overflow);           /* pos1 =i*5+1 */
  pos[1] = add(i, 1, &Overflow);

  index  = shr(index, 3, &Overflow);
  i      = index & (Word16)7;
  i      = add(i, shl(i, 2, &Overflow), &Overflow);           /* pos2 =i*5+1 */
  pos[2] = add(i, 2, &Overflow);

  index  = shr(index, 3, &Overflow);
  j      = index & (Word16)1;
  index  = shr(index, 1, &Overflow);
  i      = index & (Word16)7;
  i      = add(i, shl(i, 2, &Overflow), &Overflow);           /* pos3 =i*5+3+j */
  i      = add(i, 3, &Overflow);
  pos[3] = add(i, j, &Overflow);

  /* decode the signs  and build the codeword */

  for (i=0; i<L_SUBFR; i++) {
    cod[i] = 0;
  }

  for (j=0; j<4; j++)
  {

    i = sign & (Word16)1;
    sign = shr(sign, 1, &Overflow);

    if (i != 0) {
      cod[pos[j]] = 8191;      /* Q13 +1.0 */
    }
    else {
      cod[pos[j]] = -8192;     /* Q13 -1.0 */
    }
  }

  return;
}
