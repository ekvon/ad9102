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
static void _ad9102_prepare_spi(uint16_t * data){
	if(spi_data.rw){
		/*	read (ignore pointer for data)	*/
		spi_data.addr|=0x8000;
		if(AD9102_SPI_BUF_SIZE-1<spi_data.size)
			spi_data.size=AD9102_SPI_BUF_SIZE-1;
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
		if(AD9102_SPI_BUF_SIZE-1<spi_data.size)
			spi_data.size=AD9102_SPI_BUF_SIZE-1;
		memset(spi_data.tx_buf,0,AD9102_SPI_BUF_SIZE*sizeof(uint16_t));
		memset(spi_data.rx_buf,0,AD9102_SPI_BUF_SIZE*sizeof(uint16_t));
		/*	copy address to send	*/
		memcpy(spi_data.tx_buf,&spi_data.addr,sizeof(spi_data.addr));
		/*	copy data to send	*/
		memcpy((uint8_t*)spi_data.tx_buf+sizeof(uint16_t),(uint8_t*)data,spi_data.size*sizeof(uint16_t));
	}
}

uint16_t ad9102_read_reg(uint16_t addr){
	spi_data.rw=1;
	spi_data.addr=addr;
	/*
		SPI module works in 8-bit data frame mode.
		In the first and in the second bytes address is sended so the first two bytes in rx-buffer must be empty.
		Try to any receive data within 3 16-bit cycles (or 6 8-bit cycles).
		So the full size of transaction is 8 byte.
	*/
	spi_data.size=0x3;
	/*	prepare buffers for transaction	*/
	_ad9102_prepare_spi(0);
	/*	try to receive data in full-duplex mode	*/
	stm32_spi_transaction((uint8_t*)spi_data.tx_buf,(uint8_t*)spi_data.rx_buf,(spi_data.size+1)*sizeof(uint16_t));
	/*	received data are in buffer	*/
  return 0;
}

void ad9102_write_reg(uint16_t addr,uint16_t value){
	spi_data.rw=0;
	spi_data.addr=addr;
	/*	write the only register	*/
	spi_data.size=1;
	/*	prepare buffers for transaction	*/
	_ad9102_prepare_spi(&value);
	/*	transaction (send data and address)	*/
	stm32_spi_transaction((uint8_t*)spi_data.tx_buf,(uint8_t*)spi_data.rx_buf,(spi_data.size+1)*sizeof(uint16_t));
}

int32_t ad9102_cmsis_read_reg(uint16_t addr){
	int32_t value;
	/*	data register value and offset	*/
	uint8_t dr,off;
	/*	counter	*/
	uint8_t i;
	
	value=0;
	off=24;
	i=0;
	addr|=0x8000;
	/*	enable SPI and start transaction	*/
	SPI1->CR1|=SPI_CR1_SPE;
	stm32_spi_cs_low();
	/*	send the first byte of address	*/
	SPI1->DR=(uint8_t)(addr>>8);
	/*	don't receive any data after the first sended byte	*/
	while(!(SPI1->SR&SPI_SR_TXE));
	/*	send the second byte of address	*/
	SPI1->DR=(uint8_t)(addr&0x00ff);
	/*	receive the first byte of response	*/
	while(!(SPI1->SR&(SPI_SR_RXNE)));
	dr=(SPI1->DR);
	/*	ignore the first byte	*/
	/*	value|=(dr<<off);	*/
	/*	receive 3 more bytes	*/
	for(i=0;i<5;i++){
		/*	wait DR becomes free	*/
		while(!(SPI1->SR&SPI_SR_TXE));
		/*	send dummy data	*/
		SPI1->DR=0;
		/*	wait data	*/
		while(!(SPI1->SR&(SPI_SR_RXNE))){
		}
		/*	read data	*/
		dr = (SPI1->DR);
		if(!i)
			/*	ignore the second byte	*/
			continue;
		value|=(dr<<off);
		if(off)
			off-=8;
	}
	/*	just in case	*/
	while(!(SPI1->SR&(SPI_SR_RXNE))){
	}
	while(SPI1->SR&(SPI_SR_BSY));
	/*	finish transaction and disable SPI	*/
	stm32_spi_cs_high();
	SPI1->CR1&=~SPI_CR1_SPE;
	return value;
}
