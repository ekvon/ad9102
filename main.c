#include "main.h"
#include "ad9102.h"

#include <memory.h>
#include <stdio.h>

#define SYSTEM_CORE_CLOCK 40000000

/*	global variables	*/
char buf[MODEM_CHAR_BUF_SIZE];
uint16_t spi_rx_buf[AD9102_SPI_BUF_SIZE];
uint16_t pattern_count;
uint8_t SysTick_Delay;

ad9102_spi_data_t spi_data;
extern ad9102_reg_t ad9102_reg[AD9102_REG_NUM];
extern ad9102_map_t ad9102_map;

/*	not in use	*/
void SysTick_Handler(){
	SysTick_Delay=0;
	/*	just disable the timer	*/
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
	uint32_t f_out,f_clkp,DDS_TW;
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
	
	/*	*/
	/*
	i=ad9102_map[CLOCKCONFIG];
	ad9102_reg[i].value|=(0x1<<3);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	*/
	
	/*	'Vref' level	*/
	i=ad9102_map[REFADJ];
	ad9102_reg[i].value&=~0x3f;
	ad9102_reg[i].value|=0x1f;
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
	ad9102_reg[i].value=0x1f;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*
	* DAC digital gain control.
	* The value is '1' like in 3rd party projects.
	*/
	i=ad9102_map[DAC_DGAIN];
	ad9102_reg[i].value|=(0xc00<<4);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	DDS output using PATTERN_PERIOD and START_DELAY	*/
	ad9102_dds_param_t param;
	param.f_clkp=40000000;
	param.f_zero=12152000;
	param.f_fill=10000;
	param.dds_cyc_out=0;
	/*	1/256	*/
	param.pattern_period=0.0032768;
	/*	delay is half of pattern period	*/
	param.start_delay=0.5;
	
	/*	DDS output modulated by waveform from RAM	*/
	ad9102_pattern_dds_ram(&param);
	
	/*	sine wave as DAC output	*/
	/*	ad9102_pattern_dds_sine(SYSTEM_CORE_CLOCK,param.f_zero+param.f_fill);	*/
	
	sprintf(buf,"ad9102_pattern_dds: number of cycles is %u\n",param.dds_cyc_out);
	stm32_usart_tx(buf,0);
	
	/*	PARTTERN_RPT: Pattern repeats finite number of times	*/
	i=ad9102_map[PAT_TYPE];
	ad9102_reg[i].value=0x1;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	used when pattern repeats finite number of times (play the only pattern)	*/
	i=ad9102_map[DAC_PAT];
	ad9102_reg[i].value&=~0xffff;
	ad9102_reg[i].value|=0x1;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	/*	PATTERN_DLY: set minimal value	*/
	i=ad9102_map[PATTERN_DLY];
	ad9102_reg[i].value&=~0xffff;
	ad9102_reg[i].value|=0x1;
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
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
	/*	initialize SysTickTimer but don't start it	*/
	SysTick->LOAD  = (uint32_t)(param.num_clkp_pattern);
  NVIC_SetPriority (SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL);
  SysTick->VAL   = 0UL;
  SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
                   SysTick_CTRL_TICKINT_Msk;
	
	pattern_count=0x6;
	while(0<pattern_count){
		param.f_fill+=10000;
		f_out=param.f_zero+param.f_fill;
		/*	save new tuning word value to shadow registers	*/
		/*
		DDS_TW=ad9102_dds_tw(f_out,param.f_clkp);
		i=ad9102_map[DDS_TW32];
		ad9102_reg[i].value&=~0xffff;
		ad9102_reg[i].value=(DDS_TW&0xffff00)>>8;
		ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
		i=ad9102_map[DDS_TW1];
		ad9102_reg[i].value&=~0xffff;
		ad9102_reg[i].value|=(DDS_TW&0xff)<<8;
		ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
		*/
		/*	establish RUN bit	*/
		/*
		i=ad9102_map[PAT_STATUS];
		ad9102_reg[i].value|=0x1;
		ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
		*/
		AD9102_Trigger_Low;
		/*	wait for pattern begining	*/
		while(1){
			/*	clear SPI buffer	*/
			memset(spi_rx_buf,0,AD9102_SPI_BUF_SIZE);	
			ad9102_read_reg(PAT_STATUS,spi_rx_buf,2);
			/*	check the RUN bit (it MUST be established)	*/
			if(!(spi_rx_buf[1]&0x1))
				continue;
			if((spi_rx_buf[1]&0x2))
				break;
		}
		SysTick_Delay=1;
		/*	start SysTickHandler	*/
		SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;
		while(SysTick_Delay){
		}
		/*	wait for pattern end	*/
		while(1){
			/*	clear SPI buffer	*/
			memset(spi_rx_buf,0,AD9102_SPI_BUF_SIZE);	
			ad9102_read_reg(PAT_STATUS,spi_rx_buf,2);
			/*	check the RUN bit (it MUST be established)	*/
			if(!(spi_rx_buf[1]&0x1))
				continue;
			if(!(spi_rx_buf[1]&0x2))
				break;
		}
		/*	turn off pattern generator and update active registers automatically	*/
		AD9102_Trigger_High;
		/*	clear RUN bit	*/
		/*
		i=ad9102_map[PAT_STATUS];
		ad9102_reg[i].value&=~0x1;
		ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
		*/
		/*	explicitly update registers	*/	
		/*
		i=ad9102_map[RAMUPDATE];
		ad9102_reg[i].value=0x1;
		ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
		*/
		/*	*/
		pattern_count--;
	}
	
	/*	turn off pattern generator and update active registers automatically	*/
	AD9102_Trigger_High;
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
	
	/*	PAT_STATUS: clear RUN bit and enable read access to SRAM	*/
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
	
	/*	0x6001	*/
	ad9102_read_reg(0x6001,spi_rx_buf,2);
	sprintf(buf,"SRAM: addr=0x6001, value={0x%.4x 0x%.4x}\n",
		spi_rx_buf[0],
		spi_rx_buf[1]
	);
	stm32_usart_tx(buf,0);
	
	/*	0x6fff	*/
	ad9102_read_reg(0x6fff,spi_rx_buf,2);
	sprintf(buf,"SRAM: addr=0x6fff, value={0x%.4x 0x%.4x}\n",
		spi_rx_buf[0],
		spi_rx_buf[1]
	);
	stm32_usart_tx(buf,0);
	/*	disable memory access from SPI port	*/
	i=ad9102_map[PAT_STATUS];
	ad9102_reg[i].value&=~0x1;
	ad9102_reg[i].value&=~(0x3<<2);
	ad9102_write_reg(ad9102_reg[i].addr,ad9102_reg[i].value);
	
	stm32_usart_tx("Programm is finished\n",0);
	/*	programm is finished (blink with the green led)	*/
	stm32_led13_blink(0x8,0xffff);
}
