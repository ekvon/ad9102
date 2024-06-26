Proj_Name=ad9102
Proj_Super_Dir=/home/kvon/W/Projects/radio
Proj_Dir=$(Proj_Super_Dir)/$(Proj_Name)
CMSIS_Dir=$(Proj_Super_Dir)/lib/CMSIS/STM32CubeF4/Drivers/CMSIS
CMSIS_Core_Include=$(CMSIS_Dir)/Core/Include
CMSIS_Device_Include=$(CMSIS_Dir)/Device/ST/STM32F4xx/Include
CMSIS_Device_Source=$(CMSIS_Dir)/Device/ST/STM32F4xx/Source/Templates

# default target
# Target=STM32F407xx
Target=STM32F411xE
Startup_FileName=startup_stm32f411xe
Source_FileName=$(Test_Name)
ifeq ($(Target),STM32F407xx)
    Startup_FileName=startup_stm32f407xx
    Source_FileName=$(Test_Name)-ve
endif
Memory_File=mem.ld

# arm toolchain directory
Arm_Dir=/home/kvon/W/Tools/gcc-arm-none-eabi
# use ARM libraries for specified architecture (see below)
Arm_Gcc_Lib_Dir=$(Arm_Dir)/lib/gcc/arm-none-eabi/10.3.1/thumb/v7e-m/nofp
# +fp/softfp
Arm_Lib_Dir=$(Arm_Dir)/arm-none-eabi/lib/thumb/v7e-m/nofp
# +fp/softfp

# explicit paths to libraries
Lib_Gcc=$(Arm_Gcc_Lib_Dir)/libgcc.a
Lib_C=$(Arm_Lib_Dir)/libc.a
Lib_M=$(Arm_Lib_Dir)/libm.a
Lib_Specs=$(Arm_Lib_Dir)/libc_nano.a

COREFLAGS=-mcpu=cortex-m4 -mthumb
LDFLAGS=$(COREFLAGS) -L$(Arm_Gcc_Lib_Dir) -L$(Arm_Lib_Dir) --specs=nosys.specs
CFLAGS=$(COREFLAGS)


all:$(Proj_Name)

$(Proj_Name):$(Startup_FileName).o system_stm32f4xx.o main.o rcc.o uart.o gpio.o spi.o ad9102-reg.o ad9102-spi.o calibration.o dds-sine.o	dds-period.o dds-ram.o tw_table.o # $(Proj_Name)-as.o pattern.o 
	arm-none-eabi-gcc $(LDFLAGS) -o $(Proj_Name).elf -Tmem.ld $(Startup_FileName).o system_stm32f4xx.o main.o rcc.o uart.o gpio.o spi.o ad9102-reg.o ad9102-spi.o calibration.o dds-sine.o dds-period.o dds-ram.o tw_table.o $(Lib_M) # $(Proj_Name)-as.o
	arm-none-eabi-objcopy -O binary $(Proj_Name).elf $(Proj_Name).bin

$(Startup_FileName).o:
	arm-none-eabi-as -mcpu=cortex-m4 -o $@ $(CMSIS_Device_Source)/gcc/$(Startup_FileName).s
	
system_stm32f4xx.o:
	arm-none-eabi-gcc $(COREFLAGS) -O2 -ffreestanding -Wall -I$(CMSIS_Device_Include) -I$(CMSIS_Core_Include) -c $(CMSIS_Device_Source)/system_stm32f4xx.c -o $@ -g -D$(Target)
		
main.o:
		arm-none-eabi-gcc $(COREFLAGS) -I$(CMSIS_Device_Include) -I$(CMSIS_Core_Include) -c -g -o $@ main.c -D$(Target) -D_DEBUG
rcc.o:
		arm-none-eabi-gcc $(COREFLAGS) -I$(CMSIS_Device_Include) -I$(CMSIS_Core_Include) -c -g -o $@ rcc.c -D$(Target)
uart.o:
		arm-none-eabi-gcc $(COREFLAGS) -I$(CMSIS_Device_Include) -I$(CMSIS_Core_Include) -c -g -o $@ uart.c -D$(Target)
gpio.o:
		arm-none-eabi-gcc $(COREFLAGS) -I$(CMSIS_Device_Include) -I$(CMSIS_Core_Include) -c -g -o $@ gpio.c -D$(Target)
spi.o:
		arm-none-eabi-gcc $(COREFLAGS) -I$(CMSIS_Device_Include) -I$(CMSIS_Core_Include) -c -g -o $@ spi.c -D$(Target)
ad9102-reg.o:
		arm-none-eabi-gcc $(COREFLAGS) -I$(CMSIS_Device_Include) -I$(CMSIS_Core_Include) -c -g -o $@ ad9102-reg.c -D$(Target)
ad9102-spi.o:
		arm-none-eabi-gcc $(COREFLAGS) -I$(CMSIS_Device_Include) -I$(CMSIS_Core_Include) -c -g -o $@ ad9102-spi.c -D$(Target)
calibration.o:
	arm-none-eabi-gcc $(COREFLAGS) -I$(CMSIS_Device_Include) -I$(CMSIS_Core_Include) -c -g -o $@ calibration.c -D$(Target)
tw_table.o:
	arm-none-eabi-gcc $(COREFLAGS) -I$(CMSIS_Device_Include) -I$(CMSIS_Core_Include) -I$(Proj_Dir) -c -g -o $@ pattern/tw_table.c -D$(Target)
dds-sine.o:
	arm-none-eabi-gcc $(COREFLAGS) -I$(CMSIS_Device_Include) -I$(CMSIS_Core_Include) -I$(Proj_Dir) -c -g -o $@ pattern/dds-sine.c -D$(Target)
dds-period.o:
	arm-none-eabi-gcc $(COREFLAGS) -I$(CMSIS_Device_Include) -I$(CMSIS_Core_Include) -I$(Proj_Dir) -c -g -o $@ pattern/dds-period.c -D$(Target)
dds-ram.o:
	arm-none-eabi-gcc $(COREFLAGS) -I$(CMSIS_Device_Include) -I$(CMSIS_Core_Include) -I$(Proj_Dir) -c -g -o $@ pattern/dds-ram.c -D$(Target)			
	
		
# $(Proj_Name)-as.o:
# 	arm-none-eabi-as -mcpu=cortex-m4 -o $@ main.s
	
clean:
	rm *.o *.bin *.elf
