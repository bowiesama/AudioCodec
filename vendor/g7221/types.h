
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if 0
//defined(__unix__)
#include "Long40.h"
typedef Long40     	Word40;
typedef int		Word32;
typedef unsigned int	UWord32;
typedef short		Word16;
typedef unsigned short 	UWord16;
#include <assert.h>

#elif defined(_TMS320C6200)
#define assert(a)
typedef long     	Word40;
typedef unsigned long 	UWord40;
typedef int		Word32;
typedef unsigned int	UWord32;
typedef short		Word16;
typedef unsigned short 	UWord16;

#elif defined(HAVE_IOS) && defined(__aarch64__)
typedef long     			Word40;
typedef int					Word32;
typedef unsigned int		UWord32;
typedef short int	 		Word16;
typedef unsigned short int 	UWord16;
#include <assert.h>

#else
/*#include "Long40.h" */
typedef long     		Word40;
typedef int		Word32;
typedef unsigned long int	UWord32;
typedef short int 		Word16;
typedef unsigned short int  	UWord16;
#include <assert.h>
#endif

