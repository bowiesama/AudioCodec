
/*
* C source code for the ITU-T G.722.1 decoder
*
* Part of the code is based on the reference C code provided with the ITU-T Recommendation G.722.1, 
* downloaded from the official ITU website. Copyrights as indicated in the reference code 
* may still apply.
*
*/

#include "g7221_defs.h"

/*
* G.722.1 decoder main function
*
* state - pointer to a structure of type decG722_1State; 
*		   holds state information for one decoder instance
* inbuf - pointer to one frame of 30 or 40 words, short format 
* outbuf - pointer to 320 decoded audio samples, short format
* fe_flag - frame erasure indicator
*/

decStatus decG7221Decode(
                         decG722_1State * state,
                         short * inbuf,
                         short * outbuf,
                         short fe_flag
                         )
{

  extern short s_dct_core[];
  extern short s_cos_msin_table[];

  short expo;
  short available_bits;
  short cat_num;
  short cat_cnt;
  short rms_level_arr[NUMBER_OF_REGIONS];
  short initial_categorization[NUMBER_OF_REGIONS];
  short category_balances[NUMBER_OF_CATEGORIZATIONS];
  short rec_mlt_coefs[DCT_LENGTH];
  short region_ind[NUMBER_OF_REGIONS]; 
  short current_samples[DCT_LENGTH];

  int r;

  huffState huff0_state;

  if (!fe_flag) {

    //Initialize huffState
    huff0_state.bits_read=0;
    huff0_state.words_read=0;
    huff0_state.read_pointer=inbuf;
    huff0_state.bits_per_frame=(short)((state->bitrate)/50);


    //Region power decode
    region_ind[0]=huff_dec(&huff0_state,5);

    for (r=1;r<NUMBER_OF_REGIONS;r++) {
      region_ind[r]=huff_dec(&huff0_state,6);
      region_ind[r]=(short)(region_ind[r]+region_ind[r-1]);
      region_ind[r]-=12;

      if ((region_ind[r]<-8)||(region_ind[r]>31)){
        fe_flag = 1;
        break;
      }
    }



    //Extract categorization number 
    cat_num=huff_dec(&huff0_state,4);


    if ((cat_num >= 0) && (cat_num <= 15) && (!fe_flag)) {

      //Categorize
      available_bits=(short)(huff0_state.bits_per_frame-huff0_state.bits_read);
      categorize(region_ind,
        available_bits,
        initial_categorization,
        category_balances);

      //Set categories according to categorization control
      cat_cnt = 0;
      while (cat_cnt < cat_num) {
        initial_categorization[category_balances[cat_cnt]]++;
        cat_cnt++;
      }

      //Find magnitude shift and rms power level for each region
      get_reg_levels(region_ind,
        &expo,
        rms_level_arr);


      //Reconstruction of mlt-coefs
      mlt_decode(state,
        &huff0_state,
        initial_categorization,
        rms_level_arr,
        rec_mlt_coefs);

      //Inverse type IV dct
      dct(rec_mlt_coefs,current_samples,1,s_dct_core,s_cos_msin_table);


      //Adjustment of dct-coefs
      adjust_dct(expo,current_samples);


      //Window-overlap-add function
      getoutbuf(state,current_samples,outbuf);


      state->fe_buf_used=0;

    }

    else {
      expo=0;
      for(r=0;r<DCT_LENGTH;r++) {
        rec_mlt_coefs[r]=0;
      }

      //Inverse type IV dct
      dct(rec_mlt_coefs,current_samples,1,s_dct_core,s_cos_msin_table);

      //Adjustment of dct-coefs
      adjust_dct(expo,current_samples);

      //Window-overlap-add function
      getoutbuf(state,current_samples,outbuf);

      state->fe_buf_used=0;
    }
  }

  else if (fe_flag && !(state->fe_buf_used)) {
    for(r=0;r<HALF_DCT_LENGTH;r++) {
      current_samples[r]=state->fe_buf[r];
      current_samples[r+160]=state->statebuf[r];
    }
    getoutbuf(state,current_samples,outbuf);
    state->fe_buf_used = 1;
  }

  else if (fe_flag && state->fe_buf_used) {
    expo=0;

    for(r=0;r<DCT_LENGTH;r++) {
      rec_mlt_coefs[r]=0;
    }
    //Inverse type IV dct
    dct(rec_mlt_coefs,current_samples,1,s_dct_core,s_cos_msin_table);

    //Adjustment of dct-coefs
    adjust_dct(expo,current_samples);

    //Window-overlap-add function
    getoutbuf(state,current_samples,outbuf);
  }


  /* For ITU testing, off the 2 lsbs. */
  for (r=0; r<DCT_LENGTH; r++)
    outbuf[r] &= 0xfffc;

  return DEC_OK;
}

/*
* opens the decoder instance
* 
* state - pointer to a structure of type decG722_1State
* config - pointer to the configuration structure
*
*/

decStatus decG7221Open(
                       decG722_1State * state,
                       decG722_1Config * config
                       )
{
  decStatus status;

  if ((config->bitrate==24000)||(config->bitrate==32000)) {
    state->bitrate=config->bitrate;
    decG7221Reset(state);
    status=DEC_OK;
  }
  else status=DEC_FAILED;
  return status;
}

/*
* decG7221Close
*/

decStatus decG7221Close(decG722_1State *state)
{
  if (state)
    return DEC_OK;

  return DEC_OK;
}
/*
* resets the encoder instance - clears previous IMLT outputs and initializes 
*		   the random generator. 
* 
* state - pointer to a structure of type decG722_1State
*
*/

void decG7221Reset(
                   decG722_1State * state
                   )
{
  int i;
  for (i=0; i<HALF_DCT_LENGTH; i++) {
    state->statebuf[i]=0;
    state->fe_buf[i]=0;
  }
  state->fe_buf_used=0;
  state->rand_obj.seed0=1;
  state->rand_obj.seed1=1;
  state->rand_obj.seed2=1;
  state->rand_obj.seed3=1;
}

/*
* sets the decoder state to new configuration (use if change of bitrate)
* 
* state - pointer to a structure of type decG722_1State
* config - pointer to the configuration structure
*
*/

decStatus decG7221SetConfig(
                            decG722_1State * state,
                            decG722_1Config * config
                            )
{
  decStatus status;

  if ((config->bitrate==24000)||(config->bitrate==32000)) {
    state->bitrate=config->bitrate;
    status=DEC_OK;
  }
  else status=DEC_FAILED;
  return status;
}

/*
* gets the decoder configuration 
* 
* state - pointer to a structure of type decG722_1State
* config - pointer to the configuration structure
*
*/

decStatus decG7221GetConfig(
                            decG722_1State * state,
                            decG722_1Config * config
                            )
{
  config->bitrate=state->bitrate;

  return DEC_OK;
}


/*  
* function to read one bit at the time 
*
* state - pointer to a structure of type huffState
*/

short get_next_bit(
                   huffState * state
                   )
{
  int tmp;
  int tmp2;
  int tmp1=*state->read_pointer;
  tmp=(state->bits_read++)%16;
  if (tmp<15) tmp2=(tmp1>>(15-tmp)&1);
  else {
    tmp2=tmp1&1; 
    state->read_pointer++;
  }
  return (short)tmp2;

}


/* 
* huffman decode - returns one index  
*	
* state - pointer to a structure of type huffState
* mode 6 - returns one (differential) region power index
* mode 7 - returns one SQVH index  
* mode 1-5 - returns 1-5 next bits 
*	
*/

short huff_dec(huffState * state, short mode
               )

{
  extern signed char differential_region_power_decoder_tree[NUMBER_OF_REGIONS][23][2];

  extern short mlt_decoder_tree_category_0[180][2];
  extern short mlt_decoder_tree_category_1[93][2];
  extern short mlt_decoder_tree_category_2[47][2];
  extern short mlt_decoder_tree_category_3[519][2];
  extern short mlt_decoder_tree_category_4[208][2];
  extern short mlt_decoder_tree_category_5[191][2];
  extern short mlt_decoder_tree_category_6[31][2];

  short index=0;
  short current_bit;
  short current_word;

  int tmp;

  tree_arr *ta;

  // get envelope codewords  
  if(mode==6) {
    tmp=0;
    do {	
      current_bit=get_next_bit(state);
      if(current_bit==0) 
        index=differential_region_power_decoder_tree[state->words_read][index][0];
      else index=differential_region_power_decoder_tree[state->words_read][index][1];
      tmp++;
    }
    while ((index>0)&&(tmp<16));

    state->words_read++;
    if(index<=0)
      return (short)(-index);
    else
      return -99; //illegal codeword
  }	

  //get mlt codewords  
  else if (mode==7) {
    if (state->category==0) ta=mlt_decoder_tree_category_0;
    else if (state->category==1) ta=mlt_decoder_tree_category_1;
    else if (state->category==2) ta=mlt_decoder_tree_category_2;
    else if (state->category==3) ta=mlt_decoder_tree_category_3;
    else if (state->category==4) ta=mlt_decoder_tree_category_4;
    else if (state->category==5) ta=mlt_decoder_tree_category_5;
    else if (state->category==6) ta=mlt_decoder_tree_category_6;
    else return -2;  // illegal category
    do {
      current_bit=get_next_bit(state);
      if(current_bit==0) index=ta[index][0];
      else index=ta[index][1];
      if((state->bits_read==state->bits_per_frame)&&(index>0)) return -1; //"insufficient bits"
    }
    while (index>0);

    state->words_read++;
    return (short)(-index);

  }

  //get 4 category bits or first region index, 5 bits 
  else if ((mode>0)&&(mode<=5)) {
    current_word=0;
    while (mode) {
      current_bit=get_next_bit(state);
      current_word=(short)(current_word<<1);
      current_word=(short)(current_word|current_bit);
      mode--;
    }
    state->words_read++;
    return current_word;
  }
  else return -3; //illegal mode

}

/*
* decoding of mlt coefficients
* 
* dec_state - pointer to a structure of type decG722_1State
* huff_state - pointer to a structure of type huffState
* categorization - pointer to chosen categorization for current frame
* rms_level_array - pointer to rms power level for all regions
* rec_mlt_coefs - pointer to recovered mlt-coefs
*/

void mlt_decode(decG722_1State * dec_state,
                huffState * huff_state,
                short * categorization,
                short * rms_level_array,
                short * rec_mlt_coefs
                )
{
  extern short vd_arr[7];
  extern short vpr_arr[7];

  short vd;
  short vpr;
  short vector_index;
  short category;
  short k_vect[MAX_VD];
  short sign_bits[MAX_VD];
  short rms_level;
  short noise_filled_mlt;
  short * rec_mlt_ptr;
  short tester;
  short sign_overflow = 0;

  int i;
  int vpr_cnt;
  int reg_no  = 0;	

  //decoding of all regions
  while (reg_no<NUMBER_OF_REGIONS) {

    category  = categorization[reg_no];
    rms_level =	rms_level_array[reg_no];

    if (category<7) {

      vd	 = vd_arr[category];
      vpr	 = vpr_arr[category];
      huff_state->category = category;

      for (vpr_cnt=0; vpr_cnt<vpr; vpr_cnt++) {

        if (huff_state->bits_read < huff_state->bits_per_frame) {
          vector_index = huff_dec(huff_state, 7);
        }
        else vector_index = -1;

        //if codeword from huff_dec is legal
        if (vector_index >= 0) {

          vector_dec(huff_state,
            &sign_overflow,
            vector_index,
            category,
            vd,
            k_vect,
            sign_bits);

          //if bitcount exceeds bits per frame:  
          //noise fill current and remaining regions
          if (sign_overflow == -1) {
            for (i=reg_no; i<NUMBER_OF_REGIONS; i++) {
              categorization[i] = 7;
            }
            category = -1;
            reg_no--;
            break;
          }

          rec_mlt_ptr = &rec_mlt_coefs[(reg_no*COEFS_PER_REGION) + (vpr_cnt*vd)];

          mlt_reconstruct(k_vect,
            sign_bits,
            rms_level,
            category,
            vd,
            rec_mlt_ptr);
        }

        //if codeword from huff_dec is illegal: 
        //noise fill current and future regions
        else if (vector_index < 0) {

          for (i=reg_no; i<NUMBER_OF_REGIONS; i++) {
            categorization[i] = 7;
          }
          category = -1;
          reg_no--;
          break;
        }
      }

      //noise fill zero mlt-coefs for category 5 and 6
      if ((category == 5) || (category == 6)) {
        rec_mlt_ptr = &rec_mlt_coefs[reg_no*COEFS_PER_REGION];
        noise_fill(category,
          rms_level,
          &noise_filled_mlt);

        tester = rand_sign(&dec_state->rand_obj);
        for (i=0; i<HALF_COEFS_PER_REGION; i++) {
          if (rec_mlt_ptr[i] == 0) {						
            if ((tester & 1) == 0) {
              rec_mlt_ptr[i] = (short)(-noise_filled_mlt);
            }
            else {
              rec_mlt_ptr[i] = noise_filled_mlt;
            }
            tester = (short)(tester >> 1);
          }
        }

        tester = rand_sign(&dec_state->rand_obj);
        for (i=HALF_COEFS_PER_REGION; i<COEFS_PER_REGION; i++) {
          if (rec_mlt_ptr[i] == 0) {						
            if ((tester & 1) == 0) {
              rec_mlt_ptr[i] = (short)(-noise_filled_mlt);
            }
            else {
              rec_mlt_ptr[i] = noise_filled_mlt;
            }
            tester = (short)(tester >> 1);
          }
        }				
      }

      reg_no++;
    }

    //no bits assigned to category 7, noise fill entire region
    else if (category==7) {
      rec_mlt_ptr = &rec_mlt_coefs[reg_no*COEFS_PER_REGION];
      noise_fill(category,
        rms_level,
        &noise_filled_mlt);

      tester = rand_sign(&dec_state->rand_obj);
      for (i=0; i<HALF_COEFS_PER_REGION; i++) {
        if ((tester & 1) == 0) {
          rec_mlt_ptr[i] = (short)(-noise_filled_mlt);
        }
        else {
          rec_mlt_ptr[i] = noise_filled_mlt;
        }
        tester = (short)(tester >> 1);
      }

      tester = rand_sign(&dec_state->rand_obj);
      for (i=HALF_COEFS_PER_REGION; i<COEFS_PER_REGION; i++) {
        if ((tester & 1) == 0) {
          rec_mlt_ptr[i] = (short)(-noise_filled_mlt);
        }
        else {
          rec_mlt_ptr[i] = noise_filled_mlt;
        }
        tester = (short)(tester >> 1);
      }

      reg_no++;			
    }		
  }

  //zero fill the top 40 coefs
  for (i=NUMBER_OF_REGIONS*COEFS_PER_REGION ; i<DCT_LENGTH; i++) {
    rec_mlt_coefs[i] = 0;
  }
}


/*
* computes the quantization indices k() with belonging signbits, based on
* a given vector_index 
* 
* state - pointer to a structure of type huffState
* sign_overflow - flag set when get_next_bit tries to read more bits than bits_per_frame
* vector_index - the vector index to compute the k()'s from
* category - category assignment for the given region
* vd - vector dimension for the given region
* k_vect - pointer to array holding the computed k()'s
* sign_bits - pointer to array holding the sign bits
*/

void vector_dec(huffState * state,
                short * sign_overflow,
                short vector_index,
                short category,
                short vd,
                short * k_vect,
                short * sign_bits
                )
{
  extern short kmax_arr[7];

  short kmax_and_one = (short)(kmax_arr[category] + 1);

  int   ktmp = 1;
  int   temp;
  int   j;

  //compute k()'s from vector index
  for (j=0; j<vd; j++) {	

    if (j) ktmp *= kmax_and_one;

    temp = vector_index / ktmp;

    k_vect[vd-j-1] = (short)(temp % kmax_and_one);
  }

  //store corresponding sign bits
  for (j=0; j<vd; j++) {

    if (k_vect[j]!=0) {
      if (state->bits_read >= state->bits_per_frame) {
        *sign_overflow = -1;
        break;
      }
      sign_bits[j] = get_next_bit(state);
    }

    else {
      sign_bits[j] = 1;
    }
  }
}

/*
* computes the noise fill magnitude
* 
* category - category assignment for the given region
* rms_level - rms power level for the given region
* noise_filled_mlt - pointer to computed noise fill magnitude
*/

void noise_fill(short category,
                short rms_level,
                short * noise_filled_mlt
                )
{	
  short fill_factor = 0;
  int tmp;

  if (category == 5) fill_factor = 5793;
  else if (category == 6) fill_factor = 8192;
  else if (category == 7) fill_factor = 23170;

  tmp = fill_factor * rms_level;
  tmp = tmp >> 15;
  *noise_filled_mlt = (short)(tmp);
}

/* function: mlt_reconstruct
* 
* reconstruction of mlt-coefs from quantization indices
* 
* k_vect - pointer to array holding the computed k()'s
* sign_bits - pointer to array holding the sign bits
* rms_level - rms power level for the given region
* category - category assignment for the given region
* vd - vector dimension for the given region
* rec_mlt_ptr - pointer to recovered mlt-coefs
*/

void mlt_reconstruct(short * k_vect,
                     short * sign_bits,
                     short rms_level,
                     short category,
                     short vd,
                     short * rec_mlt_ptr
                     )
{
  extern short mlt_quant_centroid[7][14];

  int i;
  int tmp;

  for (i=0; i<vd; i++) {

    tmp = rms_level * mlt_quant_centroid[category][k_vect[i]];
    tmp = tmp >> 12;

    rec_mlt_ptr[i] = (short)(tmp);

    if (sign_bits[i] == 0)
      rec_mlt_ptr[i] = (short)(-rec_mlt_ptr[i]);
  }
}


/*
* compute rms power levels for regions in current frame
* 
* region index - pointer to absolute region power index
* expo - magnitude shift of mlt coefficients
* reg_level- pointer to array of rms power levels
*/

void get_reg_levels(short * region_index,
                    short * expo,
                    short * reg_level
                    )
{
  extern short quant_rms_arr[64];

  short i_max = 0;
  short tmp_dev = 0;
  short index;
  short i_tmp;
  short m_tmp;

  int reg_no;

  for (reg_no=0; reg_no<NUMBER_OF_REGIONS; reg_no++) {
    index = (short)(region_index[reg_no] + 17);
    i_tmp = (short)(index - i_max);

    if (i_tmp > 0) {
      i_max = index;
    }
    tmp_dev = (short)(tmp_dev + quant_rms_arr[index]);
  }

  index = 9;
  i_tmp = (short)(tmp_dev - 8);
  m_tmp = (short)(i_max - 28);

  while ((index>=0) && ((i_tmp>=0) || (m_tmp>0))) {
    index--;
    tmp_dev = (short)(tmp_dev >> 1);
    i_max -= 2;

    i_tmp = (short)(tmp_dev - 8);
    m_tmp = (short)(i_max - 28);
  }

  *expo = index;

  tmp_dev = (short)(17 + (2*(*expo)));

  for (reg_no=0; reg_no<NUMBER_OF_REGIONS; reg_no++) {
    index = (short)(region_index[reg_no] + tmp_dev);
    reg_level[reg_no] = quant_rms_arr[index];
  }
}

/*
* random generator
*/
short rand_sign(Rand_Obj *randobj)
{
  short random_word;

  random_word = (short)(randobj->seed0 + randobj->seed3);

  if ((random_word & 32768) != 0)
    random_word += 1;

  randobj->seed3 = randobj->seed2;
  randobj->seed2 = randobj->seed1;
  randobj->seed1 = randobj->seed0;
  randobj->seed0 = random_word;

  return(random_word);
}

/*
* window-overlap-add function to get new output samples
* 
* state - pointer to a structure of type decG722_1State
* current samples - generated samples for current frame
* outbuf - pointer to output samples
*/

void getoutbuf(decG722_1State * state,
               short * current_samples,
               short * outbuf)
{
  extern short rmlt_to_samples_window[DCT_LENGTH];

  int tmpbuf,scaled;
  int n;

  for (n=0; n<160; n++) 
  {		
    tmpbuf  = rmlt_to_samples_window[n] * current_samples[159-n];
    tmpbuf += rmlt_to_samples_window[319-n] * state->statebuf[n];
    tmpbuf += 4096;

    scaled = tmpbuf >> 13;
    if(scaled < -32767)
    {
      scaled = -32767;
    }
    if(scaled > 32767)
    {
      scaled = 32767;
    }

    outbuf[n] = (short)scaled;

    tmpbuf  = rmlt_to_samples_window[160+n] * current_samples[n];
    tmpbuf -= rmlt_to_samples_window[159-n] * state->statebuf[159-n];
    tmpbuf += 4096;

    scaled = tmpbuf >> 13;
    if(scaled < -32767)
    {
      scaled = -32767;
    }
    if(scaled > 32767)
    {
      scaled = 32767;
    }

    outbuf[n+160] = (short)scaled;
  }

  //store old samples and error buffer in case of frame erasure
  for (n=0; n<160; n++) {
    state->fe_buf[n] = current_samples[n];
    state->statebuf[n] = current_samples[160+n];
  }
}

/*
* adjust dct-coefs based on expo and bias-table
* 
* expo - magnitude shift for dct-coefs
* current samples - pointer to generated samples for current frame
*/

void adjust_dct(short expo,
                short * current_samples
                )
{	
  extern signed char mlt_bias[DCT_LENGTH];

  int r;
  int tmp;


  //add bias to computed samples
  for(r=0; r<DCT_LENGTH; r++) {
    tmp = current_samples[r] + mlt_bias[r];

    if (tmp > 32767) tmp = 32767;
    if (tmp < -32768) tmp = -32768;

    current_samples[r] = (short)(tmp);
  }

  //compensate for expo
  if (expo > 0) {

    for(r=0; r<DCT_LENGTH; r++) {
      current_samples[r] = (short)(current_samples[r] >> expo);
    }
  }

  else if (expo < 0) {

    for(r=0; r<DCT_LENGTH; r++)
    {
      current_samples[r] = (short)(current_samples[r] << (-expo));
    }
  }
}
