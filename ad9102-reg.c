#include "ad9102.h"

#include <memory.h>

ad9102_map_t ad9102_map;

ad9102_reg_t ad9102_reg[AD9102_REG_NUM]={
	{"SPICONFIG",0x00,0x0000},
	{"POWERCONFIG",0x01,0x0000},
	{"CLOCKCONFIG",0x02,0x0000},
	{"REFADJ",0x03,0x0000},
	{"DACAGAIN",0x07,0x0000},
	{"DACRANGE",0x08,0x0000},
	{"DACRSET",0x0c,0x000a},
	{"CALCONFIG",0x0d,0x0000},
	{"COMPOFFSET",0x0e,0x0000},
	{"RAMUPDATE",0x1d,0x0000},
	{"PAT_STATUS",0x1e,0x0000},
	{"PAT_TYPE",0x1f,0x0000},
	{"PATTERN_DLY",0x20,0x0001},
	{"DACDOF",0x25,0x0000},
	{"WAV_CONFIG",0x27,0x0000},
	{"PAT_TIMEBASE",0x28,0x0111},
	{"PAT_PERIOD",0x29,0x8000},
	{"DAC_PAT",0x2b,0x0101},
	{"DOUT_START",0x2c,0x0003},
	{"DOUT_CONFIG",0x2d,0x0000},
	{"DAC_CST",0x31,0x0000},
	{"DAC_DGAIN",0x35,0x0000},
	{"SAW_CONFIG",0x37,0x0000},
	{"DDS_TW32",0x3e,0x0000},
	{"DDS_TW1",0x3f,0x0000},
	{"DDS_PW",0x43,0x0000},
	{"TRIG_TW_SEL",0x44,0x0000},
	{"DDS_CONFIG",0x45,0x0000},
	{"TW_RAM_CONFIG",0x47,0x0000},
	{"START_DELAY",0x5c,0x0000},
	{"START_ADDR",0x5d,0x0000},
	{"STOP_ADDR",0x5e,0x0000},
	{"DDS_CYC",0x5f,0x0001},
	{"CFG_ERROR",0x60,0x0000}
};

void ad9102_map_init(){
	uint8_t i=0;
	uint16_t addr;
	
	memset(ad9102_map,0,(AD9102_REG_MAX_ADDR+1)*sizeof(uint8_t));
	for(i=0;i<AD9102_REG_NUM;i++){
		addr=ad9102_reg[i].addr;
		ad9102_map[addr]=i;
	}
}
