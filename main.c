#include "main.h"
#include "ad9102.h"

#include <memory.h>
#include <stdio.h>

#define SYSTEM_CORE_CLOCK 40000000

/*	global variables	*/
char buf[MODEM_CHAR_BUF_SIZE];
uint16_t spi_rx_buf[AD9102_SPI_BUF_SIZE];

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
	/*	used to define DDS_TW	*/
	uint32_t f_dds,f_clkp,DDS_TW;
	/*	calibration result	*/
	ad9102_cal_res_t cal_res;
	
	/*	PLLCLK settings	*/
	stm32_pll_t stm32_pll;
	/*	USART baudrate settings	*/
	stm32_usart_br_param_t usart_br;

	stm32_rcc_init();
	/*
	* PLLCLK settings.
	* HSE (MHz) is used as source clock. Function SystemCoreClockUpdate() return invalid value.
	* At this configuration PLL frequency is 40MHz.
	*/
	stm32_pll.f_in=8000000;
	stm32_pll.P=4;
	stm32_pll.M=8;
	stm32_pll.N=160;
	if(stm32_rcc_pll_init(&stm32_pll)<0)
		/*	PLLCLK configuration error	*/
		return;
	/*	switch to PLLCLK	*/
	stm32_pll.sw=1;
	stm32_rcc_pll(&stm32_pll);
	/*	get current value of SYSCLK (invalid value is returned)	*/
	SystemCoreClockUpdate();
		
	stm32_led_init();
	
	/*	manually set SystemCoreClock (see above)	*/
	usart_br.f_ck=/*SystemCoreClock*/SYSTEM_CORE_CLOCK;
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
	* SPI1_NSS pin will be moved manually.
	*/
	if(stm32_spi_init(1)<0){
		sprintf(buf,"SPI configuration error\n");
		stm32_usart_tx(buf,0);
		return;
	}
	/*
	* Hardware reset of ad9102 (the only time).
	* It's disabled as software reset with the help of SPI port is used.
	/*
	AD9102_Reset_High;
	AD9102_Reset_Low;
	AD9102_Reset_High;
	*/
	AD9102_Trigger_High;
	/*	mapping register address on index value	*/
	ad9102_map_init();
	/*	software reset procedure	*/
	i=ad9102_map[SPICONFIG];
	ad9102_reg[i].value=0x2004;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);		
	/*	update registers values	*/
	i=ad9102_map[RAMUPDATE];
	ad9102_reg[i].value=0x1;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	/*	wait acknowledge	*/
	while(1){
		ad9102_read_reg(RAMUPDATE,spi_rx_buf,2);
		if(!(spi_rx_buf[1]&0x1))
			break;
		dummy_loop(0xff);
	}
	
	stm32_usart_tx("Registers reset values:\n",0);
	/*	read and store reset values of registers	*/
	for(i=0;i<AD9102_REG_NUM;i++){
		memset(spi_rx_buf,0,AD9102_SPI_BUF_SIZE*sizeof(uint16_t));
		ad9102_read_reg(ad9102_reg[i].addr,spi_rx_buf,2);
		sprintf(buf,"%d: addr=0x%.4x, value={0x%.4x 0x%.4x %s}\n",
			i,
			ad9102_reg[i].addr,
			/*	first value is not used	*/
			spi_rx_buf[0],
			/*	register reset value	*/
			spi_rx_buf[1],
			ad9102_reg[i].name
		);
		/*	store reset value	*/
		ad9102_reg[i].value=spi_rx_buf[1];
		stm32_usart_tx(buf,0);
	}
	
	/*
	* Calibration procedure.
	* At this moment it's not used.
	* The amplitude of output signal is established with the help of DACRSET and DACAGAIN registers (see datasheet).
	*/ 
	/*
	if(calibration(&cal_res)<0){
		stm32_usart_tx("Calibration is failed\n",0);
		return;
	}
	else{
		sprintf(buf,"Calibration result: gain=%u, rset=%u\n",cal_res.dac_gain_cal,cal_res.dac_rset_cal);
		stm32_usart_tx(buf,0);
	}*/
	
	/*	Configure of ad9102	*/
	i=ad9102_map[SPICONFIG];
	/*	default SPI settings	*/
	ad9102_reg[i].value=0;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	'Vref' level	*/
	i=ad9102_map[REFADJ];
	ad9102_reg[i].value&=~0x3f;
	ad9102_reg[i].value|=0x1e;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*
	* Enable internal Rset resistor and set it's value to minimum.
	* Then 'Iref' has the maximum value with specified 'Vref' (see above).
	* Calibration data are not used. 
	*/
	i=ad9102_map[DACRSET];
	/*	ad9102_reg[i].value=0x800c;	*/
	ad9102_reg[i].value=0x8001;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*
	* DAC gain control (maximum value).
	* It doesn't impact on amplitude of output signal (it's not understand).
	* The calibration value is 0x1f or 0x20. 
	*/
	i=ad9102_map[DACAGAIN];
	ad9102_reg[i].value&=~0x7f;
	ad9102_reg[i].value|=0x1f;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*
	* DAC digital gain control.
	* The value is '1' like in 3rd party projects.
	*/
	i=ad9102_map[DAC_DGAIN];
	ad9102_reg[i].value|=(0xc00<<4);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	*/
	i=ad9102_map[DACDOF];
	ad9102_reg[i].value|=(0xfff<<4);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	sine wave as DAC output	*/
	/*	ad9102_pattern_dds_sine(SYSTEM_CORE_CLOCK,23416000);	*/
	
	/*	DDS output using PATTERN_PERIOD and START_DELAY	*/
	ad9102_dds_tw_t param;
	param.f_clkp=40000000;
	param.start_addr=0x6000;
	/*	fill the all memory with the same value	*/
	param.is_tw=1;
	/*	tw	(i=16, f=12308249)	*/
	/*	tw	(i=32, f=12619629)	*/
	param.x_zero=0x4ec400;
	/*	tw increment	*/
	param.x_inc=0;
	/*	number of tw	*/
	param.x_num=0xfff;
	/*	1/256	*/
	param.pattern_period=/*	0.00390625	*/0.0002048;
	/*	delay is half of pattern period	*/
	param.start_delay=0.5;
	
	ad9102_pattern_dds_tw(&param);
	
	/*	PAT_STATUS (0x1f): RUN=1	*/
	i=ad9102_map[PAT_STATUS];
	ad9102_reg[i].value|=0x1;
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
	
	/*	play the specified pattern	*/
	AD9102_Trigger_Low;
	/*	time delay for gnuradio	*/
	dummy_loop(0xfffffff);
	/*	stop play the pattern (the RUN bit is not cleared)	*/
	AD9102_Trigger_High;
	/*	just in case	*/
	dummy_loop(0xff);

	/*	debug output	*/
	stm32_usart_tx("\n",0);
	stm32_usart_tx("Registers updated values\n",0);	
	/*	check the registers	values	*/
	for(i=0;i<AD9102_REG_NUM;i++){
		memset(spi_rx_buf,0,AD9102_SPI_BUF_SIZE*sizeof(uint16_t));
		ad9102_read_reg(ad9102_reg[i].addr,spi_rx_buf,2);
		sprintf(buf,"Register: addr=0x%.4x, value={0x%.4x 0x%.4x 0x%.4x %s}\n",
			ad9102_reg[i].addr,
			spi_rx_buf[0],
			spi_rx_buf[1],
			/*	register value	*/
			ad9102_reg[i].value,
			ad9102_reg[i].name
		);
		stm32_usart_tx(buf,0);
	}
	/*	enable access to SRAM in read mode	*/
		/*	enable access to SRAM from SPI	*/
	i=ad9102_map[PAT_STATUS];
	ad9102_reg[i].value&=~0x1;
	ad9102_reg[i].value|=(0x3<<2);
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
	
	/*	check SRAM values (first and last address)	*/
	ad9102_read_reg(0x6000,spi_rx_buf,2);
	sprintf(buf,"SRAM: addr=0x6000, value={0x%.4x 0x%.4x}\n",
		spi_rx_buf[0],
		spi_rx_buf[1]
	);
	stm32_usart_tx(buf,0);
	ad9102_read_reg(0x6fff,spi_rx_buf,2);
	sprintf(buf,"SRAM: addr=0x6fff, value={0x%.4x 0x%.4x}\n",
		spi_rx_buf[0],
		spi_rx_buf[1]
	);
	stm32_usart_tx(buf,0);
	
	stm32_usart_tx("Programm is finished\n",0);
	/*	programm is finished (blink with the green led)	*/
	stm32_led13_blink(0x8,0xffff);
}
