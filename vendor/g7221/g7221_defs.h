
#include "g7221.h"

#define NUMBER_OF_REGIONS 14
#define COEFS_PER_REGION 20
#define MAX_NUMBER_OF_BITS 640
#define NUMBER_OF_CATEGORIZATIONS 16
#define MAX_VPR 10
#define MAX_VD 5
#define NUM_CATEGORIES 8
#define NUMBER_OF_LEVELS 24
#define HALF_COEFS_PER_REGION 10
#define MAX_MLT_WORDS 280

typedef short tree_arr[2];

typedef struct {
	short words_read;
	short bits_read;
	short bits_per_frame;
	short * read_pointer;
	short category;
}huffState;

short get_next_bit(
	huffState * state
);

short huff_dec(
	huffState * state,
	short mode
);

void dct(
	short * in_buffer,
	short * out_buffer,
	short dct_type,
	short * dct_core_adr,
	short * cos_msin_table_adr
);

void MLT_encode(
	short  available_bits,
	short  expo,
	short  * mlt,	    			
	short  * rms_index_arr,	    
	short  * categorization,
	short  * cat_bal,
	unsigned short  * code_words,
	short  * bpcw,
	short  * btr,
	short  * wtr,
	short  * cat_ctrl	
);

void vector_index_encode(
	short  category,
	short  expo,
	short  available_bits,
	short  reg_no,
	short  rms_index,
	short  vd,
	short  vpr,
	short  * mlt_selection,
	unsigned short  region_mlt_coded[NUMBER_OF_REGIONS][COEFS_PER_REGION],
	short  region_mlt_bcnt[NUMBER_OF_REGIONS][COEFS_PER_REGION],
	short  * bits_in_reg
);

void categorize(
	short * pow_ind,
	short avail_bits,
	short * init_cat,
	short * cat_balance
);

short crp_enc(
	short * inbuf,
	short exponent,
	short * rpi,
	short * bits_per_codeword,
	unsigned short * coded
);

void make_frame(
	unsigned short * coded_pow_ind,
	short * bits_per_pow_ind,
	unsigned short * coded_mlt,
	short * bits_per_coded_mlt,
	short num_mlt_words,
	short stuff_count,
	short outbuf_size,
	short cat_control_bits,
	short * output
);

void reg_ind_just(
	short *reg_ind,
	short *dctoutptr
);

short win_overl_add(
	short * inbuf1,
	short * inbuf2,
	short * outbuf
);

void cpy_addbias(
	short * indata,
	short * oldindata,
	short * wrkbuffer
);

void get_reg_levels(
	short *region_index,
	short *expo,
	short *reg_level
);

void mlt_decode(
	decG722_1State * dec_state,
	huffState * huff_state,
	short * categorization,
	short * rms_level_arr,
	short * rec_mlt_coefs
);

void vector_dec(
	huffState * state,
	short * sign_overflow,
	short vector_index,
	short category,
	short vd,
	short * k_vect,
	short * sign_counter
);

void mlt_reconstruct(
	short * k_vect,
	short * sign_bits,
	short rms_level,
	short category,
	short vd,
	short * rec_mlt_ptr
);

void noise_fill(
	short category,
	short rms_level,
	short * noise_filled_mlt
);

short rand_sign(
	Rand_Obj *randobj
);

void getoutbuf(
	decG722_1State * state,
	short * current_samples,
	short * outbuf
);


void adjust_dct(
	short expo,
	short *current_samples
);
