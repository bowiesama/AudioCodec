#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "FFT_float.h"
#include "aecm_core.h"


static float twiddleFactor768[768] = 
{
	0.0000000f,  0.0081811f,  0.0163617f,  0.0245412f,  0.0327191f,  0.0408947f,  0.0490677f,  0.0572373f,  0.0654031f,  0.0735646f,  0.0817211f,  0.0898721f,  
	0.0980171f,  0.1061556f,  0.1142870f,  0.1224107f,  0.1305262f,  0.1386330f,  0.1467305f,  0.1548182f,  0.1628955f,  0.1709619f,  0.1790169f,  0.1870599f,  
	0.1950903f,  0.2031077f,  0.2111116f,  0.2191012f,  0.2270763f,  0.2350361f,  0.2429802f,  0.2509080f,  0.2588190f,  0.2667128f,  0.2745886f,  0.2824461f,  
	0.2902847f,  0.2981038f,  0.3059030f,  0.3136817f,  0.3214395f,  0.3291757f,  0.3368899f,  0.3445815f,  0.3522500f,  0.3598950f,  0.3675159f,  0.3751122f,  
	0.3826834f,  0.3902290f,  0.3977485f,  0.4052413f,  0.4127070f,  0.4201451f,  0.4275551f,  0.4349364f,  0.4422887f,  0.4496113f,  0.4569039f,  0.4641658f,  
	0.4713967f,  0.4785961f,  0.4857634f,  0.4928982f,  0.5000000f,  0.5070683f,  0.5141027f,  0.5211027f,  0.5280679f,  0.5349976f,  0.5418916f,  0.5487493f,  
	0.5555702f,  0.5623540f,  0.5691001f,  0.5758082f,  0.5824777f,  0.5891082f,  0.5956993f,  0.6022505f,  0.6087614f,  0.6152316f,  0.6216606f,  0.6280479f,  
	0.6343933f,  0.6406962f,  0.6469562f,  0.6531728f,  0.6593458f,  0.6654747f,  0.6715590f,  0.6775983f,  0.6835923f,  0.6895405f,  0.6954426f,  0.7012982f,  
	0.7071068f,  0.7128681f,  0.7185816f,  0.7242471f,  0.7298641f,  0.7354322f,  0.7409511f,  0.7464204f,  0.7518398f,  0.7572088f,  0.7625272f,  0.7677945f,  
	0.7730105f,  0.7781746f,  0.7832867f,  0.7883464f,  0.7933533f,  0.7983072f,  0.8032075f,  0.8080542f,  0.8128467f,  0.8175848f,  0.8222682f,  0.8268966f,  
	0.8314696f,  0.8359870f,  0.8404484f,  0.8448536f,  0.8492022f,  0.8534940f,  0.8577286f,  0.8619059f,  0.8660254f,  0.8700870f,  0.8740903f,  0.8780352f,  
	0.8819213f,  0.8857483f,  0.8895161f,  0.8932243f,  0.8968727f,  0.9004612f,  0.9039893f,  0.9074569f,  0.9108638f,  0.9142098f,  0.9174945f,  0.9207178f,  
	0.9238795f,  0.9269794f,  0.9300172f,  0.9329928f,  0.9359059f,  0.9387564f,  0.9415441f,  0.9442687f,  0.9469301f,  0.9495282f,  0.9520627f,  0.9545335f,  
	0.9569403f,  0.9592832f,  0.9615618f,  0.9637761f,  0.9659258f,  0.9680109f,  0.9700313f,  0.9719866f,  0.9738770f,  0.9757021f,  0.9774620f,  0.9791564f,  
	0.9807853f,  0.9823485f,  0.9838460f,  0.9852776f,  0.9866433f,  0.9879430f,  0.9891765f,  0.9903438f,  0.9914449f,  0.9924795f,  0.9934478f,  0.9943495f,  
	0.9951847f,  0.9959533f,  0.9966552f,  0.9972905f,  0.9978589f,  0.9983606f,  0.9987955f,  0.9991635f,  0.9994646f,  0.9996988f,  0.9998661f,  0.9999665f,  
	1.0000000f,  0.9999665f,  0.9998661f,  0.9996988f,  0.9994646f,  0.9991635f,  0.9987955f,  0.9983606f,  0.9978589f,  0.9972905f,  0.9966552f,  0.9959533f,  
	0.9951847f,  0.9943495f,  0.9934478f,  0.9924795f,  0.9914449f,  0.9903438f,  0.9891765f,  0.9879430f,  0.9866433f,  0.9852776f,  0.9838460f,  0.9823485f,  
	0.9807853f,  0.9791564f,  0.9774620f,  0.9757021f,  0.9738770f,  0.9719866f,  0.9700313f,  0.9680109f,  0.9659258f,  0.9637761f,  0.9615618f,  0.9592832f,  
	0.9569403f,  0.9545335f,  0.9520627f,  0.9495282f,  0.9469301f,  0.9442687f,  0.9415441f,  0.9387564f,  0.9359059f,  0.9329928f,  0.9300172f,  0.9269794f,  
	0.9238795f,  0.9207178f,  0.9174945f,  0.9142098f,  0.9108638f,  0.9074569f,  0.9039893f,  0.9004612f,  0.8968727f,  0.8932243f,  0.8895161f,  0.8857483f,  
	0.8819213f,  0.8780352f,  0.8740903f,  0.8700870f,  0.8660254f,  0.8619059f,  0.8577286f,  0.8534940f,  0.8492022f,  0.8448536f,  0.8404484f,  0.8359870f,  
	0.8314696f,  0.8268966f,  0.8222682f,  0.8175848f,  0.8128467f,  0.8080542f,  0.8032075f,  0.7983072f,  0.7933533f,  0.7883464f,  0.7832867f,  0.7781746f,  
	0.7730105f,  0.7677945f,  0.7625272f,  0.7572088f,  0.7518398f,  0.7464204f,  0.7409511f,  0.7354322f,  0.7298641f,  0.7242471f,  0.7185816f,  0.7128681f,  
	0.7071068f,  0.7012982f,  0.6954426f,  0.6895405f,  0.6835923f,  0.6775983f,  0.6715590f,  0.6654747f,  0.6593458f,  0.6531728f,  0.6469562f,  0.6406962f,  
	0.6343933f,  0.6280479f,  0.6216606f,  0.6152316f,  0.6087614f,  0.6022505f,  0.5956993f,  0.5891082f,  0.5824777f,  0.5758082f,  0.5691001f,  0.5623540f,  
	0.5555702f,  0.5487493f,  0.5418916f,  0.5349976f,  0.5280679f,  0.5211027f,  0.5141027f,  0.5070683f,  0.5000000f,  0.4928982f,  0.4857634f,  0.4785961f,  
	0.4713967f,  0.4641658f,  0.4569039f,  0.4496113f,  0.4422887f,  0.4349364f,  0.4275551f,  0.4201451f,  0.4127070f,  0.4052413f,  0.3977485f,  0.3902290f,  
	0.3826834f,  0.3751122f,  0.3675159f,  0.3598950f,  0.3522500f,  0.3445815f,  0.3368899f,  0.3291757f,  0.3214395f,  0.3136817f,  0.3059030f,  0.2981038f,  
	0.2902847f,  0.2824461f,  0.2745886f,  0.2667128f,  0.2588190f,  0.2509080f,  0.2429802f,  0.2350361f,  0.2270763f,  0.2191012f,  0.2111116f,  0.2031077f,  
	0.1950903f,  0.1870599f,  0.1790169f,  0.1709619f,  0.1628955f,  0.1548182f,  0.1467305f,  0.1386330f,  0.1305262f,  0.1224107f,  0.1142870f,  0.1061556f,  
	0.0980171f,  0.0898721f,  0.0817211f,  0.0735646f,  0.0654031f,  0.0572373f,  0.0490677f,  0.0408947f,  0.0327191f,  0.0245412f,  0.0163617f,  0.0081811f, 
	-0.0000000f, -0.0081811f, -0.0163617f, -0.0245412f, -0.0327191f, -0.0408947f, -0.0490677f, -0.0572373f, -0.0654031f, -0.0735646f, -0.0817211f, -0.0898721f,
	-0.0980171f, -0.1061556f, -0.1142870f, -0.1224107f, -0.1305262f, -0.1386330f, -0.1467305f, -0.1548182f, -0.1628955f, -0.1709619f, -0.1790169f, -0.1870599f, 
	-0.1950903f, -0.2031077f, -0.2111116f, -0.2191012f, -0.2270763f, -0.2350361f, -0.2429802f, -0.2509080f, -0.2588190f, -0.2667128f, -0.2745886f, -0.2824461f, 
	-0.2902847f, -0.2981038f, -0.3059030f, -0.3136817f, -0.3214395f, -0.3291757f, -0.3368899f, -0.3445815f, -0.3522500f, -0.3598950f, -0.3675159f, -0.3751122f,
	-0.3826834f, -0.3902290f, -0.3977485f, -0.4052413f, -0.4127070f, -0.4201451f, -0.4275551f, -0.4349364f, -0.4422887f, -0.4496113f, -0.4569039f, -0.4641658f,
	-0.4713967f, -0.4785961f, -0.4857634f, -0.4928982f, -0.5000000f, -0.5070683f, -0.5141027f, -0.5211027f, -0.5280679f, -0.5349976f, -0.5418916f, -0.5487493f, 
	-0.5555702f, -0.5623540f, -0.5691001f, -0.5758082f, -0.5824777f, -0.5891082f, -0.5956993f, -0.6022505f, -0.6087614f, -0.6152316f, -0.6216606f, -0.6280479f, 
	-0.6343933f, -0.6406962f, -0.6469562f, -0.6531728f, -0.6593458f, -0.6654747f, -0.6715590f, -0.6775983f, -0.6835923f, -0.6895405f, -0.6954426f, -0.7012982f,
	-0.7071068f, -0.7128681f, -0.7185816f, -0.7242471f, -0.7298641f, -0.7354322f, -0.7409511f, -0.7464204f, -0.7518398f, -0.7572088f, -0.7625272f, -0.7677945f, 
	-0.7730105f, -0.7781746f, -0.7832867f, -0.7883464f, -0.7933533f, -0.7983072f, -0.8032075f, -0.8080542f, -0.8128467f, -0.8175848f, -0.8222682f, -0.8268966f, 
	-0.8314696f, -0.8359870f, -0.8404484f, -0.8448536f, -0.8492022f, -0.8534940f, -0.8577286f, -0.8619059f, -0.8660254f, -0.8700870f, -0.8740903f, -0.8780352f, 
	-0.8819213f, -0.8857483f, -0.8895161f, -0.8932243f, -0.8968727f, -0.9004612f, -0.9039893f, -0.9074569f, -0.9108638f, -0.9142098f, -0.9174945f, -0.9207178f, 
	-0.9238795f, -0.9269794f, -0.9300172f, -0.9329928f, -0.9359059f, -0.9387564f, -0.9415441f, -0.9442687f, -0.9469301f, -0.9495282f, -0.9520627f, -0.9545335f,
	-0.9569403f, -0.9592832f, -0.9615618f, -0.9637761f, -0.9659258f, -0.9680109f, -0.9700313f, -0.9719866f, -0.9738770f, -0.9757021f, -0.9774620f, -0.9791564f,
	-0.9807853f, -0.9823485f, -0.9838460f, -0.9852776f, -0.9866433f, -0.9879430f, -0.9891765f, -0.9903438f, -0.9914449f, -0.9924795f, -0.9934478f, -0.9943495f, 
	-0.9951847f, -0.9959533f, -0.9966552f, -0.9972905f, -0.9978589f, -0.9983606f, -0.9987955f, -0.9991635f, -0.9994646f, -0.9996988f, -0.9998661f, -0.9999665f, 
	-1.0000000f, -0.9999665f, -0.9998661f, -0.9996988f, -0.9994646f, -0.9991635f, -0.9987955f, -0.9983606f, -0.9978589f, -0.9972905f, -0.9966552f, -0.9959533f, 
	-0.9951847f, -0.9943495f, -0.9934478f, -0.9924795f, -0.9914449f, -0.9903438f, -0.9891765f, -0.9879430f, -0.9866433f, -0.9852776f, -0.9838460f, -0.9823485f, 
	-0.9807853f, -0.9791564f, -0.9774620f, -0.9757021f, -0.9738770f, -0.9719866f, -0.9700313f, -0.9680109f, -0.9659258f, -0.9637761f, -0.9615618f, -0.9592832f, 
	-0.9569403f, -0.9545335f, -0.9520627f, -0.9495282f, -0.9469301f, -0.9442687f, -0.9415441f, -0.9387564f, -0.9359059f, -0.9329928f, -0.9300172f, -0.9269794f, 
	-0.9238795f, -0.9207178f, -0.9174945f, -0.9142098f, -0.9108638f, -0.9074569f, -0.9039893f, -0.9004612f, -0.8968727f, -0.8932243f, -0.8895161f, -0.8857483f, 
	-0.8819213f, -0.8780352f, -0.8740903f, -0.8700870f, -0.8660254f, -0.8619059f, -0.8577286f, -0.8534940f, -0.8492022f, -0.8448536f, -0.8404484f, -0.8359870f, 
	-0.8314696f, -0.8268966f, -0.8222682f, -0.8175848f, -0.8128467f, -0.8080542f, -0.8032075f, -0.7983072f, -0.7933533f, -0.7883464f, -0.7832867f, -0.7781746f, 
	-0.7730105f, -0.7677945f, -0.7625272f, -0.7572088f, -0.7518398f, -0.7464204f, -0.7409511f, -0.7354322f, -0.7298641f, -0.7242471f, -0.7185816f, -0.7128681f, 
	-0.7071068f, -0.7012982f, -0.6954426f, -0.6895405f, -0.6835923f, -0.6775983f, -0.6715590f, -0.6654747f, -0.6593458f, -0.6531728f, -0.6469562f, -0.6406962f,
	-0.6343933f, -0.6280479f, -0.6216606f, -0.6152316f, -0.6087614f, -0.6022505f, -0.5956993f, -0.5891082f, -0.5824777f, -0.5758082f, -0.5691001f, -0.5623540f, 
	-0.5555702f, -0.5487493f, -0.5418916f, -0.5349976f, -0.5280679f, -0.5211027f, -0.5141027f, -0.5070683f, -0.5000000f, -0.4928982f, -0.4857634f, -0.4785961f,
	-0.4713967f, -0.4641658f, -0.4569039f, -0.4496113f, -0.4422887f, -0.4349364f, -0.4275551f, -0.4201451f, -0.4127070f, -0.4052413f, -0.3977485f, -0.3902290f,
	-0.3826834f, -0.3751122f, -0.3675159f, -0.3598950f, -0.3522500f, -0.3445815f, -0.3368899f, -0.3291757f, -0.3214395f, -0.3136817f, -0.3059030f, -0.2981038f, 
	-0.2902847f, -0.2824461f, -0.2745886f, -0.2667128f, -0.2588190f, -0.2509080f, -0.2429802f, -0.2350361f, -0.2270763f, -0.2191012f, -0.2111116f, -0.2031077f,
	-0.1950903f, -0.1870599f, -0.1790169f, -0.1709619f, -0.1628955f, -0.1548182f, -0.1467305f, -0.1386330f, -0.1305262f, -0.1224107f, -0.1142870f, -0.1061556f, 
	-0.0980171f, -0.0898721f, -0.0817211f, -0.0735646f, -0.0654031f, -0.0572373f, -0.0490677f, -0.0408947f, -0.0327191f, -0.0245412f, -0.0163617f, -0.0081811f,
};

/* reorder table */
static unsigned char reverseSplit128[128] = 
{
	0,64,32,96,16,80,48,112,8,72,40,104,24,88,56,120,4,68,36,100,20,84,52,116,12,76,
	44,108,28,92,60,124,2,66,34,98,18,82,50,114,10,74,42,106,26,90,58,122,6,70,38,102,
	22,86,54,118,14,78,46,110,30,94,62,126,1,65,33,97,17,81,49,113,9,73,41,105,25,89,57,
	121,5,69,37,101,21,85,53,117,13,77,45,109,29,93,61,125,3,67,35,99,19,83,51,115,11,
	75,43,107,27,91,59,123,7,71,39,103,23,87,55,119,15,79,47,111,31,95,63,127
};
/* fft in split-radix for len = 2^n */
int splitRadixFFT(float *real,float *imag,int len)
{
	int	n,i,j,step=1,pos1,pos2;
	int *isThisLevel,level,levelNum;
	int LStart,LFourthStart,LHalfStart,LThreeFourthStart,LEnd;
	int LSize=len<<1,LHalfSize=len,LForthSize=len>>1;
	float *temr,*temi;
	float temr1,temr2,temr3,temr4,temr5,temr6,temi1,temi2,temi3,temi4,temi5,temi6;
	float temrS,temiS,temrF,temiF,temrH,temiH,temrT,temiT;
	float *twiddleFactor,e,nk;

	if(NULL==real||NULL==imag)
		return 1;

	levelNum = 1;
	for(i=1;levelNum<32;levelNum++)
	{
		i <<= 1;
		if(i==len) break;       /* len=2^levelNum */
		if(i>len) 
			return 1;
	}

	twiddleFactor = (float*)malloc(3*len*sizeof(float));	
	if(NULL==twiddleFactor)
		return 1;
	memset(twiddleFactor,0,3*len*sizeof(float));
	temr = twiddleFactor+len;
	temi = temr+len;




	isThisLevel = (int*)malloc(len*sizeof(int));	
	if(NULL==isThisLevel)
	{
		free(twiddleFactor);
		twiddleFactor = NULL;
		return 1;
	}
	memset(isThisLevel,0,len*sizeof(int));

	/* calculate twiddle factors */
	e = (float)6.28318530718/len;
	for(n=0;n<len;n++)
	{
		nk = e * n;
		twiddleFactor[n] = (float)sin(nk); 
	}



	for(n=0;n<len;n++)
	{
		temr[n] = real[n];
		temi[n] = imag[n];
	}

	isThisLevel[0] = 1;
	for(level=1;level<levelNum;level++)
	{ 
		LSize >>= 1;
		LHalfSize = LSize >> 1;
		LForthSize = LHalfSize >> 1;
		LEnd = LSize;
		for(LStart=0;LStart<len;LStart=LEnd)
		{
			LEnd = LStart + LSize;
			if(isThisLevel[LStart]==level)
			{
				pos1 = 0;
				LFourthStart = LStart + LForthSize; 
				LHalfStart = LStart + LHalfSize;
				LThreeFourthStart = LHalfStart + LForthSize;

				/* update level value */
				isThisLevel[LStart] = level + 1;
				isThisLevel[LThreeFourthStart] = isThisLevel[LHalfStart] = level + 2;

				/* calculate L */
				for(n=0;n<LForthSize;n++)
				{				
					if(level!=levelNum)
					{
						temrS = temr[LStart];
						temiS = temi[LStart];
						temrF = temr[LFourthStart];
						temiF = temi[LFourthStart];
						temrH = temr[LHalfStart];
						temiH = temi[LHalfStart];
						temrT = temr[LThreeFourthStart];
						temiT = temi[LThreeFourthStart];


						temr1 = temrS + temrH;
						temi1 = temiS + temiH;
						temr2 = temrF + temrT;
						temi2 = temiF + temiT;
						temr3 = temrS  - temrH;
						temi3 = temiF - temiT;
						temr4 = -temrF + temrT;
						temi4 = temiS - temiH;

						temr5 = temr3 + temi3;
						temi5 = temr4 + temi4;
						temr6 = temr3 - temi3;
						temi6 = temi4 - temr4;

						/* multiply the twiddle factor */
						pos2 = pos1*3;
						temr[LHalfStart] = temr5*twiddleFactor[pos1+len/4] + temi5*twiddleFactor[pos1];
						temi[LHalfStart] = temi5*twiddleFactor[pos1+len/4] - temr5*twiddleFactor[pos1];
						temr[LThreeFourthStart] = temr6*twiddleFactor[pos2+len/4] + temi6*twiddleFactor[pos2];
						temi[LThreeFourthStart] = temi6*twiddleFactor[pos2+len/4] - temr6*twiddleFactor[pos2];

						pos1 += step;

					}else{

						temrS = temr[LStart];
						temiS = temi[LStart];
						temrF = temr[LFourthStart];
						temiF = temi[LFourthStart];
						temrH = temr[LHalfStart];
						temiH = temi[LHalfStart];
						temrT = temr[LThreeFourthStart];
						temiT = temi[LThreeFourthStart];

						temr1 = temrS + temrH;
						temi1 = temiS + temiH;
						temr2 = temrF + temrT;
						temi2 = temiF + temiT;

						temr3 = temrS - temrH;
						temi3 = temiF - temiT;
						temr4 = - temrF + temrT;
						temi4 = temiS - temiH;

						temr[LHalfStart] = temr3 + temi3;
						temi[LHalfStart] = temr4 + temi4;
						temr[LThreeFourthStart] =  temr3 - temi3;
						temi[LThreeFourthStart] =  temi4 - temr4;
					}

					temr[LStart] = temr1;
					temi[LStart] = temi1;
					temr[LFourthStart] = temr2;
					temi[LFourthStart] = temi2;

					LStart++;
					LFourthStart++;
					LHalfStart++;
					LThreeFourthStart++;
				}
			}
		}
		step <<= 1;
	}

	for(LStart=0;LStart<len;LStart+=2)
	{
		//calculate 2 points DFT
		if(isThisLevel[LStart]==levelNum)
		{
			temr1 = temr[LStart] + temr[LStart+1];
			temi1 = temi[LStart] + temi[LStart+1];
			temr[LStart+1] = temr[LStart] - temr[LStart+1];
			temi[LStart+1] = temi[LStart] - temi[LStart+1];
			temr[LStart] = temr1;
			temi[LStart] = temi1;
		}
	}

	/* reorder */
	for(i=0,j=0;i<len-1;i++)
	{
		if(i<=j)
		{
			real[i] = temr[j];
			imag[i] = temi[j];
			real[j] = temr[i];
			imag[j] = temi[i];
		}
		n = len >> 1;
		while(n<j+1)
		{
			j = j - n;
			n = n >> 1;
		}
		j = j + n;
	}
	real[len-1] = temr[len-1];
	imag[len-1] = temi[len-1];

	free(twiddleFactor);
	twiddleFactor = NULL;

	free(isThisLevel);
	isThisLevel = NULL;

	return 0;
}

/* fft for len = 2^n */
/* do fft in radix-4 if len = 4^n */
/* do fft in split radix if len is not 4^n */
int FFT2(float *real,float *imag,int len)
{
	int i,level;

	if((NULL==real)||(NULL==imag))
		return 1;


	if(splitRadixFFT(real,imag,len)!=0)
		return 1;


	return 0;
}
//out_put is real. imag will be modified.
int InvFFT2(float *real,float *imag,int len)
{
	int i;
	float tmp;
	float *imfp=imag;
	if ((real==NULL) || (imag==NULL)|| (len<=0))
	{
		return 1;
	}
	for (i=0;i<len;i++)
	{
		*imfp*=-1.f;
		imfp++;
	}
	FFT2(real,imag,len);
	imfp=real;
	tmp=1.f/(float)len;
	for (i=0;i<len;i++)
	{
		*imfp*=tmp;
		imfp++;
	}
	return 0;
}

/* 128 points split-radix FFT */
int splitFFT128(float *real,float *imag)
{
	int	n,i,step=3,pos1,pos2;
	int isThisLevel[128]={0},level;
	int LStart,LFourthStart,LHalfStart,LThreeFourthStart,LEnd;
	int LSize=256,LHalfSize=128,LForthSize=64;
	float temr[128],temi[128];
	float temr1,temr2,temr3,temr4,temr5,temr6,temi1,temi2,temi3,temi4,temi5,temi6;
	float temrS,temiS,temrF,temiF,temrH,temiH,temrT,temiT;

	if(NULL==real||NULL==imag)
		return 1;

	for(n=0;n<128;n++)
	{
		temr[n] = real[n];
		temi[n] = imag[n];
	}

	isThisLevel[0] = 1;
	for(level=1;level<7;level++)
	{
		step <<= 1; 
		LSize = LSize >> 1;
		LHalfSize = LSize >> 1;
		LForthSize = LHalfSize >> 1;
		LEnd = LSize;
		for(LStart=0;LStart<128;LStart=LEnd)
		{
			LEnd = LStart + LSize;
			if(isThisLevel[LStart]==level)
			{
				pos1 = 0;
				LFourthStart = LStart + LForthSize; 
				LHalfStart = LStart + LHalfSize;
				LThreeFourthStart = LHalfStart + LForthSize;

				/* update level value */
				isThisLevel[LStart] = level + 1;
				isThisLevel[LThreeFourthStart] = isThisLevel[LHalfStart] = level + 2;

				/* calculate L */
				for(n=0;n<LForthSize;n++)
				{				
					if(level!=6)
					{
						temrS = temr[LStart];
						temiS = temi[LStart];
						temrF = temr[LFourthStart];
						temiF = temi[LFourthStart];
						temrH = temr[LHalfStart];
						temiH = temi[LHalfStart];
						temrT = temr[LThreeFourthStart];
						temiT = temi[LThreeFourthStart];


						temr1 = temrS + temrH;
						temi1 = temiS + temiH;
						temr2 = temrF + temrT;
						temi2 = temiF + temiT;
						temr3 = temrS  - temrH;
						temi3 = temiF - temiT;
						temr4 = -temrF + temrT;
						temi4 = temiS - temiH;

						temr5 = temr3 + temi3;
						temi5 = temr4 + temi4;
						temr6 = temr3 - temi3;
						temi6 = temi4 - temr4;

						/* multiply the twiddle factor */
						pos2 = pos1*3;
						temr[LHalfStart] = temr5*twiddleFactor768[pos1+192] + temi5*twiddleFactor768[pos1];
						temi[LHalfStart] = temi5*twiddleFactor768[pos1+192] - temr5*twiddleFactor768[pos1];
						temr[LThreeFourthStart] = temr6*twiddleFactor768[pos2+192] + temi6*twiddleFactor768[pos2];
						temi[LThreeFourthStart] = temi6*twiddleFactor768[pos2+192] - temr6*twiddleFactor768[pos2];

						pos1 += step;

					}else{

						temrS = temr[LStart];
						temiS = temi[LStart];
						temrF = temr[LFourthStart];
						temiF = temi[LFourthStart];
						temrH = temr[LHalfStart];
						temiH = temi[LHalfStart];
						temrT = temr[LThreeFourthStart];
						temiT = temi[LThreeFourthStart];

						temr1 = temrS + temrH;
						temi1 = temiS + temiH;
						temr2 = temrF + temrT;
						temi2 = temiF + temiT;

						temr3 = temrS - temrH;
						temi3 = temiF - temiT;
						temr4 = - temrF + temrT;
						temi4 = temiS - temiH;

						temr[LHalfStart] = temr3 + temi3;
						temi[LHalfStart] = temr4 + temi4;
						temr[LThreeFourthStart] =  temr3 - temi3;
						temi[LThreeFourthStart] =  temi4 - temr4;
					}

					temr[LStart] = temr1;
					temi[LStart] = temi1;
					temr[LFourthStart] = temr2;
					temi[LFourthStart] = temi2;

					LStart++;
					LFourthStart++;
					LHalfStart++;
					LThreeFourthStart++;
				}
			}
		}
	}

	for(LStart=0;LStart<128;LStart+=2)
	{
		//calculate 2 points DFT
		if(isThisLevel[LStart]==7)
		{
			temr1 = temr[LStart] + temr[LStart+1];
			temi1 = temi[LStart] + temi[LStart+1];
			temr[LStart+1] = temr[LStart] - temr[LStart+1];
			temi[LStart+1] = temi[LStart] - temi[LStart+1];
			temr[LStart] = temr1;
			temi[LStart] = temi1;
		}
	}

	/* reorder */
	for(i=0;i<128;i++)
	{
		real[i] = temr[reverseSplit128[i]];
		imag[i] = temi[reverseSplit128[i]];
	}
	return 0;
}

static const short WebRtcAecm_kSqrtHanning[] = {
	0, 399, 798, 1196, 1594, 1990, 2386, 2780, 3172,
	3562, 3951, 4337, 4720, 5101, 5478, 5853, 6224,
	6591, 6954, 7313, 7668, 8019, 8364, 8705, 9040,
	9370, 9695, 10013, 10326, 10633, 10933, 11227, 11514,
	11795, 12068, 12335, 12594, 12845, 13089, 13325, 13553,
	13773, 13985, 14189, 14384, 14571, 14749, 14918, 15079,
	15231, 15373, 15506, 15631, 15746, 15851, 15947, 16034,
	16111, 16179, 16237, 16286, 16325, 16354, 16373, 16384
};


static void ComplexFFT_float(const short* time_signal,
	complex_float* freq_signal)
{
	int i, j;

	float fft_real[PART_LEN2] = {0.0};
	float fft_image[PART_LEN2] = {0.0};
	// FFT of signal
	for (i = 0; i < PART_LEN; i++)
	{
		fft_real[i] = (float) time_signal[i] *	WebRtcAecm_kSqrtHanning[i] / 16384.0;
		fft_real[PART_LEN + i] = (float) time_signal[PART_LEN + i] *	WebRtcAecm_kSqrtHanning[PART_LEN - i] / 16384.0;
	}

	// Do forward FFT, then take only the first PART_LEN complex samples,
	// and change signs of the imaginary parts.
	splitFFT128(fft_real,fft_image);

	for (i = 0; i < PART_LEN; i++) {
		freq_signal[i].real = fft_real[i] / PART_LEN2;
		freq_signal[i].imag = -fft_image[i] / PART_LEN2;
	}

	for (i = PART_LEN; i < PART_LEN2; i++) {
		freq_signal[i].real = fft_real[i] / PART_LEN2;
		freq_signal[i].imag = fft_image[i] / PART_LEN2;
	}


}

void InverseFFT_float( AecmCore_float* aecm,
	complex_float* efw,
	short* output,
	const short* nearendClean)
{
	int i, j, outCFFT;
	float ifft_real[PART_LEN2] = {0.0};
	float ifft_image[PART_LEN2] = {0.0};

	// Synthesis
	for (i = 1; i < PART_LEN; i++)
	{
		ifft_real[i] = efw[i].real;
		ifft_real[PART_LEN2-i]=efw[i].real;
		ifft_image[i] = -efw[i].imag;
		ifft_image[PART_LEN2-i] = efw[i].imag;

	}

	ifft_real[0] = efw[0].real;
	ifft_real[PART_LEN]=efw[PART_LEN].real;
	ifft_image[0] = -efw[0].imag;
	ifft_image[PART_LEN]= -efw[PART_LEN].imag;

	InvFFT2(ifft_real,ifft_image,128);

	for (i = 0; i < PART_LEN; i++) {
		efw[i].real = 	(ifft_real[i] * PART_LEN2)*	WebRtcAecm_kSqrtHanning[i]/16384.0 + aecm->outBuf[i];

		output[i] = (short) (efw[i].real +0.5);

		aecm->outBuf[i] = (ifft_real[PART_LEN + i] * PART_LEN2)*	WebRtcAecm_kSqrtHanning[PART_LEN - i]/16384.0;
	}

	// Copy the current block to the old position (aecm->outBuf is shifted elsewhere)
	memcpy(aecm->xBuf, aecm->xBuf + PART_LEN, sizeof(float) * PART_LEN);
	memcpy(aecm->dBufNoisy, aecm->dBufNoisy + PART_LEN, sizeof(float) * PART_LEN);
	if (nearendClean != NULL)
	{
		memcpy(aecm->dBufClean, aecm->dBufClean + PART_LEN, sizeof(float) * PART_LEN);
	}
}


// Transforms a time domain signal into the frequency domain, outputting the
// complex valued signal, absolute value and sum of absolute values.
//
// time_signal          [in]    Pointer to time domain signal
// freq_signal_real     [out]   Pointer to real part of frequency domain array
// freq_signal_imag     [out]   Pointer to imaginary part of frequency domain
//                              array
// freq_signal_abs      [out]   Pointer to absolute value of frequency domain
//                              array
// freq_signal_sum_abs  [out]   Pointer to the sum of all absolute values in
//                              the frequency domain array
// return value                 The Q-domain of current frequency values
//
int TimeToFrequencyDomainFloat(AecmCore_float *aecm, const short* time_signal,
	complex_float* freq_signal,
	float* freq_signal_abs,
	float* freq_signal_sum_abs)
{
	int i = 0;
	float sum = 0.0;
	(*freq_signal_sum_abs)=0.0;

	//	WebRtcAecm_WindowAndFFT(aecm, fft, time_signal, freq_signal, time_signal_scaling);
	ComplexFFT_float(time_signal, freq_signal);

	// Extract imaginary and real part, calculate the magnitude for all frequency bins
	for (i = 0; i <= PART_LEN; i++)
	{
		sum = freq_signal[i].real * freq_signal[i].real + freq_signal[i].imag * freq_signal[i].imag;
		freq_signal_abs[i] = sqrt(sum);

		(*freq_signal_sum_abs) += freq_signal_abs[i];
	}

	return 0;
}
