
/*
 * Part of the code is based on the reference C code provided with the ITU-T Recommendation G.722.1, 
 * downloaded from the official ITU website. Copyrights as indicated in the reference code 
 * may still apply.
 *
 */

#include "g7221_defs.h"
#ifndef _TMS320C6X
#include "cintrins.h"
#endif

/* in_buffer  points to input data (320 shorts)
 * out_buffer points to output data (320 shorts)
 * dct_type is 1 for decoder, 0 for encoder
 * dct_core_adr points to correct dct_core table
 * cos_msin_table_adr points to correct cos_msin_table
 */
void dct(
	short * in_buffer,
	short * out_buffer,
	short dct_type,
	short * dct_core_adr,
	short * cos_msin_table_adr
)
{
    extern unsigned char s_dither[];

	short shift_factor = 1;
	short dptr = 0;

    short dflg = dct_type;
    short cnt0;
    short cnt1;
    short i;
    short j;
    short k;
    short * ptr0;
    short * ptr1;
    short * ptr2;
    short * ptr4;
    short * ptr5;
    short * ptr6;
    short * iptr;
    short * optr;
    short * tmpptr;
    short * cos_msin_ptr;
    short d0;
    short d1;
    short tmp;
    short tmp1;
    short tmp2;
    short tmp3;
    short tmp4;
    int   outl;
    int   outh;
    int   prod;
    int   sum;
      
    // sum/difference butterflies, converts 320pt transform into 32 10pt transforms
    
    cnt0 = 5;
    do {
		i = (short)(5 - cnt0);
        k = (short)(DCT_LENGTH >> i);
        j = (short)(1 << i);
		
        ptr0 = in_buffer;
        optr = out_buffer;
        
		cnt1 = j;
			
		do {
			ptr1 = optr;
			optr = optr + k;
			ptr2 = optr - 1;
            
            do {

                // dither is used if decoder, if encoder dflg is 0, and only the first word of
                // s_dither is read, it's also masked with dflg to be zero

                d0 = s_dither[dptr];
                d0 = (short)(d0 & dflg);
                dptr = (short)(dptr + dflg);
                d1 = s_dither[dptr];
                d1 = (short)(d1 & dflg);
                dptr = (short)(dptr + dflg);
	
	            tmp1 = *ptr0;
	            ptr0++;
	            tmp2 = *ptr0;
	            ptr0++;
	            
	            outl = d0     + tmp1;
	            outl = outl + tmp2;
	            outl = outl >> shift_factor;
                *ptr1 = (short)outl;
                ptr1++;
                
	            outh = d1      + tmp1;
	            outh = outh - tmp2;
	            outh = outh >> shift_factor;
	            *ptr2 = (short)outh;
	            ptr2--;

	            tmp = (short)(ptr2 > ptr1);
	            
		    } while (tmp);

		    cnt1--;
		
	    } while (cnt1);


	    tmpptr = in_buffer;
	    in_buffer = out_buffer;
	    out_buffer = tmpptr;

        dflg = 0;
	   
	    shift_factor = (short)(1 - dct_type);
	   
	    cnt0--;
	   
    } while (cnt0);

    // do 32 10pt transforms
    
    ptr0 = in_buffer;
    ptr1 = out_buffer;
    	
	cnt0 = 32;
	
	do {
	    ptr4 = dct_core_adr;	    
	    cnt1 = 10;
	    
	    do {
			
			sum = 0;
			
			tmp1 = *ptr0;
			ptr0 += 1;
			tmp2 = *ptr4;
			ptr4 += 10;			
			prod = _smpy(tmp1,tmp2);
			sum  = _sadd(sum,prod);  // 1

			tmp1 = *ptr0;
			ptr0 += 1;
			tmp2 = *ptr4;
			ptr4 += 10;			
			prod = _smpy(tmp1,tmp2);
			sum  = _sadd(sum,prod);  // 2

			tmp1 = *ptr0;
			ptr0 += 1;
			tmp2 = *ptr4;
			ptr4 += 10;			
			prod = _smpy(tmp1,tmp2);
			sum  = _sadd(sum,prod);  // 3

			tmp1 = *ptr0;
			ptr0 += 1;
			tmp2 = *ptr4;
			ptr4 += 10;			
			prod = _smpy(tmp1,tmp2);
			sum  = _sadd(sum,prod);  // 4

			tmp1 = *ptr0;
			ptr0 += 1;
			tmp2 = *ptr4;
			ptr4 += 10;			
			prod = _smpy(tmp1,tmp2);
			sum  = _sadd(sum,prod);  // 5

			tmp1 = *ptr0;
			ptr0 += 1;
			tmp2 = *ptr4;
			ptr4 += 10;			
			prod = _smpy(tmp1,tmp2);
			sum  = _sadd(sum,prod);  // 6

			tmp1 = *ptr0;
			ptr0 += 1;
			tmp2 = *ptr4;
			ptr4 += 10;			
			prod = _smpy(tmp1,tmp2);
			sum  = _sadd(sum,prod);  // 7

			tmp1 = *ptr0;
			ptr0 += 1;
			tmp2 = *ptr4;
			ptr4 += 10;			
			prod = _smpy(tmp1,tmp2);
			sum  = _sadd(sum,prod);  // 8

			tmp1 = *ptr0;
			ptr0 += 1;
			tmp2 = *ptr4;
			ptr4 += 10;			
			prod = _smpy(tmp1,tmp2);
			sum  = _sadd(sum,prod);  // 9

			tmp1 = *ptr0;
			ptr0 -= 9;               // different mod
			tmp2 = *ptr4;
			ptr4 -= 89;	             // different mod	
			prod = _smpy(tmp1,tmp2);
			sum  = _sadd(sum,prod);  // 10
			
            sum  = _sadd(sum,0x8000);
            tmp1 = (short)(sum >> 16);

            *ptr1 = tmp1;
            ptr1++;
            
		    cnt1--; 

	    } while (cnt1);
	    
	    ptr0 += 10;	
	    cnt0--;
	    
    } while (cnt0);

	// perform rotation butterflies

	tmpptr = in_buffer;
	in_buffer = out_buffer;
	out_buffer = tmpptr;
	
	cos_msin_ptr = cos_msin_table_adr;
	k = 0;
	
	cnt0 = 5;
	
	do {
		
   	    cos_msin_ptr = cos_msin_ptr + k;
	   
	    i = (short)(cnt0 - 1);
	    k = (short)(DCT_LENGTH >> i);
	    j = (short)(1 << i);
 
        iptr = in_buffer;
        optr = out_buffer;
       
        cnt1 = j;
       
        do {
           
            tmpptr = iptr;
            ptr0 = tmpptr;
           
            tmp1 = (short)(k >> 1);
            tmpptr = tmpptr + tmp1;
           
            ptr1 = tmpptr;
           
            tmpptr = tmpptr + tmp1;
            iptr = tmpptr;
           
            tmpptr = optr;
            ptr4 = tmpptr;
           
            tmpptr = tmpptr + k;
            optr = tmpptr;
           
            tmpptr--;
            ptr5 = tmpptr;
           
            ptr6 = cos_msin_ptr;
            
            do {
			  
			   tmp1 = *ptr0;
			   ptr0++;
			   tmp2 = *ptr1;
			   ptr1++;
			   tmp3 = *ptr6;
			   ptr6++;
			   tmp4 = *ptr6;
			   ptr6++;
			  
			   sum  = _smpy(tmp1,tmp4);
			   prod = _smpy(tmp2,tmp3);
			   sum  = sum + prod;
			  
			   sum  = _sshl(sum,dct_type);
               sum  = _sadd(sum,0x8000);
			  
			   tmp = (short)(sum >> 16);
			  
			   *ptr5 = tmp;
			   ptr5--;
			  
			   sum  = _smpy(tmp1,tmp3);
			   prod = _smpy(tmp2,tmp4);
			   sum  = sum - prod;
			  
			   sum  = _sshl(sum,dct_type);
               sum  = _sadd(sum,0x8000);
			  
			   tmp = (short)(sum >> 16);
			  
			   *ptr4 = tmp;
			   ptr4++;
		  
			   tmp1 = *ptr0;
			   ptr0++;
			   tmp2 = *ptr1;
			   ptr1++;
			   tmp3 = *ptr6;
			   ptr6++;
			   tmp4 = *ptr6;
			   ptr6++;	      

			   sum  = _smpy(tmp1,tmp3);
			   prod = _smpy(tmp2,tmp4);
			   sum  = sum + prod;
			  
			   sum  = _sshl(sum,dct_type);
               sum  = _sadd(sum,0x8000);
			  
			   tmp = (short)(sum >> 16);
			  
			   *ptr4 = tmp;
			   ptr4++;
			  
			   sum  = _smpy(tmp1,tmp4);
			   prod = _smpy(tmp2,tmp3);
			   sum  = sum - prod;
			  
			   sum  = _sshl(sum,dct_type);
               sum  = _sadd(sum,0x8000);
			  
			   tmp = (short)(sum >> 16);
			  
			   *ptr5 = tmp;
			   ptr5--;

               tmp = (short)(ptr5 > ptr4);
               
	       } while (tmp);

		   cnt1--;
		   
       } while (cnt1);

   	   tmpptr = in_buffer;
	   in_buffer = out_buffer;
	   out_buffer = tmpptr;
	   
	   cnt0--;
	    
    } while (cnt0);
}

/*
 * Compute 16 different categorizations 
 * 
 * pow_ind - pointer to 14 rms indices, short format
 * avail_bits - number of bits available for encoding of mlt-coeffs.
 * init_cat - categorization 0, short format
 * cat_balance - 15 elements, each element gives the region where the 
 *				 category is adjusted + 1 from previous categorization 
 */

void categorize(
	short * pow_ind,
	short avail_bits,
	short * init_cat,
	short * cat_balance
)
{
	extern short expected_bits_table[8];

	short min_cat[14];
	short temp_cat_bal[32];

	int tmp0;
	int tmp1;
	int tmp2;
	int offset=-32;
	int delta=32;
	int r;
	int expect_bits;
	int max_bits;
	int min_bits;
	int max_pointer=16;
	int min_pointer=16;
	int cat_count=1;
	int region=-1;

	//adjust number of available bits
	tmp0=avail_bits-320;
	if (tmp0>0) {
		tmp0*=5;
		tmp0=tmp0>>3;
		avail_bits=(short)(tmp0+320);
	}

	//initial categorization
	tmp1=1;
	while (tmp1) {
		
		tmp0=offset+delta;
		expect_bits=0;
		for (r=0;r<NUMBER_OF_REGIONS;r++) {
			tmp2=tmp0-pow_ind[r];
			tmp2=tmp2/2;
			if(tmp2<0) tmp2=0;
			else if (tmp2>7) tmp2=7;
			expect_bits+=expected_bits_table[tmp2];
			min_cat[r]=(short)tmp2;
			init_cat[r]=(short)tmp2;	
		}
		tmp0=avail_bits-32;
		if(expect_bits>=tmp0) offset+=delta;
		
		tmp1=delta;
		delta=delta>>1;
	}

	max_bits=expect_bits;
	min_bits=expect_bits;

	//other 15 categorizations
	while(cat_count<16) {
		tmp0=max_bits+min_bits;
		tmp1=avail_bits<<1;
		if(tmp0<=tmp1) {	//categorize with more bits
			tmp1=1000;		//or any large enough number
			for (r=0;r<NUMBER_OF_REGIONS;r++) {
				if(init_cat[r]>0) {
					tmp0=init_cat[r]<<1;
					tmp2=offset-pow_ind[r];
					tmp0=tmp2-tmp0;
					if(tmp0<tmp1) {
						tmp1=tmp0;
						region=r;
					}
				}
				
			}
			temp_cat_bal[--max_pointer]=(short)region;
			max_bits-=expected_bits_table[init_cat[region]];
			init_cat[region]-=1;
			max_bits+=expected_bits_table[init_cat[region]];
		}
		else {		//categorize with less bits 
			tmp1=-1000;
			for (r=NUMBER_OF_REGIONS-1;r>=0;r--) {
				if(min_cat[r]<7) {
					tmp0=min_cat[r]<<1;
					tmp2=offset-pow_ind[r];
					tmp0=tmp2-tmp0;
					if(tmp0>tmp1) {
						tmp1=tmp0;
						region=r;
					}
				}
			}
			temp_cat_bal[min_pointer++]=(short)region;
			min_bits-=expected_bits_table[min_cat[region]];
			min_cat[region]+=1;
			min_bits+=expected_bits_table[min_cat[region]];		
		}
		cat_count++;	
	} 

	for(r=0;r<15;r++) {
		cat_balance[r]=temp_cat_bal[max_pointer++];
	}
	
}
