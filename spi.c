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
	GPIOA->OSPEEDR|=(0x2<<10)|(0x2<<12)|(0x2<<14);
	/*	pull-down for PA5, PA6, PA7	*/
	GPIOA->PUPDR|=(2<<10)|(2<12)|(2<14);
	/*	BIDIMODE=0 RXONLY=0	(full-duplex)	*/
	SPI1->CR1&=~SPI_CR1_BIDIMODE;
	SPI1->CR1&=~SPI_CR1_RXONLY;
	/*	CPOL=0 CPHA=0	*/
	SPI1->CR1&=~SPI_CR1_CPOL;
	SPI1->CR1&=~SPI_CR1_CPHA;
	/*	set clock to fPCLK/2	*/
	SPI1->CR1&=~(1<<3);
	SPI1->CR1&=~(1<<4);
	SPI1->CR1&=~(1<<5);
	/*	set MSB first	*/
	SPI1->CR1&=~SPI_CR1_LSBFIRST;
	/*	set mode to MASTER	*/
	SPI1->CR1|=SPI_CR1_MSTR;
	/*	set 8 bit data mode	*/
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
 
int stm32_spi_send(int8_t * data,uint32_t size){
	uint32_t i;

	/*	enable SPI module	*/
	SPI1->CR1|=SPI_CR1_SPE;
	/*	start transmission	*/
	stm32_spi_cs_low();
	i=0;
	while(i<size){
		/*	wait until TXE is set	*/
		while(!(SPI1->SR&(SPI_SR_TXE))){
		}
		/*	write the data to the data register	*/
		SPI1->DR=data[i];
		i++;
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

int stm32_spi_recv(int8_t * data,uint32_t size){
	/*	switch to RXONLY mode	*/
	SPI1->CR1|=SPI_CR1_RXONLY;
	/*	enable SPI (the clock MUST be started at this time)	*/
	SPI1->CR1|=SPI_CR1_SPE;
	/*	start transmission	*/
	stm32_spi_cs_low();
	while(size){
		/*	wait data	*/
		while(!(SPI1->SR&(SPI_SR_RXNE))){
		}
		/*	read data from data register	*/
		*data++ = (SPI1->DR);
		size--;
	}
	/*	end of transmission	*/
	stm32_spi_cs_high();
	/*	disable SPI	*/
	SPI1->CR1&=~SPI_CR1_SPE;
	/*	backtrack to BIDIMODE=0 RXONLY=0	*/
	SPI1->CR1&=~SPI_CR1_RXONLY;
	return (int)size;
}

int stm32_spi_transaction(int8_t * tx_buf,int8_t * rx_buf,uint16_t size){
	if(tx_buf==NULL||rx_buf==NULL||!size)
		return -1;
		
	/*	enable SPI and start transaction	*/
	SPI1->CR1|=SPI_CR1_SPE;
	stm32_spi_cs_low();
	/*	send the first byte	*/
	SPI1->DR=*tx_buf++;
	/*	don't receive data after first sended byte	*/
	*rx_buf++=0;
	while(--size){
		/*	wait DR becomes free	*/
		while(!(SPI1->SR&SPI_SR_TXE));
		/*	send data	*/
		SPI1->DR=*tx_buf++;
		/*	wait data	*/
		while(!(SPI1->SR&(SPI_SR_RXNE))){
		}
		/*	read data	*/
		*rx_buf++ = (SPI1->DR);
	}
	/*	just in case	*/
	while(!(SPI1->SR&(SPI_SR_RXNE))){
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
