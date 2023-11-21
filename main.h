#ifndef __AMUNGO__MODEM__MAIN__H
#define __AMUNGO__MODEM__MAIN__H

#include <stm32f4xx.h>

#include <stddef.h>
#include <stdint.h>

#define MODEM_SUCCESS 0
#define MODEM_ERROR -1

#define MODEM_CHAR_BUF_SIZE 0x400

enum led_t
{
	MODEM_LED_B13=13,
	MODEM_LED_B14=14
};

/*	supported baudrate values	*/
enum usart_br_t
{
	MODEM_USART_B9600=9600,
	MODEM_USART_B19200=19200,
	MODEM_USART_B115200=115200
};

typedef struct stm32_usart_br_param
{
	/*	input settings	*/
	uint32_t f_ck;
	uint32_t br;
	uint8_t over8;
	/*	calculated parameters	*/
	uint16_t mantissa;
	uint8_t fraction;
	uint32_t reg;
} stm32_usart_br_param_t;

/*	setting for PLL clock	*/
typedef struct stm32_pll
{
	/*	input settings (should be filled by the user)	*/
	uint32_t f_in;
	uint8_t P;
	uint8_t M;
	uint16_t N;
	/*	output frequency at specified settings	*/
	uint32_t f_out;
	/*	RCC_PLLCFGR value	*/
	uint32_t reg;
} stm32_pll_t;

typedef struct spi_data
{
	/*	ad9102 register address	*/
	uint16_t addr;
	uint16_t value;
	/*	registers values	*/
	char tx_buf[MODEM_CHAR_BUF_SIZE];
	char rx_buf[MODEM_CHAR_BUF_SIZE];
} spi_data_t;

/*	RCC initialization	*/
int stm32_rcc_init();
/*	define value of RCC_PLLCFGR at specified settings	*/
int stm32_rcc_pll_init(stm32_pll_t * stm32_pll);
/*	switch to PLLCLK; before calling this function struct stm32_pll should be initialized with the help of 'stm32_rcc_pll_init'	*/
void stm32_rcc_switch(stm32_pll_t * stm32_pll);

/*	LED pins initialization	*/
int stm32_led_init();
/*	blink specified number of times	*/
void stm32_led13_blink(uint8_t count,uint32_t delay);
void stm32_led14_blink(uint8_t count,uint32_t delay);
/*	time delay	*/
void dummy_loop(uint32_t delay);

/*	USART initialization	*/
int stm32_usart_init(stm32_usart_br_param_t * usart_br);
/*	define value of USART->BRR at specified settings	*/
int stm32_usart_br_init(stm32_usart_br_param_t * usart_br);
/*	send data using USART1	*/
int stm32_usart_tx(int8_t * data,size_t len);
/*	receive data using USART1	*/
int stm32_usart_rx(int8_t * data,size_t len);

/*	stm32 SPI module interface	*/
int stm32_spi_init(uint8_t ssm_enable);
/*	transmit only procedure for BIDIMODE=0 RXONLY=0 (rm0383 p.573)	*/
int stm32_spi_send(int8_t * data,uint16_t size,uint8_t mode);
/*	receive only procedure for BIDIMODE=0 RXONLY=1 (rm0383 p.575)	*/
int stm32_spi_recv(int8_t * data,uint16_t size,uint8_t mode);
/*	full-duplex transmit procedure for BIDIMODE=0 RXONLY=0 (rm0383 p.572)	*/
int stm32_spi_transaction(int8_t * tx_buf,int8_t * rx_buf,uint16_t size,uint8_t mode);
/*	chip select management in SSM enable mode	*/
void stm32_spi_cs_high();
void stm32_spi_cs_low();

#endif
