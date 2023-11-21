#include <stm32f4xx.h>

#include <memory.h>
#include <stdio.h>

#include "main.h"
#include "ad9102.h"

/*	global variables	*/
char buf[MODEM_CHAR_BUF_SIZE];
ad9102_spi_data_t spi_data;
extern ad9102_reg_t ad9102_reg[AD9102_REG_NUM];

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

	/*
	AD9102_Reset_High;
	AD9102_Reset_Low;
	dummy_loop(0xff);
	AD9102_Reset_High;
	*/

	/*
	rv=ad9102_read_reg(addr);
	sprintf(buf,"Register: addr=0x%.4x, rx_buf=0x%.4x 0x%.4x 0x%.4x\n",
		addr,
		spi_data.rx_buf[2],
		spi_data.rx_buf[3],
		spi_data.rx_buf[4],
		spi_data.rx_buf[5],
		spi_data.rx_buf[6],
		spi_data.rx_buf[7]);
	stm32_usart_tx(buf,0);
	*/
	
	/*	registers test	*/
	/*	enable to write to SRAM	*/
	ad9102_write_reg(0x1e,0x0006);
	ad9102_write_reg(0x07,0x000f);
	for(i=0;i<AD9102_REG_NUM;i++){
		memset(rx_buf,0,rx_buf_size*sizeof(uint16_t));
		ad9102_read_reg(ad9102_reg[i].addr,rx_buf,2);
		sprintf(buf,"Register: addr=0x%.4x, value={0x%.4x 0x%.4x 0x%.4x 0x%.4x}\n",
			ad9102_reg[i].addr,
			rx_buf[0],
			rx_buf[1],
			rx_buf[2],
			/*	register reset value	*/
			ad9102_reg[i].value
		);
		stm32_usart_tx(buf,0);
	}
	
	/*	system memory test	*/
	ad9102_write_reg(0x6001,0x0007);
	ad9102_read_reg(0x6001,rx_buf,2);
	sprintf(buf,"SRAM: addr=0x6001, value={0x%.4x 0x%.4x 0x%.4x}\n",
		rx_buf[0],
		rx_buf[1],
		rx_buf[2]
	);
	stm32_usart_tx(buf,0);
	
	stm32_usart_tx("Programm is finished\n",0);
	/*	programm is finished	*/
	stm32_led13_blink(0x8,0xffff);
}

