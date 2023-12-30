#include "ad9102.h"

#include <math.h>

/*
* Configure DAC output as prestored waveform using START_DELAY and PATTERN_PERIOD.
* DDS output (sine wave) is used as prestored waveform.
*	Tuning words for DDS are stored in SRAM. The following format of tuning word are used:
*		DDSTW={RAM[13:0],10'b0}
*	The base frequency is 23437500 Hz (DDSTW: 0x960000). Tuning word increment (decrement) is 0x1000.
* Frequency increment (decrement) corresponding this change of tuning word is about 9766 Hz. 100 different frequencies are used.
*/

extern uint16_t spi_rx_buf[AD9102_SPI_BUF_SIZE];
extern ad9102_reg_t ad9102_reg[AD9102_REG_NUM];
extern ad9102_map_t ad9102_map;

/*
	Params:
*/
void ad9102_pattern_dds_tw(ad9102_dds_tw_t * param){
	/*	number of clocks in pattern period	*/
	uint32_t num_clkp_pattern;
	/*	number of clocks in start delay	*/
	uint32_t num_clkp_delay;
	/*	tuning word for specified frequency	*/
	int dds_tw;
	/*	*/
	int f,tw;
	/*	start delay interval (s)	*/
	float start_delay_time;
	/*	time interval on which DDS is active	*/
	float dds_play_time;
	/*	*/
	float ratio;
	int i,j;
	
	/*	number of clocks inside the only pattern period	*/
	num_clkp_pattern=(uint32_t)(param->f_clkp*param->pattern_period);
	/*	start delay in seconds	*/
	start_delay_time=param->pattern_period*param->start_delay;
	/*	number of clocks inside the start delay	*/
	num_clkp_delay=(uint32_t)(param->f_clkp*start_delay_time);
	/*	*/
	dds_play_time=param->pattern_period-start_delay_time;

	/*	PARTTERN_RPT-Pattern continuously runs	*/
	i=ad9102_map[PAT_TYPE];
	ad9102_reg[i].value&=~0x1;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	used when pattern repeats finite number of times	*/
	/*
	i=ad9102_map[DAC_PAT];
	ad9102_reg[i].value|=0xff;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	*/
	
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
	ad9102_reg[i].value&=~(0xf<<4);
	ad9102_reg[i].value|=(0xf<<4);
	ad9102_reg[i].value|=(0xf<<8);
	/*	START_DELAY_BASE	*/
	ad9102_reg[i].value&=~(0xf);
	ad9102_reg[i].value|=(0xf);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	i=ad9102_map[PAT_PERIOD];
	/*	store number of clocks divided on base number	*/
	ad9102_reg[i].value=(uint16_t)(((num_clkp_pattern/0xf)&0xffff));
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);

	i=ad9102_map[START_DELAY];
	/*	store number of clocks divided on base number	*/
	ad9102_reg[i].value=(uint16_t)(((num_clkp_delay/0xf)&0xffff));
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	establish the start and stop address	*/
	i=ad9102_map[START_ADDR];
	ad9102_reg[i].value&=~(0xfff<<4);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	i=ad9102_map[STOP_ADDR];
	ad9102_reg[i].value&=~(0xfff<<4);
	ad9102_reg[i].value|=0xfff;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	configure DDS tuning word in SRAM	*/
	i=ad9102_map[DDS_CONFIG];
	ad9102_reg[i].value|=(0x1<<2);
	ad9102_reg[i].value|=0x1;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	i=ad9102_map[TW_RAM_CONFIG];
	/*	DDSTW={RAM[13:0],10'b0}	*/
	ad9102_reg[i].value&=~0x1f;
	/*	DDSTW={DDSTW[23:22],RAM[13:0],8'b0}	*/
	ad9102_reg[i].value|=0x2;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	number of DDS cycles inside the only pattern period (not in use)	*/
	i=ad9102_map[DDS_CYC];
	ad9102_reg[i].value=0x1;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
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
	
	/*	index of DDS_TW1 register	*/
	j=ad9102_map[DDS_TW32];
	
	/*	store tuning words in SRAM	*/
	for(i=0;i<=param->x_num;i++){
		if(param->is_tw){
			tw=param->x_zero+i*param->x_inc;
			tw&=0xffffff;
			/*	store 2-bit in DDS_TW32 highest bits	*/
			ad9102_reg[j].value=(tw&0xc00000)>>8;
			ad9102_write_reg(ad9102_reg[j].addr,ad9102_reg[j].value);
			/*	store next 14-bit in SRAM	*/
			ad9102_write_reg(param->start_addr+i,(((tw&0x3fffff)>>8)<<2));
		}
		else{
			/*	current frequency	*/
			f=param->x_zero+i*param->x_inc;
			/*	tuning word for current frequency	*/
			ratio=(1.0*f)/param->f_clkp;
			tw=(int)(0x1000000*ratio);
			tw&=0xffffff;
			/*	store 2-bit in DDS_TW32 highest bits	(mode 1)	*/
			ad9102_reg[j].value=(tw&0xc00000)>>8;
			ad9102_write_reg(ad9102_reg[j].addr,ad9102_reg[j].value);
			/*	store next 14-bit in SRAM	*/
			ad9102_write_reg(param->start_addr+i,(((tw&0x3fffff)>>8)<<2));
			
			/*	store highest 14-bit in SRAM (mode 0)	*/
			/*	ad9102_write_reg(param->start_addr+i,((tw>>10)<<2));	*/
		}
	}
	
	/*	disable access to memory from SPI	*/
	i=ad9102_map[PAT_STATUS];
	ad9102_reg[i].value&=~(0x1<<2);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
}
