#ifndef __AMUNGO__MODEM__AD_9102__H
#define __AMUNGO__MODEM__AD_9102__H

#include <stdint.h>

#define AD9102_REG_BASE	0x0
#define AD9102_REG(Addr) AD9102_REG_BASE+Addr
#define AD9102_SPI_BUF_SIZE 0xf

#define SPICONFIG 0x0
#define POWERCONFIG 0x1
#define CLOCKCONFIG 0x2
#define REFADJ 0x3
#define DACAGAIN 0x7
#define DACRANGE 0x8
#define DACRSET 0xc 

#define AD9102_Reset_High GPIOA->BSRR&=~GPIO_BSRR_BR8;\
	GPIOA->BSRR&=~GPIO_BSRR_BS8
#define AD9102_Reset_Low GPIOA->BSRR&=~GPIO_BSRR_BS8;\
	GPIOA->BSRR&=~GPIO_BSRR_BR8

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

/*	SPI communication using stm32 functions	*/
uint16_t ad9102_read_reg(uint16_t addr);
void ad9102_write_reg(uint16_t addr,uint16_t value);
/*	SPI communication using CMSIS directly (experimental)	*/
int32_t ad9102_cmsis_read_reg(uint16_t addr);

#endif
