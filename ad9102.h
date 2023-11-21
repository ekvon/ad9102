#ifndef __AMUNGO__MODEM__AD_9102__H
#define __AMUNGO__MODEM__AD_9102__H

#include <stdint.h>

#define AD9102_REG_NUM 0x21
#define AD9102_REG_BASE	0x0
#define AD9102_REG(Addr) AD9102_REG_BASE+Addr
#define AD9102_SPI_BUF_SIZE 0xf

#define AD9102_Reset_High GPIOA->BSRR&=~GPIO_BSRR_BR8;\
	GPIOA->BSRR&=~GPIO_BSRR_BS8
#define AD9102_Reset_Low GPIOA->BSRR&=~GPIO_BSRR_BS8;\
	GPIOA->BSRR&=~GPIO_BSRR_BR8
	
typedef struct ad9102_reg
{
	const char * name;
	uint16_t addr;
	uint16_t value;
} ad9102_reg_t;

typedef struct ad9102_spi_data
{
	/*	1-read;0-write	*/
	uint8_t rw;
	uint16_t addr;
	/*	number of half words to send/recv	starting from specified address	*/
	uint8_t size;
	/*	buffers for data	*/
	uint16_t tx_buf[AD9102_SPI_BUF_SIZE];
	uint16_t rx_buf[AD9102_SPI_BUF_SIZE];
} ad9102_spi_data_t; 

/*	prepare SPI buffers for communication with using stm32 functions	*/
void ad9102_prepare_spi_data(uint16_t * data);
/*	SPI communication using CMSIS directly (8-bit)	*/
void ad9102_read_reg(uint16_t addr,uint16_t buf[],uint8_t size);
/*	SPI communication using CMSIS directly (16-bit)	*/
void ad9102_read_reg16(uint16_t addr,uint16_t buf[],uint8_t size);
/*	SPI communication using CMSIS directly (8-bit)	*/
void ad9102_write_reg(uint16_t addr,uint16_t value);
/*	SPI communication using CMSIS directly (16-bit)	*/
void ad9102_write_reg16(uint16_t addr,uint16_t value);
#endif
