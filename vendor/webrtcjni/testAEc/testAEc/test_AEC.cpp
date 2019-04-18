
#include "stdafx.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// add by chen, for test
#include <sstream>
#include <sys/stat.h>
#include <algorithm>
#include "WaveIO.h"

#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/interface/module_common_types.h"
//#include "..\..\jni\webrtc\modules\audio_processing\include\audio_processing.h"
//e:\cwcochlealib01\webrtcjni\jni\webrtc\modules\audio_processing\include\audio_processing.h

using namespace webrtc;

#define framesize 10.f
#define sample_rate_hz 8000
#define framelen 10
#define num_render_channels 1

int main()
{
	FILE *framefile;
	//CAnalysis *anafil_des,*anafil_ref;
	//CSynthesis *synfil_des;
	//CPrePosAna *preposfil_des,*preposfil_ref;
#ifdef debug0
	__int64 kclock=0;
#endif
	SWavFileHead wavhead;
	short *data_in_s,*data_out_s;
	//float *data_in_f_des,*data_in_f_ref,*data_out_f_des,*data_out_f_ref;		
	char *nearfile, *farfile;
	char *outfile;
	CWavFileOp *readfarfile;
	CWavFileOp *readnearfile;
	CWavFileOp *writefile;

	AudioFrame far_frame;
	AudioFrame near_frame;
	
	int capture_level=127;
	int samples_per_channel;
	long farfilelen,nearfilelen,chanlelen;
	long processinglen;
	long i,k;
	long chanle_spos;
	long outfileleng;
	//float * FFTbuffer_des,*FFTbuffer_ref;
	//int fftlen;

	framefile=fopen("frmaeAECCPU.txt","w");

	nearfile="near.wav";
	farfile = "far.wav";
	outfile="aecmOut.wav";
	readnearfile=new CWavFileOp(nearfile,"rb");
	readfarfile = new CWavFileOp(farfile,"rb");
	if (readfarfile->m_FileStatus==-2)
	{
		delete readfarfile;
		return 0;
	}
	if (readnearfile->m_FileStatus==-2)
	{
		delete readfarfile;
		return 0;
	}

	writefile=new CWavFileOp(outfile,"wb");
	if (writefile->m_FileStatus==-2)
	{
		delete writefile;
		return 0;
	}

	readfarfile->ReadHeader(&wavhead);
	//input file: left channel is mic, right channel is speaker
	//output file: left channel is mic, right channel is error
	if (wavhead.NChannels!=1)
	{
		return 0;
	}
	if (wavhead.SampleRate!=8000)
	{
		return -1000;
	}
	farfilelen=wavhead.RawDataFileLength/wavhead.BytesPerSample*wavhead.NChannels;

	readnearfile->ReadHeader(&wavhead);
	//input file: left channel is mic, right channel is speaker
	//output file: left channel is mic, right channel is error
	if (wavhead.NChannels!=1)
	{
		return 0;
	}
	if (wavhead.SampleRate!=8000)
	{
		return -1000;
	}
	nearfilelen=wavhead.RawDataFileLength/wavhead.BytesPerSample*wavhead.NChannels;
	
	processinglen = farfilelen>=nearfilelen?nearfilelen:farfilelen;

	int fremaelen=int(framesize*wavhead.SampleRate/1000);
	
	data_in_s=new short[fremaelen*wavhead.NChannels];
	data_out_s=new short[fremaelen*wavhead.NChannels*2];
	/*memset(data_in_s,0,(fremaelen*wavhead.NChannels*2)*sizeof(short));
	//memset(data_in_s,0,fremaelen*wavhead.NChannels*sizeof(short));
	data_in_f_des=new float[fremaelen*4];
	data_out_f_des=data_in_f_des+fremaelen;
	data_in_f_ref=data_out_f_des+fremaelen;
	data_out_f_ref=data_in_f_ref+fremaelen;

	memset(data_in_f_des,0,(fremaelen*4)*sizeof(float));
	*/
	//memset(data_in_f,0,fremaelen*wavhead.NChannels*sizeof(float));
#ifdef unpreprocess
    preposfil_des=new CPrePosAna(wavhead.SampleRate,10.f,1);
#endif
	/*
	anafil_des=new CAnalysis(wavhead.SampleRate,10.f,16.f);
	synfil_des=new CSynthesis(wavhead.SampleRate,10.f,16.f);
	
	anafil_ref=new CAnalysis(wavhead.SampleRate,10.f,16.f);
	//synfil_des=new CSynthesis(wavhead.SampleRate,10.f,16.f);
	preposfil_ref=new CPrePosAna(wavhead.SampleRate,10.f,1);
	
	if(wavhead.NChannels==2)
	{
		anafil[1]=new CAnalysis(wavhead.SampleRate,10.f,16.f);
		synfil[1]=new CSynthesis(wavhead.SampleRate,10.f,16.f);
		//preposfil[1]=new CPrePosAna(wavhead.SampleRate,10.f);

	}	
	*/
	// creat apm and enable aecm
	AudioProcessing* apm = AudioProcessing::Create(0);
	apm->set_sample_rate_hz(8000);
	apm->echo_control_mobile()->Enable(true);
	apm->set_stream_delay_ms(0);
	apm->echo_cancellation()->set_stream_drift_samples(0);


	samples_per_channel = sample_rate_hz*framelen/1000;
	far_frame.sample_rate_hz_ = sample_rate_hz;
    far_frame.samples_per_channel_ = samples_per_channel;
    far_frame.num_channels_ = num_render_channels;
    near_frame.sample_rate_hz_ = sample_rate_hz;
    near_frame.samples_per_channel_ = samples_per_channel;


//	AEC=new CAEC(wavhead.SampleRate,16.f,10.f,32768.f);
//    AEC->SetAEC_OnOff(OFF);
//	AEC->SetNR_OnOff(ON);
	//fftlen=int(16.f*wavhead.SampleRate/1000);
	//FFTbuffer_des=new float[fftlen*2];
	//FFTbuffer_ref=FFTbuffer_des+fftlen;
	//memset((void*)FFTbuffer_des,0,fftlen*2*sizeof(float));

	writefile->WriteHeader(wavhead);
	outfileleng=0;
	while (outfileleng<(processinglen-fremaelen*wavhead.NChannels))
	{
		
		outfileleng+= readfarfile->ReadSample(data_in_s,fremaelen*wavhead.NChannels);
		memcpy((void *)far_frame.data_,(void *)data_in_s, fremaelen*wavhead.NChannels*2);
		//outfileleng+= readfarfile->ReadSample(far_frame.data_,fremaelen*wavhead.NChannels);
		readnearfile->ReadSample(near_frame.data_,fremaelen*wavhead.NChannels);
#ifdef debug0

#endif

#ifdef debug0
		//kclock=0;
		//kclock-=rdtsc();
#endif
#ifdef unpreprocess
		preposfil_des->preprocess(data_out_f_des,data_in_f_des,0);
#endif
		apm->AnalyzeReverseStream(&far_frame);

		const int capture_level_in = capture_level;
        //ASSERT_EQ(apm->kNoError,
        //          apm->gain_control()->set_stream_analog_level(capture_level));
		apm->gain_control()->set_stream_analog_level(capture_level);
        //ASSERT_EQ(apm->kNoError,
        //          apm->set_stream_delay_ms(delay_ms + extra_delay_ms));
		apm->set_stream_delay_ms(0);
        apm->echo_cancellation()->set_stream_drift_samples(0);
		int err = apm->ProcessStream(&near_frame);

		apm->num_output_channels();
        capture_level = apm->gain_control()->stream_analog_level();
  //      stream_has_voice =
   //         static_cast<int8_t>(apm->voice_detection()->stream_has_voice());
 //       if (err == apm->kBadStreamParameterWarning) {
 //         printf("Bad parameter warning. %s\n", trace_stream.str().c_str());
 //       }

//		AEC->process(data_in_f_des,data_in_f_ref,data_out_f_des,fremaelen,fremaelen);
#ifdef debug0	
	//	kclock+=rdtsc()-100;
		//printf("kclock=%i\n",kclock);
		//fprintf(framefile,"%d   %d\n",(outfileleng/fremaelen),kclock);
#endif

		/////////////
		/*
		for(i=0;i<(fremaelen);i++)
		{
			data_out_s[i*wavhead.NChannels  ]=data_in_s[i*wavhead.NChannels];
			data_out_s[i*wavhead.NChannels+1]=near_frame.data_(i*wavhead.NChannels); // short(data_out_f_des[i]);//*32768.f
		}
		*/
		writefile->WriteSample(near_frame.data_,(fremaelen*wavhead.NChannels));//

	}	
	//writefile->UpdateHeader(wavhead.NChannels,int(chanlelen/F)+1000);
#ifdef debug0
	//printf("\n Whole data length=%f s",float(farfilelen)/wavhead.SampleRate/wavhead.NChannels);
//	printf("\n Compute time     =%f s\n",kclock/cpu);
//	printf("Compute time percentage= %f%% \n",kclock*wavhead.SampleRate/cpu/float(filelen)*100*wavhead.NChannels);
#endif

	if(NULL!= framefile)
	{
		fclose(framefile);
		framefile = NULL;
	}
	if(NULL!= readfarfile)
	{
		delete readfarfile;
		readfarfile = NULL;
	}
	if(NULL!= readnearfile)
	{
		delete readnearfile;
		readnearfile =NULL;
	}
	if(NULL!= writefile)
	{
		delete writefile;
		writefile =NULL;
	}
//	if(NULL!= data_in_f_des)
//	{
//		delete data_in_f_des;
//		data_in_f_des = NULL;
//	}
	if(NULL!= data_in_s)
	{
	//delete data_in_s;
	//delete data_in_f;
		delete data_in_s;
		data_in_s = NULL;
	}
	if(NULL!= data_out_s)
	{
		delete data_out_s;
		data_out_s = NULL;
	}
	/*
	if(NULL!= nearfile)
	{
		delete nearfile;
		nearfile = NULL;
	}	
	if(NULL!= farfile)
	{
		delete farfile;
		farfile = NULL;
	}	
	if(NULL!= outfile)
	{
		delete outfile;
		outfile = NULL;
	}
	*/

	//delete anafil_des;
	//delete synfil_des;
//	delete preposfil_des;



	return 1;
}