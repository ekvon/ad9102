#include "main.h"

#include <memory.h>

int stm32_spi_init(uint8_t ssm_enable){	
	/*	AF for PA5 (AF5)	*/
	GPIOA->MODER|=GPIO_MODER_MODER5_1;
	/*	AF for PA6 (AF5)	*/
	GPIOA->MODER|=GPIO_MODER_MODER6_1;
	/*	AF for PA7 (AF5)	*/
	GPIOA->MODER|=GPIO_MODER_MODER7_1;
	/*	AF5 for PA5 (SPI1_SCK)	*/
	GPIOA->AFR[0]|=GPIO_AFRL_AFSEL5_0|GPIO_AFRL_AFSEL5_2;
	/*	AF5 for PA6 (SPI1_MISO)	*/
	GPIOA->AFR[0]|=GPIO_AFRL_AFSEL6_0|GPIO_AFRL_AFSEL6_2;
	/*	AF5 for PA7 (SPI1_MOSI)	*/
	GPIOA->AFR[0]|=GPIO_AFRL_AFSEL7_0|GPIO_AFRL_AFSEL7_2;
	/*	set output speed fast for all SPI pins	*/
	/*	GPIOA->OSPEEDR|=(0x2<<10)|(0x2<<12)|(0x2<<14);	*/
	/*	pull-down for PA5, PA6, PA7	*/
	/*	GPIOA->PUPDR|=(1<<10)|(1<12)|(1<14);	*/
	/*	BIDIMODE=0 RXONLY=0	(full-duplex)	*/
	SPI1->CR1&=~SPI_CR1_BIDIMODE;
	SPI1->CR1&=~SPI_CR1_RXONLY;
	/*	CPOL=0 CPHA=0	*/
	SPI1->CR1|=SPI_CR1_CPOL;
	SPI1->CR1|=SPI_CR1_CPHA;
	/*	set clock to fPCLK/2	*/
	SPI1->CR1&=~(1<<3);
	SPI1->CR1&=~(1<<4);
	SPI1->CR1&=~(1<<5);
	/*	set MSB first	*/
	SPI1->CR1&=~SPI_CR1_LSBFIRST;
	/*	set mode to MASTER	*/
	SPI1->CR1|=SPI_CR1_MSTR;
	/*	set 16 bit data mode	*/
	SPI1->CR1&=~SPI_CR1_DFF;
	/*	select software slave management (if flag is established)	*/
	if(ssm_enable){
		SPI1->CR1|=SPI_CR1_SSM;
		SPI1->CR1|=SPI_CR1_SSI;
		/*	general output for SPI1_NSS	*/
		GPIOA->MODER|=GPIO_MODER_MODER4_0;
		/*	set CS pin high	*/
		GPIOA->BSRR&=~GPIO_BSRR_BR4;
		GPIOA->BSRR|=GPIO_BSRR_BS4;
	}
	else{
		/*	not in use (has not been tested yet)	*/
		SPI1->CR1&=~SPI_CR1_SSM;
		SPI1->CR2|=SPI_CR2_SSOE;
		/*	AF for PA4 (AF5)	*/
		GPIOA->MODER|=GPIO_MODER_MODER4_1;
		/*	AF5 for PA4 (SPI1_NSS)	*/
		GPIOA->AFR[0]|=GPIO_AFRL_AFSEL4_0|GPIO_AFRL_AFSEL4_2;
		/*	output speed for PA4	*/
		GPIOA->OSPEEDR|=(0x3<<8);
	}
	/*	SPI module is disabled and MUST be enabled at each transaction	*/
	/*	check fault	*/
	if(SPI1->SR&SPI_SR_MODF)
		return MODEM_ERROR;
	return MODEM_SUCCESS;
}
 
int stm32_spi_send(int8_t * data,uint16_t size,uint8_t mode){
	uint16_t i;
	uint8_t off;

	off=0;
	/*	enable SPI module	*/
	SPI1->CR1|=SPI_CR1_SPE;
	/*	start transmission	*/
	stm32_spi_cs_low();
	for(i=0;i<size;i++){
		/*	wait until TXE is set	*/
		while(!(SPI1->SR&(SPI_SR_TXE))){
		}
		if(mode){
			/*	16-bit data frame	*/
			/*	write half-word to the data register	*/
			SPI1->DR=(uint16_t)*(data+off);
			off+=2;
		}
		else{
			/*	write byte to the data register	*/
			SPI1->DR=*(data+off);
			off+=1;
		}
	}
	/*	wait until TXE is set	*/
	while(!(SPI1->SR&(SPI_SR_TXE))){
	}
	/*	wait for BUSY flag to reset	*/
	while((SPI1->SR&(SPI_SR_BSY))){
	}
	/*	clear OVR flag	*/
	(void)SPI1->DR;
	(void)SPI1->SR;
	/*	finish transmission	*/
	stm32_spi_cs_low();
	/*	disable	SPI	*/
	SPI1->CR1&=~SPI_CR1_SPE;
	return (int)size;
}

int stm32_spi_recv(int8_t * data,uint16_t size,uint8_t mode){
	uint16_t off;
	uint16_t dr;
	
	off==0;
	/*	switch to RXONLY mode	*/
	SPI1->CR1|=SPI_CR1_RXONLY;
	/*	enable SPI (the clock MUST be started at this time)	*/
	SPI1->CR1|=SPI_CR1_SPE;
	/*	start transmission	*/
	stm32_spi_cs_low();
	while(size--){
		/*	wait data	*/
		while(!(SPI1->SR&(SPI_SR_RXNE))){
		}
		if(mode){
			/*	16-bit data frame (store half-word)	*/
			dr=(SPI1->DR);
			memcpy(data+off,&dr,sizeof(uint16_t));
			off+=2;
		}
		else{
			/*	8-bit data frame (store byte)	*/
			*(data+off)=(uint8_t)(SPI1->DR);
			off+=1;
		}
	}
	/*	end of transmission	*/
	stm32_spi_cs_high();
	/*	disable SPI	*/
	SPI1->CR1&=~SPI_CR1_SPE;
	/*	backtrack to BIDIMODE=0 RXONLY=0	*/
	SPI1->CR1&=~SPI_CR1_RXONLY;
	return (int)size;
}

int stm32_spi_transaction(int8_t * tx_buf,int8_t * rx_buf,uint16_t size,uint8_t mode){
	uint16_t off;
	uint16_t dr;
	if(tx_buf==NULL||rx_buf==NULL||!size)
		return -1;
		
	off=0;
	/*	nulling receive	buffer	*/
	if(mode){
		/*	16-bit data frame	*/
		memset(rx_buf,0,size*sizeof(uint16_t));
	}
	else{
		/*	8-bit data frame	*/
			memset(rx_buf,0,size);
	}
	/*	enable SPI and start transaction	*/
	SPI1->CR1|=SPI_CR1_SPE;
	stm32_spi_cs_low();
	/*	send the first data frame	*/
	if(mode){
		/*	16-bit	*/
		SPI1->DR=(uint16_t)*(tx_buf+off);
		/*	don't receive any data	*/
		off+=2;
	}
	else{
		/*	8-bit	*/
		SPI1->DR=*(tx_buf+off);
		/*	don't receive any data	*/
		off+=1;
	}
	/*	*/
	while(--size){
		if(mode){
			/*	16-bit	*/
			while(!(SPI1->SR&SPI_SR_TXE)){
			}
			SPI1->DR=(uint16_t)*(tx_buf+off);
			while(!(SPI1->SR&(SPI_SR_RXNE))){
			}
			dr=(uint16_t)(SPI1->DR);
			*(rx_buf+off)=dr;
			off+=2;
		}
		else{
			/*	8-bit	*/
			while(!(SPI1->SR&SPI_SR_TXE)){
			}
			SPI1->DR=*(tx_buf+off);
			while(!(SPI1->SR&(SPI_SR_RXNE))){
			}
			*(rx_buf+off)=(uint8_t)(SPI1->DR);
			off+=1;
		}
	}
	/*	just in case	*/
	while(!(SPI1->SR&(SPI_SR_TXE))){
	}
	while(SPI1->SR&(SPI_SR_BSY));
	/*	finish transaction and disable SPI	*/
	stm32_spi_cs_high();
	SPI1->CR1&=~SPI_CR1_SPE;
	return (int)size;
}

void stm32_spi_cs_high(){
	if(!(SPI1->CR1&SPI_CR1_SSM))
		/*	software slave management is disabled	*/
		return;
	GPIOA->BSRR&=~GPIO_BSRR_BR4;
	GPIOA->BSRR|=GPIO_BSRR_BS4;
}

void stm32_spi_cs_low(){
	if(!(SPI1->CR1&SPI_CR1_SSM))
		/*	software slave management is disabled	*/
		return;
	GPIOA->BSRR&=~GPIO_BSRR_BS4;
	GPIOA->BSRR|=GPIO_BSRR_BR4;
}
