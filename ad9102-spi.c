#include "ad9102.h"

#include <stm32f4xx.h>

#include <memory.h>

extern int stm32_spi_send(int8_t*,uint32_t);
extern int stm32_spi_recv(int8_t*,uint32_t);
extern int stm32_spi_transaction(int8_t*,int8_t*,uint16_t);
extern void stm32_spi_cs_high();
extern void stm32_spi_cs_low();

extern ad9102_spi_data_t spi_data;

/*	internal procedure	*/
void ad9102_prepare_spi_data(uint16_t * data){
	if(spi_data.rw){
		/*	read (ignore pointer for data)	*/
		spi_data.addr|=0x8000;
		if(AD9102_SPI_BUF_SIZE-1<spi_data.size){
			/*	restrict number of half-words to receive	*/
			spi_data.size=AD9102_SPI_BUF_SIZE-1;
		}
		/*	nulling buffers	*/
		memset(spi_data.tx_buf,0,AD9102_SPI_BUF_SIZE*sizeof(uint16_t));
		memset(spi_data.rx_buf,0,AD9102_SPI_BUF_SIZE*sizeof(uint16_t));
		/*	copy address to send	*/
		memcpy(spi_data.tx_buf,&spi_data.addr,sizeof(uint16_t));
	}
	else{
		if(!data)
			return;
		/*	write	*/
		spi_data.addr&=0x7fff;
		if(AD9102_SPI_BUF_SIZE-1<spi_data.size){
			/*	restrict number of half-words to write	*/
			spi_data.size=AD9102_SPI_BUF_SIZE-1;
		}
		/*	nulling buffers	*/
		memset(spi_data.tx_buf,0,AD9102_SPI_BUF_SIZE*sizeof(uint16_t));
		memset(spi_data.rx_buf,0,AD9102_SPI_BUF_SIZE*sizeof(uint16_t));
		/*	copy address to send	*/
		memcpy(spi_data.tx_buf,&spi_data.addr,sizeof(spi_data.addr));
		/*	copy data to send	*/
		memcpy(((uint8_t*)spi_data.tx_buf)+sizeof(uint16_t),(uint8_t*)data,spi_data.size*sizeof(uint16_t));
	}
}

void ad9102_read_reg(uint16_t addr,uint16_t buf[],uint8_t size){
	/*	data register value and offset	*/
	uint8_t dr,off;
	uint8_t lo;
	
	if(!size)
		return;
	/*	nulling recv buffer	*/
	memset((uint8_t*)buf,0,size*sizeof(uint16_t));
	/*	offset in bits (not in use)	*/
	lo=0;
	off=0;
	addr|=0x8000;
	/*	enable SPI and start transaction	*/
	SPI1->CR1|=SPI_CR1_SPE;
	stm32_spi_cs_low();
	/*	send the first byte of address	*/
	SPI1->DR=(uint8_t)(addr>>8);
	/*	don't receive any data after the first sended byte	*/
	while(!(SPI1->SR&SPI_SR_TXE)){
	}
	/*	send the second byte of address	*/
	SPI1->DR=(uint8_t)(addr&0x00ff);
	/*	receive the high byte of the first value	*/
	while(!(SPI1->SR&(SPI_SR_RXNE))){
	}
	dr=(uint8_t)(SPI1->DR);
	/*	ignore the first received word	*/
	/*	*(buf+off)|=(dr<<8);	*/
	lo=1;
	/*	receive others value	*/
	for(;off<size;){
		/*	wait DR becomes free	*/
		while(!(SPI1->SR&SPI_SR_TXE)){
		}
		/*	send dummy data	*/
		SPI1->DR=0;
		/*	wait data	*/
		while(!(SPI1->SR&(SPI_SR_RXNE))){
		}
		/*	receive the low byte of the first value	*/
		dr=(uint8_t)(SPI1->DR);
		if(lo){
			if(0<off)
				/*	the low byte is received	*/
				*(buf+off)|=dr;
			/*	receive the hight byte	*/
			lo=0;
			/*	read the next value	*/
			off+=1;
		}
		else{
			/*	the high byte is received	*/
			*(buf+off)|=(dr<<8);
			/*	receive the low byte	*/
			lo=1;
		}
	}
	while(!(SPI1->SR&(SPI_SR_TXE))){
	}
	while(SPI1->SR&(SPI_SR_BSY));
	/*	finish transaction and disable SPI	*/
	stm32_spi_cs_high();
	SPI1->CR1&=~SPI_CR1_SPE;
}

void ad9102_read_reg16(uint16_t addr,uint16_t buf[],uint8_t size){
	uint16_t off;
	
	off=0;
	addr|=0x8000;
	/*	enable SPI and start transaction	*/
	SPI1->CR1|=SPI_CR1_SPE;
	stm32_spi_cs_low();
	/*	send address	*/
	SPI1->DR=addr;
	while(size--){
		/*	send dummy data to provide the clock	*/
		while(!(SPI1->SR&SPI_SR_TXE)){
		}
		SPI1->DR=0;
		/*	receive data	*/
		while(!(SPI1->SR&(SPI_SR_RXNE))){
		}
		*(buf+off)=(uint16_t)(SPI1->DR);
		/*	'buf' pointer to uint16_t	*/
		off+=1;
	}
	while(!(SPI1->SR&(SPI_SR_TXE))){
	}
	while(SPI1->SR&(SPI_SR_BSY)){
	}
	/*	finish transaction and disable SPI	*/
	stm32_spi_cs_high();
	SPI1->CR1&=~SPI_CR1_SPE;
}

void ad9102_write_reg(uint16_t addr,uint16_t value){
	/*	used to read data register	*/
	uint8_t dr;
	
	/*	clear the highest bit	*/
	addr&=0x7fff;
	/*	enable SPI and start transaction	*/
	SPI1->CR1|=SPI_CR1_SPE;
	stm32_spi_cs_low();
	while(!(SPI1->SR&SPI_SR_TXE)){
	}
	/*	send the high byte of address	*/
	SPI1->DR=(uint8_t)(addr>>8);
	while(!(SPI1->SR&SPI_SR_TXE)){
	}
	/*	send the low byte of address	*/
	SPI1->DR=(uint8_t)(addr&0xff);
	/*	read data register (full-duplex mode)	*/
	/*
	while(!(SPI1->SR&(SPI_SR_RXNE))){
	}
	dr=(SPI1->DR);
	*/
	while(!(SPI1->SR&SPI_SR_TXE)){
	}
	/*	send the high byte of value	*/
	(SPI1->DR)=(uint8_t)(value>>8);
	/*	read data register (full-duplex mode)	*/
	/*
	while(!(SPI1->SR&(SPI_SR_RXNE))){
	}
	dr=(SPI1->DR);
	*/
	while(!(SPI1->SR&SPI_SR_TXE)){
	}
	/*	send the low byte of value	*/
	(SPI1->DR)=(uint8_t)(value&0xff);
	/*	read data register (full-duplex mode)	*/
	/*
	while(!(SPI1->SR&(SPI_SR_RXNE))){
	}
	dr=(SPI1->DR);
	*/
	while(!(SPI1->SR&SPI_SR_TXE)){
	}
	while(SPI1->SR&(SPI_SR_BSY)){
	}
	(void)SPI1->DR;
	(void)SPI1->SR;
	/*	finish transaction and disable SPI	*/
	stm32_spi_cs_high();
	SPI1->CR1&=~SPI_CR1_SPE;
}

void ad9102_write_reg16(uint16_t addr,uint16_t value){
	/*	clear the highest bit	*/
	addr&=0x7fff;
	/*	enable SPI and start transaction	*/
	SPI1->CR1|=SPI_CR1_SPE;
	stm32_spi_cs_low();
	/*	send address	*/
	SPI1->DR=addr;
	while(!(SPI1->SR&SPI_SR_TXE)){
	}
	SPI1->DR=value;
	while(!(SPI1->SR&(SPI_SR_RXNE))){
	}
	while(!(SPI1->SR&SPI_SR_TXE)){
	}
	(void)SPI1->DR;
	(void)SPI1->SR;
	/*
	while(SPI1->SR&(SPI_SR_BSY)){
	}
	*/
	/*	finish transaction and disable SPI	*/
	stm32_spi_cs_high();
	SPI1->CR1&=~SPI_CR1_SPE;
}
