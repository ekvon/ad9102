#include "ad9102.h"

#include <math.h>

/*
* Configure DAC output as prestored waveform using START_DELAY and PATTERN_PERIOD modulated by waveform from RAM.
* This pattern is based on pattern where dds used with START_DELAY and PATTERN_PERIOD.
* Additional data (magnitude of DDS output) are stored in SRAM.
* 
* The following procedure is used to define parameters of the pattern:
* User must specify clock frequency (f_clkp), frequency of FPGA dds_compiler (f_zero), impulse filling frequency (f_fill),
* pattern period and start delay. Start delay is defined relative to pattern period.
* Pattern period, f_zero and f_fill defines DDS output frequency (f_out):
*		f_out=f_zero+f_fill	(1)
* the number of clocks required to produce the only DDS sine wave:
*		f_clkp*(f_out/2^24)	(2)
*	the number of clocks inside the only pattern period:
*		f_clkp*t_play=f_clkp*(t_pattern-t_delay)	(3)
* the number of sine waves inside the only pattern period
*		f_clkp*f_out=f_clkp*(f_zero+f_fill)	(4)
*/

extern uint16_t spi_rx_buf[AD9102_SPI_BUF_SIZE];
extern ad9102_reg_t ad9102_reg[AD9102_REG_NUM];
extern ad9102_map_t ad9102_map;

static void ad9102_ram_waveform_ramp_up_saw();
static void ad9102_ram_waveform_ramp_down_saw();
static void ad9102_ram_waveform_triangle_saw();

/*
	Params:
*/
void ad9102_pattern_dds_ram(ad9102_dds_param_t * param){
	/*	tuning word	*/
	uint32_t DDS_TW;
	/*	*/
	uint32_t f_out;
	/*	start delay interval (s)	*/
	float start_delay_time;
	/*	time interval on which DDS is active	*/
	float dds_play_time;
	/*	counter	*/
	int i;
	
	/*	number of clocks inside the only pattern period	*/
	param->num_clkp_pattern=(uint32_t)(param->f_clkp*param->pattern_period);
	/*	start delay in seconds	*/
	start_delay_time=param->pattern_period*param->start_delay;
	/*	number of clocks inside the start delay	*/
	param->num_clkp_delay=(uint32_t)(param->f_clkp*start_delay_time);
	/*	time interval (s) on which DDS is active	*/
	dds_play_time=param->pattern_period-start_delay_time;
	if(!param->dds_cyc_out){
		/*	number of cycles at specified zero and filling frequncies	*/
		f_out=param->f_zero+param->f_fill;
		param->dds_cyc_out=(uint16_t)(f_out*dds_play_time);
		param->dds_cyc_fill=(uint16_t)(param->f_fill*dds_play_time);
	}
	else{
		/*
		* DDS filling frequency at specified number of cycles	and zero frequency
		* Not realized yet. The number of cycles must be specified in cycles of filling frequency.
		*/
	}
	
	i=ad9102_map[WAV_CONFIG];
	/*	prestored waveform using START_DELAY and PATTERN_PERIOD modulated by waveform in SRAM	*/
	ad9102_reg[i].value&=~0x3;
	ad9102_reg[i].value|=0x3;
	/*	DDS output as prestored waveform	*/
	ad9102_reg[i].value&=~(0x3<<4);
	ad9102_reg[i].value|=(0x3<<4);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	i=ad9102_map[PAT_TIMEBASE];
	/*	PATTERN_PERIOD_BASE	*/
	ad9102_reg[i].value|=(0xf<<4);
	ad9102_reg[i].value|=(0xf<<4);
	ad9102_reg[i].value|=(0xf<<8);
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
	ad9102_reg[i].value=param->dds_cyc_out;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	establish the start and stop address	*/
	i=ad9102_map[START_ADDR];
	ad9102_reg[i].value&=~(0xfff<<4);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	i=ad9102_map[STOP_ADDR];
	ad9102_reg[i].value&=~(0xfff<<4);
	ad9102_reg[i].value|=(0xfff<<4);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	enable DDS MSB	*/
	/*
	i=ad9102_map[DDS_CONFIG];
	ad9102_reg[i].value|=(0x1<<2);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	*/
	
	/*	enable access to SRAM from SPI	*/
	i=ad9102_map[PAT_STATUS];
	ad9102_reg[i].value&=~0x1;
	ad9102_reg[i].value|=(0x1<<2);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	update registers	*/
	i=ad9102_map[RAMUPDATE];
	ad9102_reg[i].value=0x1;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	/*	wait to apply changes	*/
	while(1){
		ad9102_read_reg(RAMUPDATE,spi_rx_buf,2);
		if(!(spi_rx_buf[1]&0x1))
			break;
		dummy_loop(0xff);
	}
	
	/*	create waveform in sram	*/
	ad9102_ram_waveform_triangle_saw();
	/*	ad9102_ram_waveform_ramp_down_saw();	*/
	
	/*	disable access to memory	*/
	i=ad9102_map[PAT_STATUS];
	ad9102_reg[i].value&=~(0x1<<2);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
}

void ad9102_ram_waveform_ramp_up_saw(){
	const int max_sram_value=0x3fff;
	const int sram_linear_step=0x3fff/AD9102_SRAM_SIZE;
	
	int value;
	int i;
	
	/*	fill in SRAM with aplitude values	*/
	for(i=0;i<0x1000;i++){
		value=i*sram_linear_step;
		ad9102_write_reg(AD9102_SRAM_BASE_ADDR+i,(value<<2));
	}
}

void ad9102_ram_waveform_ramp_down_saw(){
	const int max_sram_value=0x3fff;
	const int sram_linear_step=0x3fff/AD9102_SRAM_SIZE;
	
	int value;
	int i;
	
	/*	fill in SRAM with aplitude values	*/
	for(i=0;i<AD9102_SRAM_SIZE;i++){
		value=i*sram_linear_step;
		ad9102_write_reg(AD9102_SRAM_BASE_ADDR+i,(value<<2));
	}
}

void ad9102_ram_waveform_triangle_saw(){
	const int max_sram_value=0x3fff;
	const int sram_linear_step=0x3fff/AD9102_SRAM_SIZE;
	
	int value;
	int i;
	
	/*	fill in SRAM with aplitude values	*/
	for(i=0;i<0x800;i++){
		value=i*sram_linear_step;
		ad9102_write_reg(AD9102_SRAM_BASE_ADDR+i,(value<<2));
	}
	
	for(;i<0x1000;i++){
		value=(0x1000-i)*sram_linear_step;
		ad9102_write_reg(AD9102_SRAM_BASE_ADDR+i,(value<<2));
	}
}
