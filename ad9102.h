#ifndef __AMUNGO__MODEM__AD_9102__H
#define __AMUNGO__MODEM__AD_9102__H

#include <stdint.h>

#define AD9102_REG_NUM 0x22
#define AD9102_REG_MAX_ADDR 0x60
#define AD9102_REG_BASE	0x0
#define AD9102_REG(Addr) AD9102_REG_BASE+Addr
#define AD9102_SPI_BUF_SIZE 0xf
#define AD9102_SRAM_BASE_ADDR 0x6000
#define AD9102_SRAM_SIZE 0x1000

#define SPICONFIG 0x00
#define POWERCONFIG 0x01
#define CLOCKCONFIG 0x02
#define REFADJ 0x03
#define DACAGAIN 0x07
#define DACRANGE 0x08
#define DACRSET 0x0c
#define CALCONFIG 0x0d
#define COMPOFFSET 0x0e
#define RAMUPDATE 0x1d
#define PAT_STATUS 0x1e
#define PAT_TYPE 0x1f
#define PATTERN_DLY 0x20
#define DACDOF 0x25
#define WAV_CONFIG 0x27
#define PAT_TIMEBASE 0x28
#define PAT_PERIOD 0x29
#define DAC_PAT 0x2b
#define DOUT_START 0x2c
#define DOUT_CONFIG 0x2d
#define DAC_CST 0x31
#define DAC_DGAIN 0x35
#define SAW_CONFIG 0x37
#define DDS_TW32 0x3e
#define DDS_TW1 0x3f
#define DDS_PW 0x43
#define TRIG_TW_SEL 0x44
#define DDS_CONFIG 0x45
#define TW_RAM_CONFIG 0x47
#define START_DELAY 0x5c
#define START_ADDR 0x5d
#define STOP_ADDR 0x5e
#define DDS_CYC 0x5f
#define CFG_ERROR 0x60

#define AD9102_Reset_High GPIOA->BSRR&=~GPIO_BSRR_BR8;\
	GPIOA->BSRR|=GPIO_BSRR_BS8
#define AD9102_Reset_Low GPIOA->BSRR&=~GPIO_BSRR_BS8;\
	GPIOA->BSRR|=GPIO_BSRR_BR8
	
#define AD9102_Trigger_High GPIOB->BSRR&=~GPIO_BSRR_BR12;\
	GPIOB->BSRR|=GPIO_BSRR_BS12		
#define AD9102_Trigger_Low GPIOB->BSRR&=~GPIO_BSRR_BS12;\
	GPIOB->BSRR|=GPIO_BSRR_BR12	
	
typedef uint8_t ad9102_map_t[AD9102_REG_MAX_ADDR+1];
	
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

/*	calibration result	*/
typedef struct ad9102_cal_res
{
	uint16_t dac_rset_cal;
	uint16_t dac_gain_cal;
} ad9102_cal_res_t;

typedef struct ad9102_dds_param
{
	uint32_t f_clkp;
	/*	zero frequency (frequency of dds_compiler)	*/
	uint32_t f_zero;
	/*	filling frequency (impulse filling frequency)	*/
	uint32_t f_fill;
	/*	pattern period (s)	*/
	float pattern_period;
	/*	start delay relative pattern period	*/
	float start_delay;
	/*	*/
	uint32_t num_clkp_pattern;
	/*	*/
	uint32_t num_clkp_delay;
	/*	number of DDS cycles inside the pattern	*/
	uint16_t dds_cyc_out;
	/*	number of DDS cycles after heterodin	*/
	uint16_t dds_cyc_fill;
} ad9102_dds_param_t;

void ad9102_map_init();
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
/*	DDS tuning word calculation	*/
uint32_t ad9102_dds_tw(uint32_t f_dds,uint32_t f_clkp);
/*	calibration procedure	*/
int ad9102_calibration();
/*	patterns configuration	*/
void ad9102_pattern_dds_sine(uint32_t f_clkp,uint32_t f_out);
void ad9102_pattern_dds_period(ad9102_dds_param_t * param);
void ad9102_pattern_dds_ram(ad9102_dds_param_t * param);
#endif
