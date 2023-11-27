#include <stm32f4xx.h>

#include <memory.h>
#include <stdio.h>

#include "main.h"
#include "ad9102.h"

/*	global variables	*/
char buf[MODEM_CHAR_BUF_SIZE];
ad9102_spi_data_t spi_data;
extern ad9102_reg_t ad9102_reg[AD9102_REG_NUM];
extern ad9102_map_t ad9102_map;

/*	not in use	*/
void SysTick_Handler(){
	stm32_usart_tx("SysTick_Handler is used\n",0);
	/*	disable the timer	*/
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;
}

void main(void)
{
	int rv,i;
	/*	register address	*/
	uint16_t addr;
	/*	use to write data to the only register	*/
	uint16_t value;
	/*	buffer to receive data from SPI port of ad9102	*/
	const uint8_t rx_buf_size=0x4;
	uint16_t rx_buf[rx_buf_size];
	/*	used to define DDS_TW	*/
	uint32_t f_dds,f_clkp,DDS_TW;
	
	/*	PLLCLK settings	*/
	stm32_pll_t stm32_pll;
	/*	USART baudrate settings	*/
	stm32_usart_br_param_t usart_br;

	stm32_rcc_init();
	/*	settings for PLLCLK	*/
	stm32_pll.f_in=8000000;
	stm32_pll.P=8;
	stm32_pll.M=8;
	stm32_pll.N=160;
	if(stm32_rcc_pll_init(&stm32_pll)<0)
		/*	PLLCLK configuration error	*/
		return;
	/*	stm32_rcc_switch(&stm32_pll);	*/
	/*	get current value of SYSCLK	*/
	SystemCoreClockUpdate();
		
	stm32_led_init();
	
	usart_br.f_ck=SystemCoreClock;
	usart_br.br=MODEM_USART_B9600;
	usart_br.over8=0;
	if(stm32_usart_br_init(&usart_br)){
		return;
	}
	stm32_usart_init(&usart_br);
	sprintf(buf,"System is configured. SystemCoreClock is %li\n",SystemCoreClock);	
	stm32_usart_tx(buf,0);
	/*	
	* SPI initialization.
	* Software slave management is enable.
	* AD9102 will be de-selected between transmissions.
	* SPI1_NSS pin will be moved manually
	*/
	if(stm32_spi_init(1)<0){
		sprintf(buf,"SPI configuration error\n");
		stm32_usart_tx(buf,0);
		return;
	}
	/*	reset ad9102 (the only time)	*/


	AD9102_Reset_High;
	AD9102_Reset_Low;
	AD9102_Reset_High;
	
	AD9102_Trigger_High;
	ad9102_map_init();
	/*	reset values of registers	*/
	for(i=0;i<AD9102_REG_NUM;i++){
		memset(rx_buf,0,rx_buf_size*sizeof(uint16_t));
		ad9102_read_reg(ad9102_reg[i].addr,rx_buf,2);
		sprintf(buf,"Register: addr=0x%.4x, value={0x%.4x 0x%.4x 0x%.4x %s}\n",
			ad9102_reg[i].addr,
			rx_buf[0],
			rx_buf[1],
			/*	register reset value	*/
			ad9102_reg[i].value,
			ad9102_reg[i].name
		);
		/*	store reset value	*/
		ad9102_reg[i].value=rx_buf[1];
		stm32_usart_tx(buf,0);
	}
	
	stm32_usart_tx("\n",0);
	stm32_usart_tx("Registers updated values\n",0);
	/*	configure DAC	*/

	i=ad9102_map[CLOCKCONFIG];
	ad9102_reg[i].value|=0x4;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);

	/*	PAT_TYPE (0x1e): PARTTERN_RPT-Pattern continuously runs	*/
	i=ad9102_map[PAT_TYPE];
	ad9102_reg[i].value&=~0x1;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	WAV_CONFIG (0x27): WAVE_SEL[1:0]-Prestored waveform, PRESTORE_SEL[5:4]-DDS output	*/
	i=ad9102_map[WAV_CONFIG];
	ad9102_reg[i].value&=~0x3;
	ad9102_reg[i].value&=~(0x3<<4);
	ad9102_reg[i].value|=/*	(0x3<<4)|*/0x1;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	for constant value generation (not in use)	*/
	i=ad9102_map[DAC_CST];
	ad9102_reg[i].value|=(0xfff<<4);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	DAC_PAT (0x2b): DAC_REPEAT_CYCLE[7:0]-Oxff	*/
	/*	just in case (as it's not required for continuously pattern)	*/
	/*
	i=ad9102_map[DAC_PAT];
	ad9102_reg[i].value|=0xff;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	*/
	
	/*	DDS_TW32	*/
	f_dds=13500000;
	f_clkp=16000000;
	DDS_TW=ad9102_dds_tw(f_dds,f_clkp);
	i=ad9102_map[DDS_TW32];
	/*	store two highest bytes	*/
	ad9102_reg[i].value=(DDS_TW>>8)&0xffff;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	/*	DDS_TW1	*/
	i=ad9102_map[DDS_TW1];
	/*	store the lower byte of tuning word in the highest byte of the register	*/
	ad9102_reg[i].value|=(DDS_TW<<8)&0x0000ff00;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	enable internal Rset resistor	*/
	i=ad9102_map[DACRSET];
	ad9102_reg[i].value|=0x8000;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	DAC gain control (set maximum value)	*/
	/*
	i=ad9102_map[DACAGAIN];
	ad9102_reg[i].value|=0x3f;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	*/
	
	/*	DAC digital gain control	*/
	i=ad9102_map[DAC_DGAIN];
	/*	set maximum value	*/
	ad9102_reg[i].value|=(0xfff<<4);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	*/
	/*
	i=ad9102_map[DDS_CONFIG];
	ad9102_reg[i].value|=0x4;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	*/
	
	/*	*/
	/*
	i=ad9102_map[DDS_CYC];
	ad9102_reg[i].value=0xffff;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	*/

	/*	programming voltage reference	*/
	/*
	i=ad9102_map[REFADJ];
	ad9102_reg[i].value|=0x20;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	*/
	
	/*	PAT_STATUS (0x1f): RUN=1	*/
	i=ad9102_map[PAT_STATUS];
	ad9102_reg[i].value|=0x1;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	
	dummy_loop(0xfff);
	/*	try to play pattern	*/
	AD9102_Trigger_High;
	AD9102_Trigger_Low;
	/*	check the registers	values	*/
	for(i=0;i<AD9102_REG_NUM;i++){
		memset(rx_buf,0,rx_buf_size*sizeof(uint16_t));
		ad9102_read_reg(ad9102_reg[i].addr,rx_buf,2);
		sprintf(buf,"Register: addr=0x%.4x, value={0x%.4x 0x%.4x 0x%.4x %s}\n",
			ad9102_reg[i].addr,
			rx_buf[0],
			rx_buf[1],
			/*	register value	*/
			ad9102_reg[i].value,
			ad9102_reg[i].name
		);
		stm32_usart_tx(buf,0);
	}
	/*	time delay for gnuradio	*/
	dummy_loop(0xffffff);
	/*	clear the RUN bit	*/
	i=ad9102_map[PAT_STATUS];
	ad9102_reg[i].value&=~0x1;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	/*	stop play the pattern	*/
	AD9102_Trigger_High;
	dummy_loop(0xfff);
	/*	check PATTERN bit	*/
	ad9102_read_reg(PAT_STATUS,rx_buf,2);
		sprintf(buf,"Register: addr=0x%.4x, value={0x%.4x 0x%.4x}\n",
		PAT_STATUS,
		rx_buf[0],
		rx_buf[1]
	);
	stm32_usart_tx(buf,0);
	stm32_usart_tx("Programm is finished\n",0);
	/*	programm is finished	*/
	stm32_led13_blink(0x8,0xffff);
}

uint32_t ad9102_dds_tw(uint32_t f_dds,uint32_t f_clkp){
	float ratio;
	uint32_t DDS_TW;
	
	/*	f_dds=DDS_TW*f_clkp/(2^24) (p.23)	*/
	ratio=(1.0*f_dds)/f_clkp;
	DDS_TW=(uint32_t)(0x01000000*ratio);
	return DDS_TW;
}

