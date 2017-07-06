/*******************************************************************************

*******************************************************************************/
/*! \file PHY/LTE_TRANSPORT/dci_NB_IoT.c
* \brief Top-level routines for implementing Tail-biting convolutional coding for transport channels (NPDCCH) for NB_IoT,	 TS 36-212, V13.4.0 2017-02
* \author M. KANJ
* \date 2017
* \version 0.0
* \company bcom
* \email: matthieu.kanj@b-com.com
* \note
* \warning
*/

#ifdef USER_MODE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif
#include "PHY/defs.h"
#include "PHY/extern.h"
#include "SCHED/defs.h"
#include "SIMULATION/TOOLS/defs.h"
#include "PHY/sse_intrin.h"

#include "assertions.h" 
#include "T.h"

static uint8_t d[2][3*(MAX_DCI_SIZE_BITS_NB_IOT + 16) + 96];
static uint8_t w[2][3*3*(MAX_DCI_SIZE_BITS_NB_IOT+16)];


void dci_encoding_NB_IoT(uint8_t *a[2],				// Table of two DCI, even if one DCI is to transmit , the number of DCI is indicated in dci_number
						uint8_t A,					// Length of table a
						uint16_t E,					// E should equals to G (number of available bits in one RB)
						uint8_t *e[2],				// *e should be e[2][G]
						uint16_t rnti[2],			// RNTI for UE specific or common search space
						uint8_t dci_number,			// This variable should takes the 1 or 2 (1 for in case of one DCI, 2 in case of two DCI)
						uint8_t agr_level)			// Aggregation level	
{
	uint8_t D = (A + 16);
	uint32_t RCC;
	uint8_t occupation_size=1;
	// encode dci
	if(dci_number == 1)
	{
		if(agr_level == 2)
		{
			occupation_size=1;
		}else{
			occupation_size=2;
		}
		memset((void *)d[0],LTE_NULL,96);
  
		ccode_encode_NB_IoT(A,2,a[0],d[0]+96,rnti[0]);    					// CRC attachement & Tail-biting convolutional coding
		RCC = sub_block_interleaving_cc_NB_IoT(D,d[0]+96,w[0]);				// Interleaving
		lte_rate_matching_cc_NB_IoT(RCC,(E/occupation_size),w[0],e[0]);		// Rate Matching
		
	}else if (dci_number == 2) {
		
		memset((void *)d[0],LTE_NULL,96);
		memset((void *)d[1],LTE_NULL,96);
		// first DCI encoding
		ccode_encode_NB_IoT(A,2,a[0],d[0]+96,rnti[0]);    					// CRC attachement & Tail-biting convolutional coding
		RCC = sub_block_interleaving_cc_NB_IoT(D,d[0]+96,w[0]);				// interleaving
		lte_rate_matching_cc_NB_IoT(RCC,E/2,w[0],e[0]);						// Rate Matching , E/2 ,  NCCE0
		// second DCI encoding
		ccode_encode_NB_IoT(A,2,a[1],d[1]+96,rnti[1]);    					// CRC attachement & Tail-biting convolutional coding
		RCC = sub_block_interleaving_cc_NB_IoT(D,d[1]+96,w[1]);				// Interleaving
		lte_rate_matching_cc_NB_IoT(RCC,E/2,w[1],e[1]);						// Rate Matching, E/2 , NCCE1
		
	}
}

///The scrambling sequence shall be initialised at the start of the search space and after every 4th NPDCCH subframes.
///
///
void npdcch_scrambling_NB_IoT(NB_IOT_DL_FRAME_PARMS *frame_parms,
							  uint8_t *e[2],							// Input data
							  int length,        						// Total number of bits to transmit in one subframe(case of DCI = G)
							  uint8_t Ns,								// Slot number (0..19)
							  uint8_t dci_number,						// This variable should takes the 1 or 2 (1 for in case of one DCI, 2 in case of two DCI)
							  uint8_t agr_level)						// Aggregation level							
{
	int i,j,k=0;
	uint32_t x1, x2, s=0;
	uint8_t reset;
	reset = 1;
	uint8_t occupation_size=1;
	
	if(agr_level == 2)
	{
		occupation_size=1;
	}else{
		occupation_size=2;
	}

	if(dci_number == 1)														// Case of one DCI
	{
		x2 = ((Ns>>1)<<9) + frame_parms->Nid_cell;                          // This is c_init in 36.211 Sec 10.2.3.1
		
		for (i=0; i<length/occupation_size; i++) {
			if ((i&0x1f)==0) {
				s = lte_gold_generic_NB_IoT(&x1, &x2, reset);
				reset = 0;
			}
			e[0][k] = (e[0][k]&1) ^ ((s>>(i&0x1f))&1);
		}
		
	}else if(dci_number == 2 && occupation_size == 2) {						// Case of two DCI
		
		// Scrambling the first DCI
		//
		x2 = ((Ns>>1)<<9) + frame_parms->Nid_cell;                          // This is c_init in 36.211 Sec 10.2.3.1
  
		for (i=0; i<length/occupation_size; i++) {
			if ((i&0x1f)==0) {
				s = lte_gold_generic_NB_IoT(&x1, &x2, reset);
				reset = 0;
			}
			e[0][k] = (e[0][k]&1) ^ ((s>>(i&0x1f))&1);
		}
		// reset of the scrambling function
		reset = 1;
		// Scrambling the second DCI
		//
		x2 = ((Ns>>1)<<9) + frame_parms->Nid_cell;                          //this is c_init in 36.211 Sec 10.2.3.1
  
		for (i=0; i<length/occupation_size; i++) {
			if ((i&0x1f)==0) {
				s = lte_gold_generic_NB_IoT(&x1, &x2, reset);
				reset = 0;
			}
			e[1][k] = (e[1][k]&1) ^ ((s>>(i&0x1f))&1);
		}
	}
}


int dci_allocate_REs_in_RB_NB_IoT(NB_IOT_DL_FRAME_PARMS *frame_parms,
                                  int32_t **txdataF,
                                  uint32_t *jj,
                                  uint32_t symbol_offset,
                                  uint8_t *x0[2],
                                  uint8_t pilots,
                                  int16_t amp,
						  	      unsigned short id_offset,
                                  uint32_t *re_allocated, 								//  not used variable ??!!
								  uint8_t dci_number,									// This variable should takes the 1 or 2 (1 for in case of one DCI, 2 in case of two DCI)
								  uint8_t agr_level) 
{
	MIMO_mode_t mimo_mode = (frame_parms->mode1_flag==1)?SISO:ALAMOUTI;
	uint32_t tti_offset,aa;
	uint8_t re, diff_re;
	int16_t gain_lin_QPSK;
	uint8_t first_re,last_re;
	int32_t tmp_sample1,tmp_sample2,tmp_sample3,tmp_sample4;
	gain_lin_QPSK = (int16_t)((amp*ONE_OVER_SQRT2_Q15)>>15);
	first_re=0;
	last_re=12;
  
	if(agr_level == 2 && dci_number == 1)
	{
		for (re=first_re; re<last_re; re++) {      		// re varies between 0 and 12 sub-carriers

			tti_offset = symbol_offset + re;				// symbol_offset = 512 * L ,  re_offset = 512 - 3*12  , re
	
			if (pilots != 1 || re%3 != id_offset)  			// if re is not a pilot
			{
													//	diff_re = re%3 - id_offset;  
				if (mimo_mode == SISO) {  								//SISO mapping
					*re_allocated = *re_allocated + 1;						// variable incremented but never used
			
					for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
						((int16_t*)&txdataF[aa][tti_offset])[0] += (x0[0][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //I //b_i
					}
					*jj = *jj + 1;
					for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
						((int16_t*)&txdataF[aa][tti_offset])[1] += (x0[0][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //Q //b_{i+1}
					}
					*jj = *jj + 1;	
			
			} else if (mimo_mode == ALAMOUTI) {
	  
				*re_allocated = *re_allocated + 1;

				((int16_t*)&tmp_sample1)[0] = (x0[0][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
				*jj=*jj+1;
				((int16_t*)&tmp_sample1)[1] = (x0[0][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
				*jj=*jj+1;

				// second antenna position n -> -x1*

				((int16_t*)&tmp_sample2)[0] = (x0[0][*jj]==1) ? (gain_lin_QPSK) : -gain_lin_QPSK;
				*jj=*jj+1;
				((int16_t*)&tmp_sample2)[1] = (x0[0][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
				*jj=*jj+1;

				// normalization for 2 tx antennas
				((int16_t*)&txdataF[0][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample1)[0]*ONE_OVER_SQRT2_Q15)>>15);
				((int16_t*)&txdataF[0][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample1)[1]*ONE_OVER_SQRT2_Q15)>>15);
				((int16_t*)&txdataF[1][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample2)[0]*ONE_OVER_SQRT2_Q15)>>15);
				((int16_t*)&txdataF[1][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample2)[1]*ONE_OVER_SQRT2_Q15)>>15);
	
				// fill in the rest of the ALAMOUTI precoding
				if ( pilots != 1 || (re+1)%3 != id_offset) {
					((int16_t *)&txdataF[0][tti_offset+1])[0] += -((int16_t *)&txdataF[1][tti_offset])[0]; //x1
					((int16_t *)&txdataF[0][tti_offset+1])[1] += ((int16_t *)&txdataF[1][tti_offset])[1];
					((int16_t *)&txdataF[1][tti_offset+1])[0] += ((int16_t *)&txdataF[0][tti_offset])[0];  //x0*
					((int16_t *)&txdataF[1][tti_offset+1])[1] += -((int16_t *)&txdataF[0][tti_offset])[1];
				} else {
					((int16_t *)&txdataF[0][tti_offset+2])[0] += -((int16_t *)&txdataF[1][tti_offset])[0]; //x1
					((int16_t *)&txdataF[0][tti_offset+2])[1] += ((int16_t *)&txdataF[1][tti_offset])[1];
					((int16_t *)&txdataF[1][tti_offset+2])[0] += ((int16_t *)&txdataF[0][tti_offset])[0];  //x0*
					((int16_t *)&txdataF[1][tti_offset+2])[1] += -((int16_t *)&txdataF[0][tti_offset])[1];
		
					re++;														// skip pilots
					*re_allocated = *re_allocated + 1;
				}
				re++;  															// adjacent carriers are taken care of by precoding
				*re_allocated = *re_allocated + 1;   							// incremented variable but never used
			}	
		}
	}
  }else if(agr_level == 1 && dci_number == 1){
	for (re=first_re; re<6; re++) {      		// re varies between 0 and 6 sub-carriers

    tti_offset = symbol_offset + re;				// symbol_offset = 512 * L ,  re_offset = 512 - 3*12  , re
	
	if (pilots != 1 || re%3 != id_offset)  			// if re is not a pilot
	{
													//	diff_re = re%3 - id_offset;  
		if (mimo_mode == SISO) {  								//SISO mapping
			*re_allocated = *re_allocated + 1;						// variable incremented but never used
			
			for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
					((int16_t*)&txdataF[aa][tti_offset])[0] += (x0[0][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //I //b_i
			}
			*jj = *jj + 1;
			for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
				((int16_t*)&txdataF[aa][tti_offset])[1] += (x0[0][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //Q //b_{i+1}
			}
			*jj = *jj + 1;	
			
		} else if (mimo_mode == ALAMOUTI) {
	  
			*re_allocated = *re_allocated + 1;

			((int16_t*)&tmp_sample1)[0] = (x0[0][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
			*jj=*jj+1;
			((int16_t*)&tmp_sample1)[1] = (x0[0][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
			*jj=*jj+1;

			// second antenna position n -> -x1*

			((int16_t*)&tmp_sample2)[0] = (x0[0][*jj]==1) ? (gain_lin_QPSK) : -gain_lin_QPSK;
			*jj=*jj+1;
			((int16_t*)&tmp_sample2)[1] = (x0[0][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
			*jj=*jj+1;

			// normalization for 2 tx antennas
			((int16_t*)&txdataF[0][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample1)[0]*ONE_OVER_SQRT2_Q15)>>15);
			((int16_t*)&txdataF[0][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample1)[1]*ONE_OVER_SQRT2_Q15)>>15);
			((int16_t*)&txdataF[1][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample2)[0]*ONE_OVER_SQRT2_Q15)>>15);
			((int16_t*)&txdataF[1][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample2)[1]*ONE_OVER_SQRT2_Q15)>>15);

			// fill in the rest of the ALAMOUTI precoding
			if ( pilots != 1 || (re+1)%3 != id_offset) {
				((int16_t *)&txdataF[0][tti_offset+1])[0] += -((int16_t *)&txdataF[1][tti_offset])[0]; //x1
				((int16_t *)&txdataF[0][tti_offset+1])[1] += ((int16_t *)&txdataF[1][tti_offset])[1];
				((int16_t *)&txdataF[1][tti_offset+1])[0] += ((int16_t *)&txdataF[0][tti_offset])[0];  //x0*
				((int16_t *)&txdataF[1][tti_offset+1])[1] += -((int16_t *)&txdataF[0][tti_offset])[1];
			} else {
				((int16_t *)&txdataF[0][tti_offset+2])[0] += -((int16_t *)&txdataF[1][tti_offset])[0]; //x1
				((int16_t *)&txdataF[0][tti_offset+2])[1] += ((int16_t *)&txdataF[1][tti_offset])[1];
				((int16_t *)&txdataF[1][tti_offset+2])[0] += ((int16_t *)&txdataF[0][tti_offset])[0];  //x0*
				((int16_t *)&txdataF[1][tti_offset+2])[1] += -((int16_t *)&txdataF[0][tti_offset])[1];
		
				re++;														// skip pilots
				*re_allocated = *re_allocated + 1;
			}
			re++;  															// adjacent carriers are taken care of by precoding
			*re_allocated = *re_allocated + 1;   							// incremented variable but never used
		 }
     }
   }
  } else {
	
	// allocate first DCI
	for (re=first_re; re<6; re++) {      		// re varies between 0 and 12 sub-carriers

    tti_offset = symbol_offset + re;				// symbol_offset = 512 * L ,  re_offset = 512 - 3*12  , re
	
	if (pilots != 1 || re%3 != id_offset)  			// if re is not a pilot
	{
													//	diff_re = re%3 - id_offset;  
      if (mimo_mode == SISO) {  								//SISO mapping
			*re_allocated = *re_allocated + 1;						// variable incremented but never used
			
			for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
				((int16_t*)&txdataF[aa][tti_offset])[0] += (x0[0][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //I //b_i
				((int16_t*)&txdataF[aa][tti_offset+6])[0] += (x0[1][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //I //b_i
			}
			*jj = *jj + 1;
			for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
				((int16_t*)&txdataF[aa][tti_offset])[1] += (x0[0][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //Q //b_{i+1}
				((int16_t*)&txdataF[aa][tti_offset+6])[1] += (x0[1][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //Q //b_{i+1}
			}
			*jj = *jj + 1;	
			
      } else if (mimo_mode == ALAMOUTI) {
	  
			*re_allocated = *re_allocated + 1;

			((int16_t*)&tmp_sample1)[0] = (x0[0][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
			((int16_t*)&tmp_sample3)[0] = (x0[1][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
			*jj=*jj+1;
			((int16_t*)&tmp_sample1)[1] = (x0[0][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
			((int16_t*)&tmp_sample3)[1] = (x0[1][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
			*jj=*jj+1;

			// second antenna position n -> -x1*

			((int16_t*)&tmp_sample2)[0] = (x0[0][*jj]==1) ? (gain_lin_QPSK) : -gain_lin_QPSK;
			((int16_t*)&tmp_sample4)[0] = (x0[1][*jj]==1) ? (gain_lin_QPSK) : -gain_lin_QPSK;
			*jj=*jj+1;
			((int16_t*)&tmp_sample2)[1] = (x0[0][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
			((int16_t*)&tmp_sample4)[1] = (x0[1][*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
			*jj=*jj+1;

			// normalization for 2 tx antennas
			((int16_t*)&txdataF[0][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample1)[0]*ONE_OVER_SQRT2_Q15)>>15);
			((int16_t*)&txdataF[0][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample1)[1]*ONE_OVER_SQRT2_Q15)>>15);
			((int16_t*)&txdataF[1][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample2)[0]*ONE_OVER_SQRT2_Q15)>>15);
			((int16_t*)&txdataF[1][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample2)[1]*ONE_OVER_SQRT2_Q15)>>15);
			
			((int16_t*)&txdataF[0][tti_offset+6])[0] += (int16_t)((((int16_t*)&tmp_sample3)[0]*ONE_OVER_SQRT2_Q15)>>15);
			((int16_t*)&txdataF[0][tti_offset+6])[1] += (int16_t)((((int16_t*)&tmp_sample3)[1]*ONE_OVER_SQRT2_Q15)>>15);
			((int16_t*)&txdataF[1][tti_offset+6])[0] += (int16_t)((((int16_t*)&tmp_sample4)[0]*ONE_OVER_SQRT2_Q15)>>15);
			((int16_t*)&txdataF[1][tti_offset+6])[1] += (int16_t)((((int16_t*)&tmp_sample4)[1]*ONE_OVER_SQRT2_Q15)>>15);

			// fill in the rest of the ALAMOUTI precoding
			if ( pilots != 1 || (re+1)%3 != id_offset) {
				((int16_t *)&txdataF[0][tti_offset+1])[0] += -((int16_t *)&txdataF[1][tti_offset])[0]; //x1
				((int16_t *)&txdataF[0][tti_offset+1])[1] += ((int16_t *)&txdataF[1][tti_offset])[1];
				((int16_t *)&txdataF[1][tti_offset+1])[0] += ((int16_t *)&txdataF[0][tti_offset])[0];  //x0*
				((int16_t *)&txdataF[1][tti_offset+1])[1] += -((int16_t *)&txdataF[0][tti_offset])[1];
				
				((int16_t *)&txdataF[0][tti_offset+6+1])[0] += -((int16_t *)&txdataF[1][tti_offset+6])[0]; //x1
				((int16_t *)&txdataF[0][tti_offset+6+1])[1] += ((int16_t *)&txdataF[1][tti_offset+6])[1];
				((int16_t *)&txdataF[1][tti_offset+6+1])[0] += ((int16_t *)&txdataF[0][tti_offset+6])[0];  //x0*
				((int16_t *)&txdataF[1][tti_offset+6+1])[1] += -((int16_t *)&txdataF[0][tti_offset+6])[1];
			} else {
				((int16_t *)&txdataF[0][tti_offset+2])[0] += -((int16_t *)&txdataF[1][tti_offset])[0]; //x1
				((int16_t *)&txdataF[0][tti_offset+2])[1] += ((int16_t *)&txdataF[1][tti_offset])[1];
				((int16_t *)&txdataF[1][tti_offset+2])[0] += ((int16_t *)&txdataF[0][tti_offset])[0];  //x0*
				((int16_t *)&txdataF[1][tti_offset+2])[1] += -((int16_t *)&txdataF[0][tti_offset])[1];
				
				((int16_t *)&txdataF[0][tti_offset+6+2])[0] += -((int16_t *)&txdataF[1][tti_offset+6])[0]; //x1
				((int16_t *)&txdataF[0][tti_offset+6+2])[1] += ((int16_t *)&txdataF[1][tti_offset+6])[1];
				((int16_t *)&txdataF[1][tti_offset+6+2])[0] += ((int16_t *)&txdataF[0][tti_offset+6])[0];  //x0*
				((int16_t *)&txdataF[1][tti_offset+6+2])[1] += -((int16_t *)&txdataF[0][tti_offset+6])[1];
		
				re++;														// skip pilots
				*re_allocated = *re_allocated + 1;
			}
			re++;  															// adjacent carriers are taken care of by precoding
			*re_allocated = *re_allocated + 1;   							// incremented variable but never used
		}
      }
    }
  }
  return(0);
}


int dci_modulation_NB_IoT(int32_t **txdataF,
						int16_t amp,
						NB_IOT_DL_FRAME_PARMS *frame_parms,
						uint8_t control_region_size,                        // control region size for LTE , values between 0..3, (0 for stand-alone / 1, 2 or 3 for in-band)
						uint8_t *e[2],										// Input data
						int G,												// number of bits per subframe		
						unsigned short NB_IoT_RB_ID
						uint8_t dci_number,									// This variable should takes the 1 or 2 (1 for in case of one DCI, 2 in case of two DCI)
						uint8_t agr_level)									// Aggregation level
{
    uint32_t jj=0;
	uint32_t re_allocated,symbol_offset;
    uint16_t l;
    uint8_t id_offset,pilots=0; 
	unsigned short bandwidth_even_odd;
    unsigned short NB_IoT_start, RB_IoT_ID;
    re_allocated=0;
	id_offset=0;
	// testing if the total number of RBs is even or odd 
		bandwidth_even_odd = frame_parms->N_RB_DL % 2; 	 	// 0 even, 1 odd
		RB_IoT_ID = NB_IoT_RB_ID;
	// step  5, 6, 7   									 	// modulation and mapping (slot 1, symbols 0..3)
	for (l=control_region_size; l<14; l++) { 								 	// loop on OFDM symbols	
		if((l>=4 && l<=8) || (l>=11 && l<=13))
		{
			pilots =1;
		} else {
			pilots=0;
		}
		id_offset = frame_parms->Nid_cell % 3;    			// Cell_ID_NB_IoT % 3
		if(RB_IoT_ID < (frame_parms->N_RB_DL/2))
		{
			NB_IoT_start = frame_parms->ofdm_symbol_size - 12*(frame_parms->N_RB_DL/2) - (bandwidth_even_odd*6) + 12*(RB_IoT_ID%(ceil(frame_parms->N_RB_DL/(float)2)));
		} else {
			NB_IoT_start = (bandwidth_even_odd*6) + 12*(RB_IoT_ID%(ceil(frame_parms->N_RB_DL/(float)2)));
		}
		symbol_offset = frame_parms->ofdm_symbol_size*l + NB_IoT_start;  						// symbol_offset = 512 * L + NB_IOT_RB start
		dci_allocate_REs_in_RB_NB_IoT(frame_parms,
								      txdataF,
							    	  &jj,
									  symbol_offset,
									  &e,
									  pilots,
									  amp,
									  id_offset,
									  &re_allocated,
									  dci_number,
									  agr_level);
	}
	
    // VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_MODULATION, VCD_FUNCTION_OUT);
  return (re_allocated);
}





//*******************************************************************************************************
//*******************************************************************************************************
//********************************** Michele code *******************************************************
//*******************************************************************************************************
//*******************************************************************************************************
//*******************************************************************************************************
uint8_t generate_dci_top_NB(uint8_t Num_dci,
                         DCI_ALLOC_NB_t *dci_alloc,
                         int16_t amp,
                         NB_DL_FRAME_PARMS *fp,
                         //NB_IoT_eNB_NPDCCH_t npdcch,
                         int32_t **txdataF,
                         uint32_t subframe)
{


  int i,L, G;
  int npdcch_start_index;

  /* PARAMETERS may not needed
  **e_ptr : store the encoding result, and as a input to modulation
  *num_pdcch_symbols : to calculate the resource allocation for pdcch
  *L = aggregation level (there is 2 (at most) in NB-IoT) (Note this is not the real value but the index)
  *lprime,kprime,kprime_mod12,mprime,nsymb,symbol_offset,tti_offset,re_offset : used in the REG allocation
  *gain_lin_QPSK,yseq0[Msymb],yseq1[Msymb],*y[2] : used in the modulation
  *mi = used in interleaving
  *e = used to store the taus sequence (taus sequence is used to generate the first sequence for DCI) Turbo coding
  *wbar used in the interleaving and also REG allocation
  */

  //num_pdcch_symbols = get_num_pdcch_symbols(num_ue_spec_dci+num_common_dci,dci_alloc,frame_parms,subframe);


  // generate DCIs in order of decreasing aggregation level, then common/ue spec
  // MAC is assumed to have ordered the UE spec DCI according to the RNTI-based randomization???

  // Value of aggregation level (FAPI/NFAPI specs v.9.0 pag 221 value 1,2)
  for (L=2; L>=1; L--) {
    for (i=0; i<Num_dci; i++) {

    	//XXX should be checked how the scheduler store the aggregation level for NB-IoT (value 1-2 or 0-1)
      if (dci_alloc[i].L == (uint8_t)L) {

        if (dci_alloc[i].firstCCE>=0) {


          //NB-IoT encoding
          /*npdcch_encoding_NB_IoT(dci_alloc[i].dci_pdu,
                                 frame_parms,
                                 npdcch, //see when function dci_top is called
                                 //no frame
                                subframe
                                //rm_stats, te_stats, i_stats
                                );*/

        }
      }
    }

  }


  //NB-IoT scrambling
  /*
   *
   * TS 36.213 ch 16.6.1
   * npdcch_start_index  indicate the starting OFDM symbol for NPDCCH in the first slot of a subframe k ad is determined as follow:
   * - if eutracontrolregionsize is present (defined for in-band operating mode (mode 0,1 for FAPI specs))
   * 	npdcch_start_index = eutracontrolregionsize (value 1,2,3) [units in number of OFDM symbol]
   * -otherwise
   * 	npdcch_start_index = 0
   *
   *Depending on npddch_start_index then we define different values for G
   */

  //XXX the setting of this npdcch_start_index parameter should be done in the MAC
//  if(fp->operating_mode == 0 || fp->operating_mode == 1) //in-band operating mode
//  {
//	  npdcch_start_index = fp->control_region_size;
//  }
//  else
//  {
//	  npdcch_start_index = 0;
//  }

  for(int i = 0; i <Num_dci; i++)
  {

	  switch(dci_alloc[i].npdcch_start_symbol) //mail Bcom matthieu
	  {
  	  	  case 0:
  	  		  G = 304;
  		 	break;
  	  	  case 1:
  	  		  G = 240;
  	  		  break;
  	  	  case 2:
  	  		  G = 224;
  	  		  break;
  	  	  case 3:
  	  		  G =200;
  	  		  break;
  	  	  default:
  	  		  LOG_E (PHY,"npdcch_start_index has unwanted value\n");
  	  		  break;
	  }



//  	  	  // NB-IoT scrambling
//  	  	  npdcch_scrambling_NB_IoT(
//  	  	              frame_parms,
//  	  				  npdcch,
//  	  				  //G,
//  	  				  //q = nf mod 2 (TS 36.211 ch 10.2.3.1)  with nf = number of frame
//  	  				  //slot_id
//  	  	                    );


  }



//  //NB-IoT modulation
//  npdcch_modulation_NB_IoT(
//      txdataF,
//      AMP,
//      frame_parms,
//      //no symbol
//      //npdcch0???
//      //RB_ID --> statically get from the higher layer (may included in the dl_frame params)
//      );




  //in NB-IoT the interleaving is done directly with the encoding procedure
  //there is no interleaving because we don't apply turbo coding


  // This is the REG allocation algorithm from 36-211
  //already done in the modulation in our NB-IoT implementaiton??
 
 //*******************************************************************************************************
 //*******************************************************************************************************
 //*******************************************************************************************************
 //*******************************************************************************************************
 //*******************************************************************************************************

  return 0;
}
