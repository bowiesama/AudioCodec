/*****************************************************************************/
/* Cintrins.h    header file for the C implementation of the    	         */
/*****************************************************************************/
#include "types.h"

#if ( __INTEL_COMPILER == 1000 )
#define restrict
#elif !defined(__INTEL_COMPILER)
#define restrict 
#endif

#ifdef __INTEL_COMPILER
#include <ia32intrin.h>
#include <stdlib.h>
#include <limits.h>
#define _sshl(a, c) ((a)<<(c))
#define _abs(a)          abs(a)                                             
#define _lmbd(a, b)      (((b) == (0)) ? (32) : (31 - _bit_scan_reverse(b)))
__forceinline int _norm(unsigned int x) 
{	
	return(_lmbd(0,abs(x + (x >> 31)))-1); 
}
__forceinline int _sadd(int src1, int src2)
{
	int result = src1 + src2;
	if( ((src1 ^ src2) & INT_MIN) == 0)
		if((result ^ src1) & INT_MIN)
			result = (src1<0) ? INT_MIN : INT_MAX;
	return(result);
}
__forceinline int _smpy(int src1,int src2)
{
	int result = ((short)src1 * (short)src2) << 1;
	if (result != INT_MIN)	
		return(result);
	else 
		return(INT_MAX);
}
#else
#define	_abs	 	abs_c
#define	_lmbd		lmbd_c
#define	_norm		norm_c
#define	_sshl		sshl_c
#define	_sadd		sadd_c
#define	_smpy		smpy_c
#endif

#define MIN_32 (Word32)0x80000000
#define MAX_32 (Word32)0x7fffffff
#define MIN_40 (Word40)0x8000000000
#define MAX_40 (Word40)0x7fffffffff



#if!defined(_TMS320C6200)

#define	_add2	 	add2_c
#define	_clr		clr_c
#define	_ext		ext_c
#define	_extu		extu_c
#define	_lnorm		lnorm_c
#define	_lsadd		lsadd_c
#define	_ssub		ssub_c
#define	_lssub		lssub_c
#define	_sat		sat_c
#define	_set		set_c
#define _sub2		sub2_c
#define _subc		subc_c
#define	_mpy		mpy_c
#define	_mpyus		mpyus_c
#define	_mpysu		mpysu_c
#define	_mpyu		mpyu_c
#define	_mpyh		mpyh_c
#define	_mpyhus		mpyhus_c
#define	_mpyhsu		mpyhsu_c
#define	_mpyhu		mpyhu_c
#define	_mpylh		mpylh_c
#define	_mpyluhs	mpyluhs_c
#define	_mpylshu	mpylshu_c
#define	_mpylhu		mpylhu_c
#define	_mpyhl		mpyhl_c
#define	_mpyhuls	mpyhuls_c
#define	_mpyhslu	mpyhslu_c
#define	_mpyhlu		mpyhlu_c
#define	_smpyh		smpyh_c
#define	_smpylh		smpylh_c
#define	_smpyhl		smpyhl_c

Word32 abs_c(Word32 src1);
Word32 add2_c(Word32 src1, Word32 src2);
UWord32 clr_c(UWord32 src1, UWord32 csta, UWord32 cstb);
Word32 ext_c(Word32 src1, UWord32 csta, UWord32 cstb);
UWord32 extu_c(UWord32 src1, UWord32 csta, UWord32 cstb);
UWord32 lmbd_c(UWord32 src1,UWord32 src2);
UWord32 lnorm_c(Word40 src2);
Word40 lsadd_c(Word32 src1, Word40 src2);

Word32 mpy_c(Word32 src1,Word32 src2);
Word32 mpyus_c(UWord32 src1,Word32 src2);
Word32 mpysu_c(Word32 src1,UWord32 src2);
Word32 mpyu_c(UWord32 src1,UWord32 src2);

Word32 mpyh_c(Word32 src1,Word32 src2);
Word32 mpyhus_c(UWord32 src1,Word32 src2);
Word32 mpyhsu_c(Word32 src1,UWord32 src2);
Word32 mpyhu_c(UWord32 src1,UWord32 src2);

Word32 mpyhl_c(Word32 src1,Word32 src2);
Word32 mpyhuls_c(UWord32 src1,Word32 src2);
Word32 mpyhslu_c(Word32 src1,UWord32 src2);
Word32 mpyhlu_c(UWord32 src1,UWord32 src2);

Word32 mpylh_c(Word32 src1,Word32 src2);
Word32 mpyluhs_c(UWord32 src1,Word32 src2);
Word32 mpylshu_c(Word32 src1,UWord32 src2);
Word32 mpylhu_c(UWord32 src1,UWord32 src2);

UWord32 norm_c(Word32 src2);
Word32 sadd_c(Word32 src1, Word32 src2);
Word32 sat_c(Word40 src2);
UWord32 set_c(UWord32 src1,UWord32 csta,UWord32 cstb);

Word32 smpy_c(Word32 src1,Word32 src2);
Word32 smpyh_c(Word32 src1,Word32 src2);
Word32 smpyhl_c(Word32 src1,Word32 src2);
Word32 smpylh_c(Word32 src1,Word32 src2);
Word32 sshl_c(Word32 src2, UWord32 src1);
Word32 ssub_c(Word32 src1, Word32 src2);
Word32 sub2_c(Word32 src1, Word32 src2);
UWord32 subc_c(UWord32 src1, UWord32 src2);

#endif


