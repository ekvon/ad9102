#include "main.h"

int stm32_led_init(){
	/*	general output for PA8 (DDS_RES)	*/
	GPIOB->MODER|=GPIO_MODER_MODER13_0;
	/*	general output for PB13 (01)	*/
	GPIOB->MODER|=GPIO_MODER_MODER13_0;
	/*	general output for PB14 (01)	*/
	GPIOB->MODER|=GPIO_MODER_MODER14_0;
	/*	pull-down for led pins	*/
	GPIOB->OTYPER|=(0x1<<26)|(0x1<<28);
	/*	turn off	*/
	GPIOB->BSRR=GPIO_BSRR_BR13;
  GPIOB->BSRR=GPIO_BSRR_BR14;
  return MODEM_SUCCESS;
}

void stm32_led13_blink(uint8_t count,uint32_t delay){
	uint8_t i;

	/*	blink	*/
	for(i=0;i<count;i++){
		GPIOB->BSRR|=GPIO_BSRR_BS13;
		GPIOB->BSRR&=~GPIO_BSRR_BR13;
		dummy_loop(delay);
		GPIOB->BSRR&=~GPIO_BSRR_BS13;
		GPIOB->BSRR|=GPIO_BSRR_BR13;
		dummy_loop(delay);
	}
}

void stm32_led14_blink(uint8_t count,uint32_t delay){
	uint8_t i;

	/*	blink	*/
	for(i=0;i<count;i++){
		GPIOB->BSRR|=GPIO_BSRR_BS14;
		GPIOB->BSRR&=~GPIO_BSRR_BR14;
		dummy_loop(delay);
		GPIOB->BSRR&=~GPIO_BSRR_BS14;
		GPIOB->BSRR|=GPIO_BSRR_BR14;
		dummy_loop(delay);
	}
}

void dummy_loop(uint32_t delay){
	while(delay--);
}
