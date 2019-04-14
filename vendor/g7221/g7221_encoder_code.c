
/*
* C source code for the ITU-T G.722.1 encoder 
*
* Part of the code is based on the reference C code provided with the ITU-T Recommendation G.722.1, 
* downloaded from the official ITU website. Copyrights as indicated in the reference code 
* may still apply.
*
*/

#include "g7221_defs.h"
#ifndef _TMS320C6X
#include "cintrins.h"
#endif

/*
*  G.722.1 encoder main function
*
*  indata - pointer to 320 pcm samples, short format
*  state - pointer to a structure of type encG722_1State, which holds the 320 previous samples, bitrate etc.
*  codewords - pointer to codeword buffer, 60 or 80 bytes
*/

encStatus encG7221Encode(
                         encG722_1State * state,
                         short * indata,
                         short * codewords
                         )
{
  extern short a_dct_core[];
  extern short a_cos_msin_table[];

  unsigned short coded_region_ind[NUMBER_OF_REGIONS];
  unsigned short mlt_code_words[MAX_MLT_WORDS];

  short wrkbuffer[DCT_LENGTH];
  short region_ind[NUMBER_OF_REGIONS];
  short bits_per_coded_region_ind[NUMBER_OF_REGIONS];
  short initial_categorization[NUMBER_OF_REGIONS];  
  short category_balances[NUMBER_OF_CATEGORIZATIONS-1];
  short * mlt_bpcw;
  short * dctoutptr;
  short expo;
  short cat_ctrl;
  short btr;
  short wtr; 
  short offset;
  short bits_used_regpow;
  short available_bits;
  short stuff;
  short outbuf_size;

  int i;
  int number_of_bits_per_frame;


  number_of_bits_per_frame=(state->bitrate)/50; 
  outbuf_size=number_of_bits_per_frame>>4;

  // window, overlap and add
  expo = win_overl_add(indata,state->statebuf,wrkbuffer);

  // copy and add bias
  cpy_addbias(indata,state->statebuf,wrkbuffer);

  // dct
  dctoutptr = indata;                                     // overwrite indata, as it should not be used anymore
  dct(wrkbuffer,dctoutptr,0,a_dct_core,a_cos_msin_table); // dct, 0 in third argument indicates encoder


  // amplitude envelope encode
  bits_used_regpow=crp_enc(dctoutptr,
    expo,
    region_ind,
    bits_per_coded_region_ind,
    coded_region_ind);



  available_bits = number_of_bits_per_frame - 4;
  available_bits -= bits_used_regpow;


  // categorize
  categorize(region_ind,
    available_bits,
    initial_categorization,
    category_balances);


  // adjust rms region power indices
  offset = (expo << 1) + 24;

  for (i=0; i<NUMBER_OF_REGIONS; i++) {
    region_ind[i] += offset - 7;
  }

  reg_ind_just(region_ind, dctoutptr);


  // mlt encode
  mlt_bpcw=wrkbuffer;
  MLT_encode(available_bits,
    expo,
    dctoutptr,	    			
    region_ind,	    
    initial_categorization,
    category_balances,
    mlt_code_words,
    mlt_bpcw,
    &btr,
    &wtr,
    &cat_ctrl);

  // collect all coded information into one frame
  stuff=available_bits-btr;

  make_frame(coded_region_ind,
    bits_per_coded_region_ind,
    mlt_code_words,
    mlt_bpcw,
    wtr,
    stuff,
    outbuf_size,
    cat_ctrl,
    codewords);

  return ENC_OK;	
}


/*
* opens the encoder instance
* 
* state - pointer to a structure of type encG722_1State which contains all 
*		   the state information for one instance of an encoder 
* config - pointer to the configuration structure
*
*/

encStatus encG7221Open(
                       encG722_1State * state,
                       encG722_1Config * config)
{
  encStatus status;

  if ((config->bitrate==24000)||(config->bitrate==32000)) {
    state->bitrate=config->bitrate;
    encG7221Reset(state);
    status=ENC_OK;
  }
  else status=ENC_FAILED;
  return status;
}

/*
* encG7221Close
*/

encStatus encG7221Close(encG722_1State *state)
{
  encG7221Reset (state);
  return ENC_OK;
}

/*
* resets the encoder instance (clears previous frame buffer)
* 
* state - pointer to a structure of type encG722_1State which contains all 
*		   the state information for one instance of an encoder 
*
*/
void encG7221Reset(
                   encG722_1State * state
                   )
{
  int i;
  for (i=0; i<DCT_LENGTH; i++) {
    state->statebuf[i]=0;
  }
}


/*
* sets the encoder state to a new configuration
* 
* state - pointer to structure of type encG722_1State which contains all 
*		   the state information for one instance of an encoder
* config - pointer to the configuration structure
*
*/

encStatus encG7221SetConfig(
                            encG722_1State * state,
                            encG722_1Config * config
                            )
{
  encStatus status;

  if ((config->bitrate==24000)||(config->bitrate==32000)) {
    state->bitrate=config->bitrate;
    status=ENC_OK;
  }
  else status=ENC_FAILED;

  return (status);

}

/*
* gets the encoder configuration
* 
* state - pointer to structure of type encG722_1State which contains all 
*		   the state information for one instance of an encoder
* config - pointer to the configuration structure
*
*/

encStatus encG7221GetConfig(
                            encG722_1State * state,
                            encG722_1Config * config
                            )
{
  config->bitrate=state->bitrate;

  return ENC_OK;

}


/*
* window, overlap and add function
*
* inbuf1 - pointer to new input frame
* inbuf2 - pointer to previous input frame
* outbuf - store pointer output
*
*/
short win_overl_add(
                    short * inbuf1,
                    short * inbuf2,
                    short * outbuf
                    )
{
  short * ptr0;
  short * ptr1;
  short * ptr2;
  short * ptr4;
  short * ptr5;
  short cnt0;
  short tmp0;
  short tmp1;
  short tmp2;
  int   prod;
  int   sum;
  int   maxv;
  int   itmp0;

  extern short samples_to_rmlt_window[];

  // copy output pointer

  ptr2 = outbuf;

  // do old data

  ptr0 = inbuf2 + (DCT_LENGTH/2-1);
  ptr1 = inbuf2 + (DCT_LENGTH/2);

  ptr4 = samples_to_rmlt_window + (DCT_LENGTH/2-1);
  ptr5 = samples_to_rmlt_window + (DCT_LENGTH/2);

  cnt0 = (DCT_LENGTH/2);
  do {

    tmp0 = *ptr0;
    ptr0--;
    tmp1 = *ptr4;
    ptr4--;

    sum  = _smpy(tmp0,tmp1);

    tmp0 = *ptr1;
    ptr1++;
    tmp1 = *ptr5;
    ptr5++;

    prod = _smpy(tmp0,tmp1);

    sum  = _sadd(sum,prod);

    sum  = _sadd(sum,0x8000);

    tmp2 = sum >> 16;

    *ptr2 = tmp2;
    ptr2++;

    cnt0--;

  } while (cnt0);


  // do new data

  ptr0 = inbuf1;
  ptr1 = inbuf1+(DCT_LENGTH-1);

  ptr4 = samples_to_rmlt_window + (DCT_LENGTH-1);
  ptr5 = samples_to_rmlt_window;

  cnt0 = (DCT_LENGTH/2);
  do {

    tmp0 = *ptr0;
    ptr0++;
    tmp1 = *ptr4;
    ptr4--;

    sum  = _smpy(tmp0,tmp1);

    tmp0 = *ptr1;
    ptr1--;
    tmp1 = *ptr5;
    ptr5++;

    prod = _smpy(tmp0,tmp1);

    prod = -prod;

    sum  = _sadd(sum,prod);

    sum  = _sadd(sum,0x8000);

    tmp2 = sum >> 16;

    *ptr2 = tmp2;
    ptr2++;

    cnt0--;

  } while (cnt0);

  // calculate how many bits to shift up the input to the DCT

  ptr2 = outbuf;
  maxv = 0;
  sum  = 0;

  cnt0 = DCT_LENGTH;
  do {

    tmp0 = *ptr2;
    ptr2++;
    tmp0 = (short)_abs(tmp0);
    if (tmp0 > maxv) maxv = (int)tmp0;
    sum += tmp0;
    cnt0--;

  } while (cnt0);

  // test max value

  tmp0  = (short)_norm(maxv);
  tmp0  = tmp0 - 17;
  itmp0 = 14000 >> tmp0;
  if (maxv < itmp0) tmp0 += 1;
  if (tmp0 > 9) tmp0 = 9;
  if (tmp0 < 0) tmp0 = 0;

  // test sum

  itmp0 = sum >> 7;
  if (maxv < itmp0) tmp0 -= 1;

  // now shift

  tmp1 = -tmp0;
  ptr2 = outbuf;    

  cnt0 = DCT_LENGTH;
  do {

    tmp2 = *ptr2;
    if (tmp0 > 0) tmp2 = (short)_sshl(tmp2,tmp0);
    if (tmp0 < 0) tmp2 = tmp2 >> tmp1;
    *ptr2 = tmp2;
    ptr2++;
    cnt0--;

  } while (cnt0);

  return tmp0;
}


/*
* copy new samples to old samples frame (for next time)
* and add bias to dct input
*
* indata    - pointer to new frame
* oldindata - pointer to previous frame, will be overwritten
* wrkbuffer - dct input, bias will be added
*
*/
void cpy_addbias(
                 short * indata,
                 short * oldindata,
                 short * wrkbuffer
                 )
{
  int i;
  int tmp;

  extern signed char abias[];

  for (i=0; i<DCT_LENGTH; i++) {
    oldindata[i] = indata[i];         // copy indata to state for next call
    tmp = wrkbuffer[i]+abias[i];
    if (tmp >  32767) tmp = 32767;
    if (tmp < -32768) tmp = -32768;
    wrkbuffer[i] = (short)tmp;        // add bias offset to windowed data
  }
}    

/*
* compute region power indices (quantized region powers) and encode 
* in number_of_regions=14 regions, 20 coeffs per region 
* 
* inbuf - pointer to DCT output data, 320 shorts
* exponent - the inputs to the DCT are shifted with exponent
* rpi - pointer to 14 region power indices, short format
* bits_per_codeword - number of bits used for each codeword
* coded - pointer to the 14 coded indices, unsigned short format 
*
* returns number of bits used to encode region power indices
*
*/

short crp_enc(
              short * inbuf,
              short exponent,
              short * rpi,
              short * bits_per_codeword,
              unsigned short * coded
              )
{
  extern unsigned char differential_region_power_bits[NUMBER_OF_REGIONS][NUMBER_OF_LEVELS];
  extern unsigned short differential_region_power_codes[NUMBER_OF_REGIONS][NUMBER_OF_LEVELS];

  short * ptr0;
  short tmp_ind[NUMBER_OF_REGIONS];
  short diff_ind[NUMBER_OF_REGIONS];

  int tmp0;	
  int tmp1;
  int tmp2;
  int nrshift;
  int ind;
  int i;
  int r;

  ptr0=inbuf;

  // compute region powers and quantize
  for (r=0; r<NUMBER_OF_REGIONS; r++) {		
    tmp0=0;
    for (i=0; i<COEFS_PER_REGION; i++) {
      tmp0 +=	(*ptr0) * (*ptr0);
      ptr0++;		
    }
    tmp1=_norm(tmp0);
    tmp2=exponent<<1;
    nrshift=16-tmp1;
    ind=33-tmp1;
    ind-=tmp2;


    if (nrshift>0) tmp0=tmp0>>nrshift;
    else if (nrshift<0) {
      nrshift=_abs(nrshift);
      tmp0=tmp0<<nrshift;
    }
    if (r==0) {
      if (tmp0>28963) ind+=1;

      if (ind<1) ind=1;
      else if (ind>31) ind=31;

    }
    else { 
      if (tmp0>28963) ind+=1;
    }

    if (ind<-8) ind=-8;
    else if (ind>40) ind=40;
    tmp_ind[r]=(short)ind;
  }

  //adjust power indices before encoding
  for (r=NUMBER_OF_REGIONS-2; r>=0; r--) {
    tmp2=tmp_ind[r+1]-11;
    if (tmp_ind[r]<tmp2) tmp_ind[r]=(short)tmp2;
  }
  for (r=1; r<NUMBER_OF_REGIONS; r++) {
    tmp2=tmp_ind[r]-tmp_ind[r-1];
    if (tmp2<-12) {
      tmp2=-12;
      tmp_ind[r]=tmp_ind[r-1]+(short)tmp2;
    }
    diff_ind[r]=(short)tmp2;
    rpi[r]=tmp_ind[r];
  }
  rpi[0]=tmp_ind[0];

  //encode differential power indices
  coded[0]=(unsigned short)tmp_ind[0];

  bits_per_codeword[0]=5;
  tmp1=5;
  for (r=1; r<NUMBER_OF_REGIONS; r++) {
    tmp0=diff_ind[r]+12;
    coded[r]=(unsigned short)differential_region_power_codes[r][tmp0];
    bits_per_codeword[r]=differential_region_power_bits[r][tmp0];
    tmp1+=bits_per_codeword[r];

  }
  return (short)tmp1;
}

/*
* mlt quantizer/encoder function
* 
* outputs quantized and coded mlt-coefficients and categorization control bits
* for the chosen categorization
* 
* available_bits - remaining bits after envelope-encoding and categorization
* expo - DCT magnitude shift
* mlt - pointer to MLT-coefs
* rms_index_arr - pointer to region power indices
* categorization - pointer to initial categorization for the current frame
* cat_bal - pointer to category balances for the current frame
* code_words - pointer to quantized and Huffman coded info
* bpcw - pointer to number of bits per codeword
* btr - pointer to total bitcount
* wtr - pointer to total wordcount
* cat_ctrl - pointer to the selected categorization
*/

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

                )
{
  extern short  vd_arr[NUM_CATEGORIES-1];
  extern short  vpr_arr[NUM_CATEGORIES-1];

  short  cat_no;
  short  reg_no;
  short  category;
  short  vd;
  short  vpr;
  short  rms_index;
  short  * mlt_selection;
  unsigned short  region_mlt_coded[NUMBER_OF_REGIONS][COEFS_PER_REGION];
  short  region_mlt_bcnt[NUMBER_OF_REGIONS][COEFS_PER_REGION];
  short  bits_in_reg[NUMBER_OF_REGIONS];
  short  total_mltbits_used;
  short  fewtest;
  short  manytest;

  int i, r, m;

  //start in middle of categorization range
  for (i=0; i<(NUMBER_OF_CATEGORIZATIONS/2); i++) {
    categorization[cat_bal[i]] += 1;
  }

  cat_no = (NUMBER_OF_CATEGORIZATIONS/2);

  *btr = 0;
  *wtr = 0;

  // complete coding of all regions in middle categorization 
  for (reg_no=0; reg_no<NUMBER_OF_REGIONS; reg_no++) {

    category  = categorization[reg_no];

    if (category < 7) { // category 7 allocated no bits for transmission 	

      // store the mlt-coefs for the given region 
      mlt_selection = &mlt[COEFS_PER_REGION*reg_no];

      rms_index = rms_index_arr[reg_no];
      vd        = vd_arr[category];
      vpr       = vpr_arr[category];

      vector_index_encode(category,
        expo,
        available_bits,
        reg_no,
        rms_index,
        vd,
        vpr,
        mlt_selection,
        region_mlt_coded,
        region_mlt_bcnt,
        bits_in_reg);
    }

    else if (category == 7) {
      bits_in_reg[reg_no] = 0;
    }		
  }

  total_mltbits_used = 0;

  for (i=0; i<NUMBER_OF_REGIONS; i++) {
    total_mltbits_used += bits_in_reg[i];
  }

  //testing if too few or to many bits have been used

  // if too few are used
  fewtest = available_bits - total_mltbits_used;

  while ((fewtest > 0) && (cat_no > 0)) {

    //new categorization
    cat_no--;
    categorization[cat_bal[cat_no]]--;
    category = categorization[cat_bal[cat_no]];		

    //code the changed region again
    reg_no = cat_bal[cat_no];
    total_mltbits_used -= bits_in_reg[reg_no];

    if (category < 7) { // category 7 allocated no bits for transmission 	

      // store the mlt-coefs for the given region 
      mlt_selection = &mlt[COEFS_PER_REGION*reg_no];

      rms_index = rms_index_arr[reg_no];
      vd        = vd_arr[category];
      vpr       = vpr_arr[category];

      vector_index_encode(category,
        expo,
        available_bits,
        reg_no,
        rms_index,
        vd,
        vpr,
        mlt_selection,
        region_mlt_coded,
        region_mlt_bcnt,
        bits_in_reg);		
    }

    else if (category == 7) {
      bits_in_reg[reg_no] = 0;
    }

    total_mltbits_used += bits_in_reg[reg_no];
    fewtest             = available_bits - total_mltbits_used;
  }


  // if too many are used
  manytest = available_bits - total_mltbits_used;

  while ((manytest < 0) && (cat_no < 15)) {

    //new categorization
    cat_no++;
    categorization[cat_bal[cat_no-1]]++;
    category = categorization[cat_bal[cat_no-1]];		

    //code the changed region again
    reg_no = cat_bal[cat_no-1];
    total_mltbits_used -= bits_in_reg[reg_no];

    if (category < 7) { // category 7 allocated no bits for transmission 	

      // store the mlt-coefs for the given region 
      mlt_selection = &mlt[COEFS_PER_REGION*reg_no];

      rms_index = rms_index_arr[reg_no];
      vd        = vd_arr[category];
      vpr       = vpr_arr[category];

      vector_index_encode(category,
        expo,
        available_bits,
        reg_no,
        rms_index,
        vd,
        vpr,
        mlt_selection,
        region_mlt_coded,
        region_mlt_bcnt,
        bits_in_reg);		
    }

    else if (category == 7) {
      bits_in_reg[reg_no] = 0;
    }

    total_mltbits_used += bits_in_reg[reg_no];
    manytest            = available_bits - total_mltbits_used;

  }

  // store the chosen codewords and categorization
  m=0;
  for (i=0; i<NUMBER_OF_REGIONS; i++) {		
    if (bits_in_reg[i] > 0) {
      for (r=0; r<COEFS_PER_REGION; r++) {				
        if (region_mlt_bcnt[i][r] > 0) {
          code_words[m] = region_mlt_coded[i][r];
          bpcw[m]       = region_mlt_bcnt[i][r];
          m++;
        }
      }
    }
  }

  *cat_ctrl = cat_no;
  *btr      = total_mltbits_used;
  *wtr      = m;
}

/*
* vector_index_encode
* 
* computes and Huffman-encodes the vector indices for a given region
*
* category - the category with which to code the given region
* expo - DCT magnitude shift
* available_bits - remaining bits after envelope-encoding and categorization
* reg_no - region index
* rms_index - region power index
* vd - vector dimension
* vpr - number of vectors per region
* mlt_selection - pointer to MLT-coefs for the given region
* region_mlt_coded - array containing the selected Huffman codewords
* region_mlt_bcnt - array containing number of of bits representing each codeword
* bits_in_reg  - pointer to number of codebits per region
*/

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
                         )
{	
  extern short  quant_rms_inv_arr[64];
  extern short  stepsize_inv_arr[NUM_CATEGORIES-1];
  extern short  deadzone_rounding_arr[NUM_CATEGORIES-1];
  extern short  int_dead_zone_low_bits[NUM_CATEGORIES-1];
  extern short  kmax_arr[NUM_CATEGORIES-1];

  extern unsigned char mlt_sqvh_bitcount_category_0[196];
  extern unsigned char mlt_sqvh_bitcount_category_1[100];
  extern unsigned char mlt_sqvh_bitcount_category_2[49];
  extern unsigned char mlt_sqvh_bitcount_category_3[625];
  extern unsigned char mlt_sqvh_bitcount_category_4[256];
  extern unsigned char mlt_sqvh_bitcount_category_5[243];
  extern unsigned char mlt_sqvh_bitcount_category_6[32];

  extern unsigned short  mlt_sqvh_code_category_0[196];
  extern unsigned short  mlt_sqvh_code_category_1[100];
  extern unsigned short  mlt_sqvh_code_category_2[49];
  extern unsigned short  mlt_sqvh_code_category_3[625];
  extern unsigned short  mlt_sqvh_code_category_4[256];
  extern unsigned short  mlt_sqvh_code_category_5[243];
  extern unsigned short  mlt_sqvh_code_category_6[32];

  short  k_vect[COEFS_PER_REGION];
  short  sign_count_arr[MAX_VPR];
  short  v_ind_arr[MAX_VPR];
  short  sign_arr[MAX_VPR];
  short  vec_tmp, vector_index;
  short  x,k,mytemp,myacca;
  short  sign_bits[COEFS_PER_REGION];
  short  k_val=kmax_arr[category];
  short  k_val_plus_one = k_val+1;
  short  low_dead = int_dead_zone_low_bits[category];
  short  dead = deadzone_rounding_arr[category];

  unsigned short * sel_cat_cw = NULL;
  unsigned char * sel_cat_bc = NULL;

  int    tmp_xk,n,j,r,i_cnt,sign_count;

  n = (int) available_bits; //DUMMY  for warning
  n = (int) expo; //DUMMY  for warning

  /* stores all sign_bits for the mlt-coefficients in an arr */
  for (n=0; n<COEFS_PER_REGION; n++) { 
    if (mlt_selection[n] >= 0) {
      sign_bits[n] = 1;
    }
    else {
      sign_bits[n] = 0;
    }
  }

  // computation of quantization indices k()
  tmp_xk   = stepsize_inv_arr[category] * quant_rms_inv_arr[rms_index];
  tmp_xk  += 4096;
  tmp_xk >>= 13;
  mytemp   = tmp_xk & 0x3;
  tmp_xk >>= 2;

  x = (short)(tmp_xk);

  for (i_cnt=0; i_cnt<COEFS_PER_REGION; i_cnt++) {

    tmp_xk   = _abs(mlt_selection[i_cnt]) * x;
    myacca   = (short)_abs(mlt_selection[i_cnt]) * mytemp;
    myacca  += low_dead;
    myacca >>= 2;
    tmp_xk  += dead;
    tmp_xk  += myacca;
    tmp_xk >>= 13;

    k = (short)(tmp_xk);

    if (k < k_val) { // k() is limited to kmax for the given region
      k_vect[i_cnt] = k;
    }
    else {
      k_vect[i_cnt] = k_val;
    }
  }

  // combining k()'s into vector-indices
  for (n=0; n<vpr; n++) {
    vector_index = 0;
    j            = 0;
    sign_count   = 0;
    sign_arr[n]  = 0;

    while (j<vd) { // compute all vectors in a region 

      vec_tmp = k_vect[n*vd+j];

      if (vd-(j+1)>=1)
        vec_tmp *= k_val_plus_one;
      if (vd-(j+1)>=2)
        vec_tmp *= k_val_plus_one;
      if (vd-(j+1)>=3)
        vec_tmp *= k_val_plus_one;
      if (vd-(j+1)>=4)
        vec_tmp *= k_val_plus_one;

      vector_index += vec_tmp;

      if (k_vect[n*vd+j] > 0) {				
        sign_arr[n] += (sign_bits[n*vd+j] << (14-sign_count));
        sign_count++;
        j++;
      }

      else {
        j++;
      }
    }

    if (sign_count == 0) {  // store sign-info to be transmitted along with code_vectors 
      sign_arr[n]       = -100;
      sign_count_arr[n] = -100;
    }
    else if (sign_count > 0) {
      sign_arr[n]     >>= (15 - sign_count);
      sign_count_arr[n] = sign_count;
    }		

    v_ind_arr[n] = vector_index;		
  }

  // pointer to lookup-tables for codewords and bitcount 

  if (category == 0) {
    sel_cat_cw  = mlt_sqvh_code_category_0;
    sel_cat_bc  = mlt_sqvh_bitcount_category_0;
  }

  else if (category == 1) {
    sel_cat_cw  = mlt_sqvh_code_category_1;
    sel_cat_bc  = mlt_sqvh_bitcount_category_1;
  }

  else if (category == 2) {
    sel_cat_cw  = mlt_sqvh_code_category_2;
    sel_cat_bc  = mlt_sqvh_bitcount_category_2;
  }

  else if (category == 3) {
    sel_cat_cw  = mlt_sqvh_code_category_3;
    sel_cat_bc  = mlt_sqvh_bitcount_category_3;
  }

  else if (category == 4) {
    sel_cat_cw  = mlt_sqvh_code_category_4;
    sel_cat_bc  = mlt_sqvh_bitcount_category_4;
  }

  else if (category == 5) {
    sel_cat_cw  = mlt_sqvh_code_category_5;
    sel_cat_bc  = mlt_sqvh_bitcount_category_5;
  }

  else if (category == 6) {
    sel_cat_cw  = mlt_sqvh_code_category_6;
    sel_cat_bc  = mlt_sqvh_bitcount_category_6;
  }	

  else {
    assert(0);
  }

  // insert vector-codewords and signbits into curr_cw, keep track of bitcount 

  r			        = 0;
  bits_in_reg[reg_no] = 0;

  for (n=0; n<vpr; n++) {

    mytemp = v_ind_arr[n];
    region_mlt_coded[reg_no][r]   = sel_cat_cw[mytemp];
    region_mlt_bcnt[reg_no][r]    = sel_cat_bc[mytemp];
    bits_in_reg[reg_no]			 += sel_cat_bc[mytemp];
    r++;

    if (sign_count_arr[n] > 0) { /* put sign-bits in curr_cw, and update bitcount */
      region_mlt_coded[reg_no][r]  = sign_arr[n];
      region_mlt_bcnt[reg_no][r]   = sign_count_arr[n];
      bits_in_reg[reg_no]         += sign_count_arr[n];			
      r++;
    }

  }

  for (n=r; n<20; n++) {
    region_mlt_bcnt[reg_no][n]    = 0;
  }
}  



/*
* Outputs the final compacted bitstream in output
*/

void make_frame(unsigned short * coded_pow_ind,
                short * bits_per_pow_ind,
                unsigned short * coded_mlt,
                short * bits_per_coded_mlt,
                short num_mlt_words,
                short stuff_count,
                short words_to_fill,
                short cat_control_bits,
                short * output
                )
{
  unsigned short tmp;
  unsigned short shifted;
  unsigned short current_word=0;

  short * ptr0;

  int avail=16;
  int leftover;
  int r;
  int shift;
  int count=0;



  // diff region pow indices
  ptr0=output;
  for(r=0;r<NUMBER_OF_REGIONS;r++) {
    shift=avail-bits_per_pow_ind[r];
    if (shift>0) {
      shifted=coded_pow_ind[r]<<shift;
      current_word=current_word|shifted;
      avail=avail-bits_per_pow_ind[r];
    }
    else {
      leftover=_abs(shift);
      shifted=coded_pow_ind[r]>>leftover;
      current_word=current_word|shifted;
      *ptr0++=current_word;
      count++;
      avail=16-leftover;
      current_word=coded_pow_ind[r]<<avail;
    }

  }

  // categorization control bits
  shift=avail-4;
  if (shift>0) {
    shifted=cat_control_bits<<shift;
    current_word=current_word|shifted;
    avail=avail-4;
  }
  else {
    leftover=_abs(shift);
    shifted=cat_control_bits>>leftover;
    current_word=current_word|shifted;
    *ptr0++=current_word;
    count++;
    avail=16-leftover;
    current_word=cat_control_bits<<avail;
  }

  // mlt codewords
  for(r=0;r<num_mlt_words;r++) {

    shift=avail-bits_per_coded_mlt[r];
    if (shift>0) {
      shifted=coded_mlt[r]<<shift;
      current_word=current_word|shifted;
      avail=avail-bits_per_coded_mlt[r];
    }
    else {
      leftover=_abs(shift);
      shifted=coded_mlt[r]>>leftover;
      current_word=current_word|shifted;
      *ptr0++=current_word;
      count++;
      avail=16-leftover;
      current_word=coded_mlt[r]<<avail;
    }
    if (count==words_to_fill) break;

  }

  // bitstuff
  if (stuff_count>0) {
    tmp=stuff_count % 16;
    tmp=1<<tmp;
    tmp-=1;
    current_word=current_word|tmp;
    if(tmp) {
      *ptr0++=current_word;
      r=stuff_count/16;
    }
    else { 
      r=stuff_count/16-1;
      *ptr0++=-1;
    }
    while (r) {
      *ptr0++=-1;
      r--;

    }
  }

}

/*
* reg_ind_just
* 
* adjusts DCT-coefs and absolute region power index
*
* reg_ind - pointer to computed rms region power index
* dctoutptr - pointer to computed DCT-coefs
*/

void reg_ind_just(short *reg_ind,short *dctoutptr)

{
  int n,i;
  short reg_no;

  int   atmp;
  short temp;

  for (reg_no=0; reg_no<NUMBER_OF_REGIONS; reg_no++)
  {
    n = reg_ind[reg_no] - 39;
    n = n >> 1;

    if (n > 0) {

      for (i=0; i<COEFS_PER_REGION; i++) {

        atmp = dctoutptr[reg_no*COEFS_PER_REGION + i] << 16;
        atmp = atmp + 32768;
        atmp = atmp >> n;
        atmp = atmp >> 16;

        dctoutptr[reg_no*COEFS_PER_REGION + i] = (short)(atmp);				
      }

      temp = n << 1;
      reg_ind[reg_no] -= temp;       
    }
  }
}



