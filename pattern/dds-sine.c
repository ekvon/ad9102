#include "ad9102.h"

#include <math.h>

/*
* Configure DDS sine wave as DAC output.
* Pattern type is continuous so pattern period and start delay are not required.
*/

extern ad9102_reg_t ad9102_reg[AD9102_REG_NUM];
extern ad9102_map_t ad9102_map;

/*
* Params:
* @f_clkp - clock frequency for DAC (Hz)
* @f_out - output frequncy of sine wave (Hz)
*/
void ad9102_pattern_dds_sine(uint32_t f_clkp,uint32_t f_out){
	uint32_t DDS_TW;
	int i;
	
	/*	PAT_TYPE (0x1e): PARTTERN_RPT-Pattern continuously runs	*/
	i=ad9102_map[PAT_TYPE];
	ad9102_reg[i].value&=~0x1;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	WAV_CONFIG (0x27): WAVE_SEL[1:0]-Prestored waveform, PRESTORE_SEL[5:4]-DDS output	*/
	i=ad9102_map[WAV_CONFIG];
	ad9102_reg[i].value&=~0x3;
	ad9102_reg[i].value&=~(0x3<<4);
	/*	saw generator is used	*/
	ad9102_reg[i].value|=(0x3<<4)|0x1;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	DDS_TW32	*/
	/*	PLL clock is used as CLKP clock (without prescalers)	*/
	DDS_TW=ad9102_dds_tw(f_out,f_clkp);
	i=ad9102_map[DDS_TW32];
	/*	store two highest bytes	*/
	ad9102_reg[i].value=(DDS_TW&0xffff00)>>8;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	/*	DDS_TW1	*/
	i=ad9102_map[DDS_TW1];
	/*	store the lower byte of tuning word in the highest byte of the register	*/
	ad9102_reg[i].value|=(DDS_TW&0xff)<<8;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
}

/*
* Calculation of tuning word for specified clock frequency and output frequncy.
* The following formula is used (see at p.23)
		f_out=DDS_TW*f_clkp/2^24
* Params:
* @f_out - output frequncy of sine wave
* @f_clkp - clock frequency
*/
uint32_t ad9102_dds_tw(uint32_t f_out,uint32_t f_clkp){
	float ratio;
	/*	tuning word	*/
	uint32_t DDS_TW;
	
	/*	f_dds=DDS_TW*f_clkp/(2^24) (p.23)	*/
	ratio=(1.0*f_out)/f_clkp;
	DDS_TW=(uint32_t)(0x01000000*ratio);
	return DDS_TW;
}
