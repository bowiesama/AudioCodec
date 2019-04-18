
#include ".//Pre_post//pre_pos_analysis.h"
#include ".//synth//synth.h"
#include ".//WaveFunction//WaveIO.h"
#include ".//Analysis//Analysis.h"
#include "./aec/AEC.h"
#include <stdio.h>
#include <memory.h>
#include <intrin.h>
#define cpu  3.0387e+009
#define framesize 10.f

typedef __int64 LONGLONG;
unsigned __int64 rdtsc()
{
	return __rdtsc();
}

int main()
{
	CAnalysis *anafil[2];
	CSynthesis *synfil[2];
	CPrePosAna *preposfil;

	SWavFileHead wavhead;
	short *data_in_s,*data_out_s;
	float *data_in_f,*data_out_f;		
	char *infile;
	char *outfile;
	CWavFileOp *readfile;
	CWavFileOp *writefile;
	CAEC *AEC;
	long filelen,chanlelen;
	long i,k;
	long chanle_spos;
	LONGLONG kclock=0;
	int outfileleng;
    float * FFTbuffer;
	int fftlen;
	
	infile="mulanshi_48k.wav";
	outfile="mulanshi_out_48k.wav";
	readfile=new CWavFileOp(infile,"rb");
	if (readfile->m_FileStatus==-2)
	{
		delete readfile;
		return 0;
	}
	writefile=new CWavFileOp(outfile,"wb");

	readfile->ReadHeader(&wavhead);
	filelen=wavhead.RawDataFileLength/wavhead.BytesPerSample*wavhead.NChannels;
   

	int fremaelen=int(framesize*wavhead.SampleRate/1000);
    data_in_s=new short[fremaelen*wavhead.NChannels];
	data_out_s=new short[fremaelen*wavhead.NChannels];
	memset(data_out_s,0,(fremaelen*wavhead.NChannels)*sizeof(short));
    memset(data_in_s,0,fremaelen*wavhead.NChannels*sizeof(short));
	data_in_f=new float[fremaelen*wavhead.NChannels];
	data_out_f=new float[fremaelen*wavhead.NChannels];

	memset(data_out_f,0,(fremaelen*wavhead.NChannels)*sizeof(float));
	memset(data_in_f,0,fremaelen*wavhead.NChannels*sizeof(float));

    anafil[0]=new CAnalysis(wavhead.SampleRate,10.f,16.f);
	synfil[0]=new CSynthesis(wavhead.SampleRate,10.f,16.f);
	preposfil=new CPrePosAna(wavhead.SampleRate,10.f,wavhead.NChannels);
	if(wavhead.NChannels==2)
	{
		anafil[1]=new CAnalysis(wavhead.SampleRate,10.f,16.f);
		synfil[1]=new CSynthesis(wavhead.SampleRate,10.f,16.f);
		//preposfil[1]=new CPrePosAna(wavhead.SampleRate,10.f);

	}
	AEC=new CAEC(wavhead.SampleRate,16.f,10.f);

	fftlen=int(16.f*wavhead.SampleRate/1000);
    FFTbuffer=new float[fftlen*2*wavhead.NChannels];
	memset((void*)FFTbuffer,0,fftlen*2*wavhead.NChannels*sizeof(float));

	writefile->WriteHeader(wavhead);
	outfileleng=0;
	while (outfileleng<(filelen-fremaelen*wavhead.NChannels))
	{
		outfileleng+= readfile->ReadSample(data_in_s,fremaelen*wavhead.NChannels);
		for (k=0;k<wavhead.NChannels;k++)
		{
			chanle_spos=k*fremaelen;

			for(i=0;i<fremaelen;i++)
			{
				data_in_f[i+chanle_spos]=float(data_in_s[i*wavhead.NChannels+k])/32768.f;
			}
		}
			/////////////
		    //DC remover+pre-emphasizer
			preposfil->preprocess(data_in_f,data_out_f);

			for (k=0;k<wavhead.NChannels;k++)
			{
				//analysis filter bank
				anafil[k]->analyse_process(data_out_f+k*fremaelen, FFTbuffer+fftlen*2*k);
				//////////////////////////////////////////////////////////////////////////
				//ad filter

				//NLP

				//////////////////////////////////////////////////////////////////////////
				//synthesis filter bank
				synfil[k]->Synth_process(FFTbuffer+fftlen*2*k,data_out_f+k*fremaelen);
				// post-emphasizer  
				preposfil->ecpostfilt(data_out_f,k);

			}

           

			/////////////
			for (k=0;k<wavhead.NChannels;k++)
			{
				chanle_spos=k*fremaelen;

				for(i=0;i<(fremaelen);i++)
				{
					data_out_s[i*wavhead.NChannels+k]=int(data_out_f[i+int(chanle_spos)]*32768.f);
				}
			}
		writefile->WriteSample(data_out_s,(fremaelen*wavhead.NChannels));//

	}	

	
	//writefile->UpdateHeader(wavhead.NChannels,int(chanlelen/F)+1000);
	
	
	printf("\n Whole data length=%f s",float(filelen)/wavhead.SampleRate);
	printf("\n Compute time     =%f s\n",kclock/cpu);
	printf("Compute time percentage= %f%% \n",kclock*wavhead.SampleRate/cpu/float(filelen)*100);
	delete readfile;
	delete writefile;;
	delete data_out_f;
	delete data_in_s;
	delete data_in_f;
	delete data_out_s;
	delete anafil[0];
	delete synfil[0];
	delete preposfil;



	return 1;
}