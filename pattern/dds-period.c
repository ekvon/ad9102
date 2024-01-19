#include "ad9102.h"

#include <math.h>

/*
* Configure DAC output as prestored waveform using START_DELAY and PATTERN_PERIOD.
* DDS output (sine wave) is used as prestored waveform.
*/

extern ad9102_reg_t ad9102_reg[AD9102_REG_NUM];
extern ad9102_map_t ad9102_map;

/*
	Params:
*/
void ad9102_pattern_dds_period(ad9102_dds_param_t * param){
	/*	tuning word	*/
	uint32_t DDS_TW;
	/*	*/
	uint32_t f_out;
	/*	start delay interval (s)	*/
	float start_delay_time;
	/*	time interval on which DDS is active	*/
	float dds_play_time;
	int i;
	
	/*	number of clocks inside the only pattern period	*/
	param->num_clkp_pattern=(uint32_t)(param->f_clkp*param->pattern_time);
	/*	start delay in seconds	*/
	param->start_delay_time=param->pattern_time*param->start_delay_ratio;
	/*	number of clocks inside the start delay	*/
	param->num_clkp_delay=(uint32_t)(param->f_clkp*param->start_delay_time);
	/*	*/
	param->play_time=param->pattern_time-param->start_delay_time;
	if(!param->dds_cyc){
		/*	number of cycles at specified zero and filling frequncies	*/
		f_out=param->f_zero+param->f_fill;
		param->dds_cyc=(uint16_t)(f_out*param->play_time);
	}
	else{
		/*	DDS filling frequency at specified number of cycles	and zero frequency	*/
		f_out=(uint32_t)(1.0*param->dds_cyc/dds_play_time);
		param->f_fill=f_out-param->f_zero;
	}

	/*	PARTTERN_RPT-Pattern continuously runs	*/
	i=ad9102_map[PAT_TYPE];
	ad9102_reg[i].value&=~0x1;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	i=ad9102_map[WAV_CONFIG];
	/*	prestored waveform using START_DELAY and PATTERN_PERIOD	*/
	ad9102_reg[i].value&=~0x3;
	ad9102_reg[i].value|=0x2;
	/*	DDS output as prestored waveform	*/
	ad9102_reg[i].value&=~(0x3<<4);
	ad9102_reg[i].value|=(0x3<<4);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	i=ad9102_map[PAT_TIMEBASE];
	/*	PATTERN_PERIOD_BASE	*/
	ad9102_reg[i].value&=~(0xf<<4);
	ad9102_reg[i].value|=(0xf<<4);
	/*	START_DELAY_BASE	*/
	ad9102_reg[i].value&=~(0xf);
	ad9102_reg[i].value|=(0xf);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	i=ad9102_map[PAT_PERIOD];
	/*	store number of clocks divided on base number	*/
	ad9102_reg[i].value=(uint16_t)(((param->num_clkp_pattern/0xf)&0xffff));
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);

	i=ad9102_map[START_DELAY];
	/*	store number of clocks divided on base number	*/
	ad9102_reg[i].value=(uint16_t)(((param->num_clkp_delay/0xf)&0xffff));
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	configure DDS tuning word (system clock established manually)	*/
	DDS_TW=ad9102_dds_tw(f_out,param->f_clkp);
	i=ad9102_map[DDS_TW32];
	ad9102_reg[i].value=(DDS_TW&0xffff00)>>8;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	i=ad9102_map[DDS_TW1];
	ad9102_reg[i].value|=(DDS_TW&0xff)<<8;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	number of DDS cycles inside the only pattern period	*/
	i=ad9102_map[DDS_CYC];
	ad9102_reg[i].value=param->dds_cyc;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
}
