
#ifndef G722_H
#define G722_H 200

/*
The program is available by ftp from CMU, and may be used and distributed
freely, provided the copyright notices are maintained.

The Carnegie Mellon ADPCM program is Copyright (c) 1993
by Carnegie Mellon University. Use of this program, for any research or
commercial purpose, is completely unrestricted.  If you make use of or
redistribute this material, we would appreciate acknowlegement of its origin.
*/


/*
 * It's based on source code from  ftp://ftp.cs.cmu.edu/project/fgdata/speech-compression/CCITT-ADPCM/64kbps/adpcm64_g722/, 
 * The source code is in public domain, no copyright is claimed.
 */


/* DEFINITION FOR SMART PROTOTYPES */
#ifndef ARGS
#if (defined(__STDC__) || defined(VMS) || defined(__DECC)  || defined(MSDOS) || defined(__MSDOS__)) || defined (__CYGWIN__) || defined (_MSC_VER)
#define ARGS(x) x
#else /* Unix: no parameters in prototype! */
#define ARGS(x) ()
#endif
#endif

/* Include function prototypes for G722 operators and type definitions */
//#include "operg722.h"

#ifdef __cplusplus 
extern "C" {
#endif 

#define G722_QMF_TAP_LEN 24

	typedef enum G722_ENCODER_STATUS_t {
		G722_ENCODER_SUCCESS = 0, 
		G722_ENCODER_ALLOC_ERR,
		G722_ENCODER_FRAMESIZE_ERR,
		G722_ENCODER_BUFFER_ERR,
	}G722_ENCODER_STATUS;  

	typedef struct g722Encoder_t{
		int detl;			/* lower band quantizer scaler  */
		int deth; 			/* higher band quantizer scaler */
		int sl; 			/* lower band predictor signal */ 
		int sh; 			/* higher band predictor signal */ 
		int xin[G722_QMF_TAP_LEN];	/* pcm data storage for QMF filter */
		int frameSampleSize;		/* pcm data size in one frame   */
		
		int nbl;
		int nbh;

		int spl ;
		int szl;
		int rlt[3];
		int al[3];
		int apl[3];
		int plt[3];
		int dlt[7];
		int bl[7];
		int bpl[7];
		int sgl[7];

		int sph;
		int szh;
		int rh[3];
		int ah[3];
		int aph[3];
		int ph[3];
		int dh[7];
		int bh[7];
		int bph[7];
		int sg[7];

		int frameCnt;			/* counter of encoded frame 	*/
		int channels;
	}g722Encoder;	


	void g722_reset_encoder(g722Encoder *pEncoder);
	int g722_encode(const short *pcm,unsigned char* data,int frame_size,g722Encoder* encoder); 



	typedef enum G722_DECODER_MODE_t{
		G722_DECODER_64K=0,
		G722_DECODER_56K,
		G722_DECODER_48K,
	}G722_DECODER_MODE; 

	typedef enum G722_DECODER_STATUS_t {
		G722_DECODER_SUCCESS = 0, 
		G722_DECODER_ALLOC_ERR,
		G722_DECODER_FRAMESIZE_ERR,
		G722_DECODER_BUFFER_ERR,
	}G722_DECODER_STATUS;  

	typedef struct g722Decoder_t{
		int detl;			/* lower band quantizer scaler */
		int deth; 			/* higher band quantizer scaler */ 
		int sl; 			/* lower band predictor signal */
		int sh;				/* higher band predictor signal */ 
		int xd[G722_QMF_TAP_LEN>>1];	/* storage for rl-rh */ 
		int xs[G722_QMF_TAP_LEN>>1];	/* storage for rl+rh */
		int frameCnt;  			/* decoder frame count */	

		int nbl ;
		int nbh;
		int spl;
		int szl;
		int rlt [3];
		int al[3];
		int apl[3];
		int plt[3];
		int dlt[7];
		int bl[7];
		int bpl[7];
		int sg[7];

		int sph;
		int szh;
		int rh[3];
		int ah[3];
		int aph[3];
		int ph[3];
		int dh[7];
		int bh[7];
		int bph[7];
		int sgh[7];


		G722_DECODER_MODE mode;		/* G722 decoder mode,64K/56K/48K */ 
		int channels;
	}g722Decoder; 

	void g722_reset_decoder(g722Decoder* pDecoder);
	int g722_decode(const unsigned char* data,short *pcm,G722_DECODER_MODE mode, int enc_size,g722Decoder* decoder); 


#ifdef __cplusplus 
}
#endif 

#endif /* G722_H */
/* ................. End of file g722.h .................................. */
