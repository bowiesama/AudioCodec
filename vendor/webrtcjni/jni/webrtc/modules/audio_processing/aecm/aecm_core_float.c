#include "aecm_core.h"
//#include "delay_estimator_float.h"


#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../utility/delay_estimator_wrapper.h"
#include "include/echo_control_mobile.h"
#include "../utility/ring_buffer.h"
#include "../../../system_wrappers/interface/compile_assert.h"
#include "../../../system_wrappers/interface/cpu_features_wrapper.h"
#include "../../../typedefs.h"

//#define	ALY_DBG
#ifdef	ALY_DBG
FILE	*fnlp;
FILE	*fhnl;
FILE	*ffe;
FILE	*fw;
FILE	*fgn;
FILE	*fnclean;
FILE	*fvad;
FILE	*ffv;

#endif

int WebRtcAecm_CreateCoreFloat(AecmCore_float **aecmInst)
{
	AecmCore_float *aecm =  malloc(sizeof(AecmCore_float));
	*aecmInst = aecm;
	if (aecm == NULL)
	{
		return -1;
	}

	aecm->farFrameBuf = WebRtc_CreateBuffer(FRAME_LEN + PART_LEN,
		sizeof(int16_t));


	if (!aecm->farFrameBuf)
	{
		WebRtcAecm_FreeCoreFloat(aecm);
		aecm = NULL;
		return -1;
	}


	aecm->nearNoisyFrameBuf = WebRtc_CreateBuffer(FRAME_LEN + PART_LEN,
		sizeof(int16_t));
	if (!aecm->nearNoisyFrameBuf)
	{
		WebRtcAecm_FreeCoreFloat(aecm);
		aecm = NULL;
		return -1;
	}

	aecm->nearCleanFrameBuf = WebRtc_CreateBuffer(FRAME_LEN + PART_LEN,
		sizeof(int16_t));
	if (!aecm->nearCleanFrameBuf)
	{
		WebRtcAecm_FreeCoreFloat(aecm);
		aecm = NULL;
		return -1;
	}

	aecm->outFrameBuf = WebRtc_CreateBuffer(FRAME_LEN + PART_LEN,
		sizeof(int16_t));
	if (!aecm->outFrameBuf)
	{
		WebRtcAecm_FreeCoreFloat(aecm);
		aecm = NULL;
		return -1;
	}

	aecm->delay_estimator_farend = WebRtc_CreateDelayEstimatorFarend(PART_LEN1,
		MAX_DELAY);
	if (aecm->delay_estimator_farend == NULL) {
		WebRtcAecm_FreeCoreFloat(aecm);
		aecm = NULL;
		return -1;
	}


	aecm->delay_estimator =
		WebRtc_CreateDelayEstimator(aecm->delay_estimator_farend, 0);
	if (aecm->delay_estimator == NULL) {
		WebRtcAecm_FreeCoreFloat(aecm);
		aecm = NULL;
		return -1;
	}

#ifdef	ALY_DBG
	fnlp = fopen("pnlp.raw", "wb");
	fhnl = fopen("phnl.raw", "wb");
	ffe = fopen("fe.raw", "wb");
	fw = fopen("wn.raw", "wb");
	fgn = fopen("gain.raw", "wb");
	fvad = fopen("vad.raw", "wb");
	ffv = fopen("fv.raw", "wb");
	fnclean = fopen("nc.raw", "wb");

#endif
	return 0;
}


int WebRtcAecm_FreeCoreFloat(AecmCore_float *aecm)
{
	if (aecm == NULL)
	{
		return -1;
	}

	WebRtc_FreeBuffer(aecm->farFrameBuf);
	WebRtc_FreeBuffer(aecm->nearNoisyFrameBuf);
	WebRtc_FreeBuffer(aecm->nearCleanFrameBuf);
	WebRtc_FreeBuffer(aecm->outFrameBuf);

	free(aecm);

#ifdef	ALY_DBG
	if (fnlp != NULL)
		fclose(fnlp);
	if (fhnl != NULL)
		fclose(fhnl);
	if (ffe != NULL)
		fclose(ffe);
	if (fgn != NULL)
		fclose(fgn);
	if (fnclean != NULL)
		fclose(fnclean);
	if (fw != NULL)
		fclose(fw);
	if (fvad != NULL)
		fclose(fvad);
	if (ffv != NULL)
		fclose(ffv);
#endif
	return 0;
}

void WebRtcAecm_InitEchoPathCoreFloat(AecmCore_float* aecm, const WebRtc_Word16* echo_path)
{
    int i = 0;
		float echo_path_tmp[PART_LEN1];

		for (i = 0; i < PART_LEN1; i++)
		{
			echo_path_tmp[i] = (float) echo_path[i] / (float)(1 << RESOLUTION_CHANNEL16);
		}

    // Reset the stored channel
    memcpy(aecm->channelStored, echo_path_tmp, sizeof(float) * PART_LEN1);
    // Reset the adapted channels
    memcpy(aecm->channelAdapt16, echo_path_tmp, sizeof(float) * PART_LEN1);
    for (i = 0; i < PART_LEN1; i++)
    {
        aecm->channelAdapt32[i] = (aecm->channelAdapt16[i]) * ONE_Q16_FLOAT;
    }

    // Reset channel storing variables
    aecm->mseAdaptOld = 1000 / ONE_Q8_FLOAT;
    aecm->mseStoredOld = 1000 / ONE_Q8_FLOAT;
    aecm->mseThreshold = WEBRTC_SPL_WORD32_MAX;
    aecm->mseChannelCount = 0;

};
static const WebRtc_Word16 kChannelStored8kHz[PART_LEN1] = {
	2040,   1815,   1590,   1498,   1405,   1395,   1385,   1418,
	1451,   1506,   1562,   1644,   1726,   1804,   1882,   1918,
	1953,   1982,   2010,   2025,   2040,   2034,   2027,   2021,
	2014,   1997,   1980,   1925,   1869,   1800,   1732,   1683,
	1635,   1604,   1572,   1545,   1517,   1481,   1444,   1405,
	1367,   1331,   1294,   1270,   1245,   1239,   1233,   1247,
	1260,   1282,   1303,   1338,   1373,   1407,   1441,   1470,
	1499,   1524,   1549,   1565,   1582,   1601,   1621,   1649,
	1676
};

// Initialization table for echo channel in 16 kHz
static const WebRtc_Word16 kChannelStored16kHz[PART_LEN1] = {
	2040,   1590,   1405,   1385,   1451,   1562,   1726,   1882,
	1953,   2010,   2040,   2027,   2014,   1980,   1869,   1732,
	1635,   1572,   1517,   1444,   1367,   1294,   1245,   1233,
	1260,   1303,   1373,   1441,   1499,   1549,   1582,   1621,
	1676,   1741,   1802,   1861,   1921,   1983,   2040,   2102,
	2170,   2265,   2375,   2515,   2651,   2781,   2922,   3075,
	3253,   3471,   3738,   3976,   4151,   4258,   4308,   4288,
	4270,   4253,   4237,   4179,   4086,   3947,   3757,   3484,
	3153
};


int WebRtcAecm_InitCoreFloat(AecmCore_float * const aecm, int samplingFreq)
{
	int i = 0;
	int tmp32 = PART_LEN1 * PART_LEN1;
	short tmp16 = PART_LEN1;

	if (samplingFreq != 8000 && samplingFreq != 16000)
	{
		samplingFreq = 8000;
		return -1;
	}
	// sanity check of sampling frequency
	aecm->mult = (WebRtc_Word16)samplingFreq / 8000;

	aecm->farBufWritePos = 0;
	aecm->farBufReadPos = 0;
	aecm->knownDelay = 0;
	aecm->lastKnownDelay = 0;

	WebRtc_InitBuffer(aecm->farFrameBuf);
	WebRtc_InitBuffer(aecm->nearNoisyFrameBuf);
	WebRtc_InitBuffer(aecm->nearCleanFrameBuf);
	WebRtc_InitBuffer(aecm->outFrameBuf);

	memset(aecm->xBuf_buf, 0, sizeof(aecm->xBuf_buf));
	memset(aecm->dBufClean_buf, 0, sizeof(aecm->dBufClean_buf));
	memset(aecm->dBufNoisy_buf, 0, sizeof(aecm->dBufNoisy_buf));
	memset(aecm->outBuf_buf, 0, sizeof(aecm->outBuf_buf));

	aecm->seed = 666;
	aecm->totCount = 0;

	if (WebRtc_InitDelayEstimatorFarend(aecm->delay_estimator_farend) != 0) {
		return -1;
	}
	if (WebRtc_InitDelayEstimator(aecm->delay_estimator) != 0) {
		return -1;
	}
	// Set far end histories to zero
	memset(aecm->far_history, 0, sizeof(uint16_t) * PART_LEN1 * MAX_DELAY);

	aecm->far_history_pos = MAX_DELAY;

	aecm->nlpFlag = 1;
	aecm->fixedDelay = -1;

	memset(aecm->nearLogEnergy, 0, sizeof(aecm->nearLogEnergy));
	aecm->farLogEnergy = 0;
	memset(aecm->echoAdaptLogEnergy, 0, sizeof(aecm->echoAdaptLogEnergy));
	memset(aecm->echoStoredLogEnergy, 0, sizeof(aecm->echoStoredLogEnergy));

	// Initialize the echo channels with a stored shape.
	if (samplingFreq == 8000)
	{
		WebRtcAecm_InitEchoPathCoreFloat(aecm, kChannelStored8kHz);
	}
	else
	{
		WebRtcAecm_InitEchoPathCoreFloat(aecm, kChannelStored16kHz);
	}

	memset(aecm->echoFilt, 0, sizeof(aecm->echoFilt));
	memset(aecm->nearFilt, 0, sizeof(aecm->nearFilt));
	aecm->noiseEstCtr = 0;

	aecm->cngMode = AecmTrue;

	memset(aecm->noiseEstTooLowCtr, 0, sizeof(aecm->noiseEstTooLowCtr));
	memset(aecm->noiseEstTooHighCtr, 0, sizeof(aecm->noiseEstTooHighCtr));
	// Shape the initial noise level to an approximate pink noise.
	for (i = 0; i < (PART_LEN1 >> 1) - 1; i++)
	{
		aecm->noiseEst[i] = (tmp32 << 8);
		tmp16--;
		tmp32 -= (WebRtc_Word32)((tmp16 << 1) + 1);
	}
	for (; i < PART_LEN1; i++)
	{
		aecm->noiseEst[i] = (tmp32 << 8);
	}

	aecm->xBuf = (WebRtc_Word16*) (((uintptr_t)aecm->xBuf_buf + 31) & ~ 31);
	aecm->dBufClean = (WebRtc_Word16*) (((uintptr_t)aecm->dBufClean_buf + 31) & ~ 31);
	aecm->dBufNoisy = (WebRtc_Word16*) (((uintptr_t)aecm->dBufNoisy_buf + 31) & ~ 31);
	aecm->outBuf = (float *) (((uintptr_t)aecm->outBuf_buf + 15) & ~ 15);

	aecm->farEnergyMin = (WEBRTC_SPL_WORD16_MAX / ONE_Q8_FLOAT);
	aecm->farEnergyMax = (WEBRTC_SPL_WORD16_MIN / ONE_Q8_FLOAT);
	aecm->farEnergyMaxMin = 0;
	aecm->farEnergyVAD = FAR_ENERGY_MIN_FLOAT; // This prevents false speech detection at the
	// beginning.
	aecm->farEnergyMSE = 0;
	aecm->currentVADValue = 0;
	aecm->vadUpdateCount = 0;
	aecm->firstVAD = 1;

	aecm->startupState = 0;
	aecm->supGain = SUPGAIN_DEFAULT_FLOAT;
	aecm->supGainOld = SUPGAIN_DEFAULT_FLOAT;

	aecm->supGainErrParamA = SUPGAIN_ERROR_PARAM_A_FLOAT;
	aecm->supGainErrParamD = SUPGAIN_ERROR_PARAM_D_FLOAT;
	aecm->supGainErrParamDiffAB = SUPGAIN_ERROR_PARAM_A_FLOAT - SUPGAIN_ERROR_PARAM_B_FLOAT;
	aecm->supGainErrParamDiffBD = SUPGAIN_ERROR_PARAM_B_FLOAT - SUPGAIN_ERROR_PARAM_D_FLOAT;

	// Assert a preprocessor definition at compile-time. It's an assumption
	// used in assembly code, so check the assembly files before any change.
	COMPILE_ASSERT(PART_LEN % 16 == 0);


#ifdef WEBRTC_DETECT_ARM_NEON
	uint64_t features = WebRtc_GetCPUFeaturesARM();
	if ((features & kCPUFeatureNEON) != 0)
	{
		WebRtcAecm_InitNeon();
	}
#elif defined(WEBRTC_ARCH_ARM_NEON)
	WebRtcAecm_InitNeon();
#endif

	return 0;
}


void WebRtcAecm_BufferFarFrameFloat(AecmCore_float* const aecm,
	const WebRtc_Word16* const farend,
	const int farLen)
{
	int writeLen = farLen, writePos = 0;

	// Check if the write position must be wrapped
	while (aecm->farBufWritePos + writeLen > FAR_BUF_LEN)
	{
		// Write to remaining buffer space before wrapping
		writeLen = FAR_BUF_LEN - aecm->farBufWritePos;
		memcpy(aecm->farBuf + aecm->farBufWritePos, farend + writePos,
			sizeof(WebRtc_Word16) * writeLen);
		aecm->farBufWritePos = 0;
		writePos = writeLen;
		writeLen = farLen - writeLen;
	}

	memcpy(aecm->farBuf + aecm->farBufWritePos, farend + writePos,
		sizeof(WebRtc_Word16) * writeLen);
	aecm->farBufWritePos += writeLen;
}

void WebRtcAecm_FetchFarFrameFloat(AecmCore_float * const aecm, WebRtc_Word16 * const farend,
	const int farLen, const int knownDelay)
{
	int readLen = farLen;
	int readPos = 0;
	int delayChange = knownDelay - aecm->lastKnownDelay;

	aecm->farBufReadPos -= delayChange;

	// Check if delay forces a read position wrap
	while (aecm->farBufReadPos < 0)
	{
		aecm->farBufReadPos += FAR_BUF_LEN;
	}
	while (aecm->farBufReadPos > FAR_BUF_LEN - 1)
	{
		aecm->farBufReadPos -= FAR_BUF_LEN;
	}

	aecm->lastKnownDelay = knownDelay;

	// Check if read position must be wrapped
	while (aecm->farBufReadPos + readLen > FAR_BUF_LEN)
	{

		// Read from remaining buffer space before wrapping
		readLen = FAR_BUF_LEN - aecm->farBufReadPos;
		memcpy(farend + readPos, aecm->farBuf + aecm->farBufReadPos,
			sizeof(WebRtc_Word16) * readLen);
		aecm->farBufReadPos = 0;
		readPos = readLen;
		readLen = farLen - readLen;
	}
	memcpy(farend + readPos, aecm->farBuf + aecm->farBufReadPos,
		sizeof(WebRtc_Word16) * readLen);
	aecm->farBufReadPos += readLen;
}

void UpdateFarHistoryFloat( AecmCore_float* self,
						   float* far_spectrum) {
	// Get new buffer position
	self->far_history_pos++;
	if (self->far_history_pos >= MAX_DELAY) {
	   self->far_history_pos = 0;
	}

	// Update far end spectrum buffer
	memcpy(&(self->far_history[self->far_history_pos * PART_LEN1]),
	   far_spectrum,
	   sizeof(float) * PART_LEN1);

}

static const float* AlignedFarendFloat( AecmCore_float* self, int delay) {
	int buffer_position = 0;
	assert(self != NULL);
	buffer_position = self->far_history_pos - delay;

	// Check buffer position
	if (buffer_position < 0) {
		buffer_position += MAX_DELAY;
	}
	// Return far end spectrum
	return &(self->far_history[buffer_position * PART_LEN1]);
}

static void CalcLinearEnergiesCFloat( AecmCore_float* aecm,
								const float* far_spectrum,
								float* echo_est,
								float* far_energy,
								float* echo_energy_adapt,
								float* echo_energy_stored)
{
	int i;

	// Get energy for the delayed far end signal and estimated
	// echo using both stored and adapted channels.
	for (i = 0; i < PART_LEN1; i++)
	{

		echo_est[i] = aecm->channelStored[i] * far_spectrum[i];

		(*far_energy) += far_spectrum[i];
		(*echo_energy_adapt) += aecm->channelAdapt16[i] *	far_spectrum[i];
		(*echo_energy_stored) += echo_est[i];
	}
}

float WebRtcAecm_AsymFiltFloat(const float filtOld, const float inVal,
							   const short stepSizePos,
							   const short stepSizeNeg)
{
	float retVal;

	if ((filtOld == (WEBRTC_SPL_WORD16_MAX / ONE_Q8_FLOAT)) | (filtOld == (WEBRTC_SPL_WORD16_MIN / ONE_Q8_FLOAT)))
	{
		return inVal;
	}

	retVal = filtOld;
	if (filtOld > inVal)
	{
		retVal -= (filtOld - inVal)/ (1<<stepSizeNeg);
	} else
	{
		retVal += (inVal - filtOld)/ (1<<stepSizePos);
	}

	return retVal;
}

void WebRtcAecm_CalcEnergiesFloat( AecmCore_float * aecm,
							 const float* far_spectrum,
							 const float nearEner,
							 float * echoEst){
	// Local variables
	float tmpAdapt = 0;
	float tmpStored = 0;
	float tmpFar = 0;

	int i;

	float zeros, frac;

	short increase_max_shifts = 4;
	short decrease_max_shifts = 11;
	short increase_min_shifts = 11;
	short decrease_min_shifts = 3;

	float tmp16 = 3.5;
	const float a = 1.0/log(2.0);

	memmove(aecm->nearLogEnergy + 1, aecm->nearLogEnergy,
	 sizeof(float) * (MAX_BUF_LEN - 1));

	if (nearEner)
        aecm->nearLogEnergy[0] = a * log(nearEner) + tmp16;

	CalcLinearEnergiesCFloat(aecm, far_spectrum, echoEst, &tmpFar, &tmpAdapt, &tmpStored);

	// Shift buffers
	memmove(aecm->echoAdaptLogEnergy + 1, aecm->echoAdaptLogEnergy,
	 sizeof(float) * (MAX_BUF_LEN - 1));
	memmove(aecm->echoStoredLogEnergy + 1, aecm->echoStoredLogEnergy,
	 sizeof(float) * (MAX_BUF_LEN - 1));

	if (tmpFar)
        aecm->farLogEnergy = a * log(tmpFar) + tmp16;

	if (tmpAdapt)
        aecm->echoAdaptLogEnergy[0] = a * log(tmpAdapt) + tmp16;

	if (tmpStored)
        aecm->echoStoredLogEnergy[0] = a * log(tmpStored) + tmp16;

	if (aecm->farLogEnergy > FAR_ENERGY_MIN_FLOAT)
	{
	 if (aecm->startupState == 0)
	 {
		 increase_max_shifts = 2;
		 decrease_min_shifts = 2;
		 increase_min_shifts = 8;
	 }

	 aecm->farEnergyMin = WebRtcAecm_AsymFiltFloat(aecm->farEnergyMin, aecm->farLogEnergy,
		 increase_min_shifts, decrease_min_shifts);
	 aecm->farEnergyMax = WebRtcAecm_AsymFiltFloat(aecm->farEnergyMax, aecm->farLogEnergy,
		 increase_max_shifts, decrease_max_shifts);
	 aecm->farEnergyMaxMin = (aecm->farEnergyMax - aecm->farEnergyMin);

	 tmp16 = 10 - aecm->farEnergyMin;
	 if (tmp16 > 0)
	 {
		 tmp16 = tmp16 * FAR_ENERGY_VAD_REGION/512.0;
	 } else
	 {
		 tmp16 = 0.0;
	 }
	 tmp16 += FAR_ENERGY_VAD_REGION/256.0;

	 if ((aecm->startupState == 0) | (aecm->vadUpdateCount > 1024))
	 {
		 // In startup phase or VAD update halted
		 aecm->farEnergyVAD = aecm->farEnergyMin + tmp16;
	 } 
	 else
	 {
		 if (aecm->farEnergyVAD > aecm->farLogEnergy)
		 {
			 //aecm->farEnergyVAD += (aecm->farLogEnergy +	tmp16 -	aecm->farEnergyVAD)/256.0;
			 aecm->farEnergyVAD += (aecm->farLogEnergy +	tmp16 -	aecm->farEnergyVAD)/64.0;
			 aecm->vadUpdateCount = 0;
		 } else
		 {
			 aecm->vadUpdateCount++;
		 }
	 }
	 // Put MSE threshold higher than VAD
	 aecm->farEnergyMSE = aecm->farEnergyVAD + 1.0;
	}

	// Update VAD variables
#ifdef	ALY_DBG
  	fwrite(&aecm->farEnergyVAD, 1, sizeof(float), ffv);
	fwrite(&aecm->farLogEnergy, 1, sizeof(float), ffe);
#endif
	if ((aecm->farLogEnergy > aecm->farEnergyVAD) )//|| ((aecm->totCount>=237) && (aecm->totCount<290)))
	{
	 if ((aecm->startupState == 0) | (aecm->farEnergyMaxMin > (FAR_ENERGY_DIFF / ONE_Q8_FLOAT)))
	 {
		 // We are in startup or have significant dynamics in input speech level
		 aecm->currentVADValue = 1;
	 }
	} else
	{
	 aecm->currentVADValue = 0;
	}

	if ((aecm->currentVADValue) && (aecm->firstVAD))
	{
	 aecm->firstVAD = 0;
	 if (aecm->echoAdaptLogEnergy[0] > aecm->nearLogEnergy[0])
	 {
		 // The estimated echo has higher energy than the near end signal.
		 // This means that the initialization was too aggressive. Scale
		 // down by a factor 8
		 for (i = 0; i < PART_LEN1; i++)
		 {
			 aecm->channelAdapt16[i] /= 8.0;
		 }
		 // Compensate the adapted echo energy level accordingly.
		 aecm->echoAdaptLogEnergy[0] -= 3.0;
		 aecm->firstVAD = 1;
	 }
	}

}

float WebRtcAecm_CalcStepSizeFloat( AecmCore_float * const aecm)
{

	float tmp;
	float a = 2.0;

	float mu_max = MU_MAX;
	float mu_min = MU_MIN;
	float mu = mu_max;

	// Here we calculate the step size mu used in the
	// following NLMS based Channel estimation algorithm
	if (!aecm->currentVADValue)
	{
		// Far end energy level too low, no channel update
		mu = 0.0;
	} else if (aecm->startupState > 0)
	{
		if (aecm->farEnergyMin >= aecm->farEnergyMax)
		{
			mu = mu_min;
		} else
		{
			tmp = MU_DIFF * (1 -(aecm->farLogEnergy - aecm->farEnergyMin)/aecm->farEnergyMaxMin);
			if (tmp < MU_MAX)
			{
				tmp = MU_MAX; // Equivalent with maximum step size of 2^-MU_MAX
			}
			mu = tmp;
		}

	}

	return mu;
}

static void StoreAdaptiveChannelCFloat( AecmCore_float* aecm,
								  const float* far_spectrum,
								  float* echo_est)
{
	int i;

	memcpy(aecm->channelStored, aecm->channelAdapt16, sizeof(float) * PART_LEN1);

	// Recalculate echo estimate
	for (i = 0; i < PART_LEN1; i ++)
	{
		echo_est[i] = aecm->channelStored[i]* far_spectrum[i];
	}
}
static void ResetAdaptiveChannelCFloat( AecmCore_float* aecm)
{
	int i;

	// The stored channel has a significantly lower MSE than the adaptive one for
	// two consecutive calculations. Reset the adaptive channel.
	memcpy(aecm->channelAdapt16, aecm->channelStored,
		sizeof(float) * PART_LEN1);
	// Restore the W32 channel
	for (i = 0; i < PART_LEN1; i += 4)
	{
		aecm->channelAdapt32[i] = aecm->channelStored[i] * ONE_Q16_FLOAT;
	}
}


void WebRtcAecm_UpdateChannelFloat( AecmCore_float * aecm,
							  const float* far_spectrum,
							  const float * const near_spectrum,
							  const float mu,
							  float * echoEst){

  int i,j,k;
  float tmpU32no1, tmpU32no2;
  float tmp32no1, tmp32no2;
  float mseStored;
  float mseAdapt;
	int vad;

  short zerosFar, zerosNum, zerosCh, zerosDfa;
  float shiftChFar, shiftNum, shift2ResChan;
  float tmpno1, tmpno2;
  float xfaQ, dfaQ;
  float a = 2.0;

#ifdef	ALY_DBG
  fwrite(&aecm->channelAdapt16[0], 1, PART_LEN1*sizeof(float), fw);
#endif
  if (mu)
  {
	  for (i = 0; i < PART_LEN1; i++)
	  {
			zerosFar = WebRtcSpl_NormW16((unsigned short)far_spectrum[i]);
		  // Multiplication is safe
		  tmpU32no1 = aecm->channelAdapt16[i]*far_spectrum[i];

		  tmp32no1 = near_spectrum[i] - tmpU32no1;

		  if ((tmp32no1)&& (far_spectrum[i] > (CHANNEL_VAD)))
		  {
			  //
			  // Update is needed
			  //
			  // This is what we would like to compute
			  //
			  // tmp32no1 = dfa[i] - (aecm->channelAdapt[i] * far_spectrum[i])
			  // tmp32norm = (i + 1)
			  // aecm->channelAdapt[i] += (2^mu) * tmp32no1
			  //                        / (tmp32norm * far_spectrum[i])
			  //

			  // Make sure we don't get overflow in multiplication.

//			  aecm->channelAdapt16[i] += pow(a,-mu) * tmp32no1 /(float) (i + 1)/far_spectrum[i];
				aecm->channelAdapt16[i] += pow(a,-mu) * tmp32no1 * far_spectrum[i] / (float) (i + 1)/ pow(a,(15-zerosFar)<<1);

			  if (aecm->channelAdapt16[i] < 0)
			  {
				  // We can never have negative channel gain
				  aecm->channelAdapt16[i] = 0;
			  }
	//		  aecm->channelAdapt16[i]	= aecm->channelAdapt32[i]/32768.0;
		  }
	  }
  }

  // END: Adaptive channel update

  // Determine if we should store or restore the channel
  if ((aecm->startupState == 0) & (aecm->currentVADValue))
  {
	  // During startup we store the channel every block,
	  // and we recalculate echo estimate
	  StoreAdaptiveChannelCFloat(aecm, far_spectrum, echoEst);
  } else
  {
	  if (aecm->farLogEnergy < aecm->farEnergyMSE)
	  {
		  aecm->mseChannelCount = 0;
	  } else
	  {
		  aecm->mseChannelCount++;
	  }
	  if (aecm->mseChannelCount >= (MIN_MSE_COUNT + 10))
	  {
		  // We have enough data.
		  // Calculate MSE of "Adapt" and "Stored" versions.
		  // It is actually not MSE, but average absolute error.
		  mseStored = 0.0;
		  mseAdapt = 0.0;
		  for (i = 0; i < MIN_MSE_COUNT; i++)
		  {
				tmpno1 = aecm->echoStoredLogEnergy[i] - aecm->nearLogEnergy[i];
				tmpno2 = (aecm->echoAdaptLogEnergy[i] - aecm->nearLogEnergy[i]);

			  mseStored += (tmpno1 >0 ? tmpno1 : -tmpno1);
			  mseAdapt += (tmpno2 >0 ? tmpno2 : -tmpno2);
		  }

		  if (((mseStored * MSE_RESOLUTION_FLOAT) < (MIN_MSE_DIFF * mseAdapt))
			  & ((aecm->mseStoredOld * MSE_RESOLUTION_FLOAT) < (MIN_MSE_DIFF	* aecm->mseAdaptOld)))
		  {
			  // The stored channel has a significantly lower MSE than the adaptive one for
			  // two consecutive calculations. Reset the adaptive channel.
			  ResetAdaptiveChannelCFloat(aecm);
		  } 
		  else if ((((MIN_MSE_DIFF * mseStored) > (mseAdapt * MSE_RESOLUTION_FLOAT)) & (mseAdapt
			  < aecm->mseThreshold) & (aecm->mseAdaptOld < aecm->mseThreshold)))
		  {
			  // The adaptive channel has a significantly lower MSE than the stored one.
			  // The MSE for the adaptive channel has also been low for two consecutive
			  // calculations. Store the adaptive channel.
				for (j = 0, k=0; j<PART_LEN1 ; j++)
					if (aecm->channelAdapt16[j]==0)
					{		  
						  aecm->mseChannelCount =0;
							// Store the MSE values.
							aecm->mseStoredOld = mseStored;
							aecm->mseAdaptOld = mseAdapt;
							return;
						
					}


			  StoreAdaptiveChannelCFloat(aecm, far_spectrum, echoEst);

			  // Update threshold
			  if (aecm->mseThreshold == (float)(0x7fffffff))
			  {
				  aecm->mseThreshold = (mseAdapt + aecm->mseAdaptOld);
			  } else
			  {
				  aecm->mseThreshold += (mseAdapt - aecm->mseThreshold * 0.625) * 205.0 / 256.0;
			  }

		  }

		  // Reset counter
		  aecm->mseChannelCount = 0;

		  // Store the MSE values.
		  aecm->mseStoredOld = mseStored;
		  aecm->mseAdaptOld = mseAdapt;
	  }

  }
}

static const short kNoiseEstQDomain = 15;
static const short kNoiseEstIncCount = 5;

static float CalcSuppressionGainFloat( AecmCore_float * const aecm)
{
	float tmp32no1;

	float supGain = SUPGAIN_DEFAULT_FLOAT;
	float tmp16no1;
	float dE = 0.0;
	const float energy_dev_tol = (ENERGY_DEV_TOL / ONE_Q8_FLOAT);

	// Determine suppression gain used in the Wiener filter. The gain is based on a mix of far
	// end energy and echo estimation error.
	// Adjust for the far end signal level. A low signal level indicates no far end signal,
	// hence we set the suppression gain to 0
#ifdef	ALY_DBG
	float	vad = aecm->currentVADValue;
  	fwrite(&vad, 1, sizeof(float), fvad);
#endif
	if (!aecm->currentVADValue)
	{
		supGain = 0.0;
	} else
	{
		// Adjust for possible double talk. If we have large variations in estimation error we
		// likely have double talk (or poor channel).
		tmp16no1 = (aecm->nearLogEnergy[0] - aecm->echoStoredLogEnergy[0] - ENERGY_DEV_OFFSET);
		dE = (tmp16no1 < 0.0) ? -tmp16no1 : tmp16no1;

		if (dE < energy_dev_tol)
		{
			// Likely no double talk. The better estimation, the more we can suppress signal.
			// Update counters
			if (dE < SUPGAIN_EPC_DT_FLOAT)
			{
				tmp32no1 = aecm->supGainErrParamDiffAB * dE /SUPGAIN_EPC_DT_FLOAT;
				supGain = aecm->supGainErrParamA - tmp32no1;
			} else
			{
				tmp32no1 = aecm->supGainErrParamDiffBD * (energy_dev_tol - dE)/ (energy_dev_tol - SUPGAIN_EPC_DT_FLOAT) ;
				supGain = aecm->supGainErrParamD + tmp32no1;
			}
		} else
		{
			// Likely in double talk. Use default value
			supGain = aecm->supGainErrParamD;

		}
	}

	if (supGain > aecm->supGainOld)
	{
		tmp16no1 = supGain;
	} else
	{
		tmp16no1 = aecm->supGainOld;
	}
	aecm->supGainOld = supGain;
	aecm->supGain += (tmp16no1 - aecm->supGain) * 0.0625;
	if(aecm->supGain * ONE_Q8_FLOAT < 1)
		aecm->supGain=0;

	// END: Update suppression gain

	return aecm->supGain;
}


int WebRtcAecm_ProcessBlockFloat( AecmCore_float * aecm,
	const short * farend,
	const short * nearendNoisy,
	const short * nearendClean,
	WebRtc_Word16 * output)
{
	int i;
    int delay;

	float CsohnHistory=0;
	float CsoSmthFactr= 0;
	float CsohnFactor = 0;

	complex_float dfw_buf[PART_LEN2 + 8];
	complex_float efw_buf[PART_LEN2 + 8];
	float echoEst32_buf[PART_LEN1 + 8];

	float xfa[PART_LEN1];
	float dfaNoisy[PART_LEN1];
	float dfaClean[PART_LEN1];
	const float* far_spectrum_ptr = NULL;
	float* ptrDfaClean = dfaClean;

	float xfaSum;
	float dfaNoisySum;
	float dfaCleanSum;

	float mu;
	float hnl[PART_LEN1];
	float numPosCoef = 0;
	float nlpGain = 1.0;
	complex_float* efw= efw_buf;
	complex_float* dfw= dfw_buf;

	float tmp32no1;
	float tmp16no2;
	float tmp16no1;
	float tmpU32;

	float *echoEst32 = echoEst32_buf;
	float echoEst32Gained;
	float supGain;

	const int kMinPrefBand = 4;
	const int kMaxPrefBand = 24;
	float avgHnl32 = 0.0;

	if (aecm->startupState < 2)
	{
		aecm->startupState = (aecm->totCount >= CONV_LEN) + (aecm->totCount >= CONV_LEN2);
	}
	// END: Determine startup state

	// Buffer near and far end signals
	memcpy(aecm->xBuf + PART_LEN, farend, sizeof(WebRtc_Word16) * PART_LEN);
	memcpy(aecm->dBufNoisy + PART_LEN, nearendNoisy, sizeof(WebRtc_Word16) * PART_LEN);
	if (nearendClean != NULL)
	{
		memcpy(aecm->dBufClean + PART_LEN, nearendClean, sizeof(WebRtc_Word16) * PART_LEN);
#ifdef	ALY_DBG
		fwrite(&nearendClean[0], 1, sizeof(WebRtc_Word16) * PART_LEN, fnclean);
#endif
	}
	TimeToFrequencyDomainFloat(aecm,aecm->xBuf,dfw,	xfa,&xfaSum);

	TimeToFrequencyDomainFloat(aecm,aecm->dBufNoisy,dfw,dfaNoisy,	&dfaNoisySum);

	if (nearendClean == NULL)
	{
		ptrDfaClean = dfaNoisy;
		dfaCleanSum = dfaNoisySum;
	} else
	{
		// Transform clean near end signal from time domain to frequency domain.
		TimeToFrequencyDomainFloat(aecm,aecm->dBufClean,dfw,dfaClean,&dfaCleanSum);

	}

	// Get the delay
	// Save far-end history and estimate delay
	UpdateFarHistoryFloat(aecm, xfa);
	if (WebRtc_AddFarSpectrumFloat(aecm->delay_estimator_farend, xfa, PART_LEN1) == -1) {
			return -1;
	}
	delay = WebRtc_DelayEstimatorProcessFloat(aecm->delay_estimator,
		dfaNoisy,
		PART_LEN1);

	if (delay == -1)
	{
		return -1;
	}
	else if (delay == -2)
	{
		// If the delay is unknown, we assume zero.
		// NOTE: this will have to be adjusted if we ever add lookahead.
		delay = 0;
	}

	if (aecm->fixedDelay >= 0)
	{
		// Use fixed delay
		delay = aecm->fixedDelay;
	}
//	 printf("%d\n",delay);

	far_spectrum_ptr = AlignedFarendFloat(aecm, delay);

	if (far_spectrum_ptr == NULL)
	{
		return -1;
	}

	// Calculate log(energy) and update energy threshold levels
	WebRtcAecm_CalcEnergiesFloat(aecm,	far_spectrum_ptr,	dfaNoisySum,echoEst32);

	// Calculate stepsize
	mu = WebRtcAecm_CalcStepSizeFloat(aecm);

	// Update counters
	aecm->totCount++;

	// This is the channel estimation algorithm.
	// It is base on NLMS but has a variable step length, which was calculated above.
	WebRtcAecm_UpdateChannelFloat(aecm, far_spectrum_ptr, dfaNoisy, mu, echoEst32);

	supGain = CalcSuppressionGainFloat(aecm);

#ifdef	ALY_DBG
	if (fgn != NULL)
		fwrite(&supGain, 1, sizeof(float), fgn);
#endif

// Calculate Wiener filter hnl[]
		for (i = 0; i < PART_LEN1; i++)
		{
			// Far end signal through channel estimate in Q8
			// How much can we shift right to preserve resolution
			tmp32no1 = echoEst32[i] - aecm->echoFilt[i];
			aecm->echoFilt[i] += tmp32no1 * 50.0/256.0;

			echoEst32Gained = aecm->echoFilt[i] * supGain;

			tmp16no1 = aecm->nearFilt[i];
			tmp32no1 = (ptrDfaClean[i] - aecm->nearFilt[i]);
			tmp16no2 = tmp32no1/16 + tmp16no1;
			aecm->nearFilt[i] = tmp16no2;


			// Wiener filter coefficients, resulting hnl in Q14
			if (echoEst32Gained == 0)
				hnl[i] = 1.0;
			else if (aecm->nearFilt[i] == 0)
				hnl[i] = 0.0;
			else {
				// Multiply the suppression gain // Rounding
				tmp32no1 = echoEst32Gained/aecm->nearFilt[i];

                // 1-echoEst/dfa
				if (tmp32no1 >= 1.0)
					hnl[i] = 0;
				else if (tmp32no1 <= 0) 
					hnl[i] = 1.0;
				else    
					hnl[i] = 1.0 - tmp32no1;
			}

			if (hnl[i])
				numPosCoef++;
		}

		// Only in wideband. Prevent the gain in upper band from being larger than
		// in lower band.
		if (aecm->mult == 2)
		{
			// TODO(bjornv): Investigate if the scaling of hnl[i] below can cause
			//               speech distortion in double-talk.
			for (i = 0; i < PART_LEN1; i++)
				hnl[i] = hnl[i]* hnl[i];

			for (i = kMinPrefBand; i <= kMaxPrefBand; i++)
				avgHnl32 += hnl[i];

			assert(kMaxPrefBand - kMinPrefBand + 1 > 0);
			avgHnl32 /= (kMaxPrefBand - kMinPrefBand + 1);

			for (i = kMaxPrefBand; i < PART_LEN1; i++)
			{
				if (hnl[i] > avgHnl32)
				{
					hnl[i] = avgHnl32;
				}
			}
		}

		// Calculate NLP gain, result is in Q14
    if (aecm->nlpFlag)
    {
        if(aecm->currentVADValue)
        {
            CsoSmthFactr=0.8;
        }
        else
        {
            CsoSmthFactr=0.08;
        }
        
		CsohnFactor = CsoSmthFactr*aecm->currentVADValue+(1-CsoSmthFactr)*CsohnHistory;
		CsohnHistory = CsohnFactor;
        
        for (i = 0; i < PART_LEN1; i++)
        {
            // Truncate values close to zero and one.
            if (hnl[i] > 1.0)
            {
                hnl[i] = 1.0;
            } else if (hnl[i] < 0.2)
            {
                hnl[i] = 0;
            }

            // Remove outliers
            if (numPosCoef < 3)
                nlpGain = 0;
            else
                nlpGain = 1.0;

#ifdef	ALY_DBG
			if (fnlp != NULL)
				fwrite(&nlpGain, 1, sizeof(float), fnlp);
			if (fhnl != NULL)
				fwrite(&hnl[i], 1, sizeof(float), fhnl);
#endif
            // NLP
            if ((hnl[i] == 1.0) && (nlpGain == 1.0))
            {
                hnl[i] = 1.0;
            } else
            {
                hnl[i] =hnl[i] * nlpGain;
            }

            // multiply with Wiener coefficients
            efw[i].real = dfw[i].real * hnl[i];
            efw[i].imag = dfw[i].imag * hnl[i];
        }
    }
    else
    {
        // multiply with Wiener coefficients
        for (i = 0; i < PART_LEN1; i++)
        {
					efw[i].real = dfw[i].real * hnl[i];
					efw[i].imag = dfw[i].imag * hnl[i];
        }
    }


	if (aecm->cngMode == AecmTrue)
	{
//		ComfortNoise(aecm, ptrDfaClean, efw, hnl);
	}

	InverseFFT_float(aecm, efw, output, nearendClean);

/*
	memcpy(aecm->xBuf, aecm->xBuf + PART_LEN, sizeof(WebRtc_Word16) * PART_LEN);
	memcpy(aecm->dBufNoisy, aecm->dBufNoisy + PART_LEN, sizeof(WebRtc_Word16) * PART_LEN);
	if (nearendClean != NULL)
	{
		memcpy(aecm->dBufClean, aecm->dBufClean + PART_LEN, sizeof(WebRtc_Word16) * PART_LEN);
	}

	*/

	return 0;
}

int WebRtcAecm_ProcessFrameFloat(AecmCore_float * aecm,
	const WebRtc_Word16 * farend,
	const WebRtc_Word16 * nearendNoisy,
	const WebRtc_Word16 * nearendClean,
	WebRtc_Word16 * out)
{
	WebRtc_Word16 outBlock_buf[PART_LEN + 8]; // Align buffer to 8-byte boundary.
	WebRtc_Word16* outBlock = (WebRtc_Word16*) (((uintptr_t) outBlock_buf + 15) & ~ 15);

	WebRtc_Word16 farFrame[FRAME_LEN];
	const int16_t* out_ptr = NULL;
	int size = 0;
	long out_energy_sum = 0;
	int out_energy_avg = 0;
	int i = 0;

	// Buffer the current frame.
	// Fetch an older one corresponding to the delay.
	WebRtcAecm_BufferFarFrameFloat(aecm, farend, FRAME_LEN);
	WebRtcAecm_FetchFarFrameFloat(aecm, farFrame, FRAME_LEN, aecm->knownDelay);

	// Buffer the synchronized far and near frames,
	// to pass the smaller blocks individually.
	WebRtc_WriteBuffer(aecm->farFrameBuf, farFrame, FRAME_LEN);
	WebRtc_WriteBuffer(aecm->nearNoisyFrameBuf, nearendNoisy, FRAME_LEN);
	if (nearendClean != NULL)
	{
		WebRtc_WriteBuffer(aecm->nearCleanFrameBuf, nearendClean, FRAME_LEN);
	}

	// Process as many blocks as possible.
	while (WebRtc_available_read(aecm->farFrameBuf) >= PART_LEN)
	{
		int16_t far_block[PART_LEN];
		const int16_t* far_block_ptr = NULL;
		int16_t near_noisy_block[PART_LEN];
		const int16_t* near_noisy_block_ptr = NULL;

		WebRtc_ReadBuffer(aecm->farFrameBuf, (void**) &far_block_ptr, far_block,
			PART_LEN);
		WebRtc_ReadBuffer(aecm->nearNoisyFrameBuf,
			(void**) &near_noisy_block_ptr,
			near_noisy_block,
			PART_LEN);
		if (nearendClean != NULL)
		{
			int16_t near_clean_block[PART_LEN];
			const int16_t* near_clean_block_ptr = NULL;

			WebRtc_ReadBuffer(aecm->nearCleanFrameBuf,
				(void**) &near_clean_block_ptr,
				near_clean_block,
				PART_LEN);
			if (WebRtcAecm_ProcessBlockFloat(aecm,
				far_block_ptr,
				near_noisy_block_ptr,
				near_clean_block_ptr,
				outBlock) == -1)
			{
				return -1;
			}
		} else
		{
			if (WebRtcAecm_ProcessBlockFloat(aecm,
				far_block_ptr,
				near_noisy_block_ptr,
				NULL,
				outBlock) == -1)
			{
				return -1;
			}
		}

		WebRtc_WriteBuffer(aecm->outFrameBuf, outBlock, PART_LEN);
	}

	// Stuff the out buffer if we have less than a frame to output.
	// This should only happen for the first frame.
	size = (int) WebRtc_available_read(aecm->outFrameBuf);
	if (size < FRAME_LEN)
	{
		WebRtc_MoveReadPtr(aecm->outFrameBuf, size - FRAME_LEN);
	}

	// Obtain an output frame.
	WebRtc_ReadBuffer(aecm->outFrameBuf, (void**) &out_ptr, out, FRAME_LEN);
	if (out_ptr != out) {
		// ReadBuffer() hasn't copied to |out| in this case.
		memcpy(out, out_ptr, FRAME_LEN * sizeof(int16_t));
		for(i = 0; i < FRAME_LEN; i++)
		{
			out_energy_sum += out[i] * out[i];
		}
		out_energy_avg = out_energy_sum / FRAME_LEN;
		//     printf("frame count = %d, Out Energy = %d\n", frameCount, out_energy_avg);
	}

	return 0;
}
