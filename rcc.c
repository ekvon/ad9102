#include "main.h"

int stm32_rcc_init(){
	/*	configure peripherial	*/
	RCC->APB2ENR|=RCC_APB2ENR_USART1EN;
	RCC->AHB1ENR|=RCC_AHB1ENR_GPIOAEN;
	RCC->AHB1ENR|=RCC_AHB1ENR_GPIOBEN;
	RCC->APB2ENR|=RCC_APB2ENR_SPI1EN;
	/*	set AHB prescaler to 16	*/
	RCC->CFGR|=(0x9<<4);
	/*	disable APB2 prescaler (clearing highest bits)	*/
	RCC->CFGR&=~0x00008000;
	return MODEM_SUCCESS;
}

int stm32_rcc_pll_init(stm32_pll_t * stm32_pll){
	uint32_t f_vco_in;
	uint32_t f_vco_out;
	uint8_t pllp_field;

	/*	check input values	*/
	if(stm32_pll->M<2||63<stm32_pll->M)
		return MODEM_ERROR;
	if(stm32_pll->N<50||43<stm32_pll->M)
		return MODEM_ERROR;
	if(stm32_pll->P!=2&&stm32_pll->P!=4&&stm32_pll->P!=6&&stm32_pll->P!=8)
		return MODEM_ERROR;
	
	/*	check intermediate and output frequencies	*/	
	f_vco_in=stm32_pll->f_in/stm32_pll->M;
	if(f_vco_in<1000000||2000000<f_vco_in)
		return MODEM_ERROR;
	f_vco_out=f_vco_in*stm32_pll->N;
	if(f_vco_out<100000000||432000000<f_vco_out)
		return MODEM_ERROR;
	stm32_pll->f_out=f_vco_out/stm32_pll->P;
	if(100000000<stm32_pll->f_out)
		return MODEM_ERROR;
	
	/*	set register value	*/
	switch(stm32_pll->P){
	case(2):{pllp_field=0x0;break;}
	case(4):{pllp_field=0x1;break;}
	case(6):{pllp_field=0x2;break;}
	case(8):{pllp_field=0x3;break;}
	/*	it's impossible	*/
	default:{return MODEM_ERROR;}
	}
	/*	highest bits in reset values; HSI used as source clock	*/
	stm32_pll->reg=0x24000000|(pllp_field<<16)|(stm32_pll->N<<6)|stm32_pll->M;
	return MODEM_SUCCESS;
}

void stm32_rcc_switch(stm32_pll_t * stm32_pll){
	/*	configure PLL clock	*/
	RCC->PLLCFGR=stm32_pll->reg;
	/*	turn on PLLCLK	*/
	RCC->CR|=RCC_CR_PLLON;
	while(!(RCC->CR&RCC_CR_PLLRDY)){
	}
	/*	try to select PLLCLK as source for SYSCLK	*/
	RCC->CFGR&=0xfffffffc;
	RCC->CFGR|=0x2;
	while((RCC->CFGR&0xc)!=0x8){
	}
}
