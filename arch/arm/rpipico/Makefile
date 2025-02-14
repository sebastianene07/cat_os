SRC := board_initialize.c startup.c uart.c
OBJS += $(patsubst %.c,%.o,$(SRC))
OBJS += bs2_default.o
PICO_SDK=arch/arm/rpipico/pico-sdk
ARCH_FLAGS:=${CFLAGS} -I$(TOPDIR)/include\
											-I$(TOPDIR)/s_alloc\
											-I./include/\
											-I$(TOPDIR)/sched/include\
											-I$(TOPDIR)/$(PICO_SDK)/src/host/hardware_uart/include/\
											-I$(TOPDIR)/$(PICO_SDK)/src/common/pico_base/include/\
											-I$(TOPDIR)/$(PICO_SDK)/../build/generated/pico_base/\
											-I$(TOPDIR)/$(PICO_SDK)/src/rp2_common/pico_platform/include/\
											-I$(TOPDIR)/$(PICO_SDK)/src/host/pico_platform/include/\
											-I$(TOPDIR)/$(PICO_SDK)/src/rp2040/hardware_structs/include/\
											-I$(TOPDIR)/$(PICO_SDK)/src/rp2_common/hardware_base/include/\
											-I$(TOPDIR)/$(PICO_SDK)/src/rp2040/hardware_regs/include/\
											-I$(TOPDIR)/$(PICO_SDK)/src/rp2_common/hardware_gpio/include/\
											-I$(TOPDIR)/$(PICO_SDK)/lib/tinyusb/lib/CMSIS_5/CMSIS/Core/Include/
LDFLAGS += -Llibbsp.a

SRC_ASM:= context_switch.s
OBJS_ASM:=$(patsubst %.s,%.o,$(SRC_ASM))

all: $(OBJS) $(OBJS_ASM) libbsp.a
	echo "Building arch specific"
	${PREFIX}ar -rc $(TOPDIR)/$(TMP_LIB) $(OBJS) $(OBJS_ASM) libbsp.a

%.o : %.c
	${PREFIX}gcc $(ARCH_FLAGS) -c $< -o $@

%.o : %.s
	${PREFIX}as $(ASM_FLAGS) -c $< -o $@

bs2_default.o: bs2_default.s
	
bs2_default.s: bs2_default.bin
	${PREFIX}as $(ASM_FLAGS) -c $@ -o bs2_default.o

bs2_default.bin: build/pico-sdk/src/rp2_common/boot_stage2/bs2_default.elf
	${PREFIX}objcopy -I elf32-littlearm -O binary build/pico-sdk/src/rp2_common/boot_stage2/bs2_default.elf bs2_default.bin --rename-section .text=.boot2
	python3 pico-sdk/src/rp2_common/boot_stage2/pad_checksum -s 0xffffffff $@ bs2_default.s

build/pico-sdk/src/rp2_common/boot_stage2/bs2_default.elf: libbsp.a

libbsp.a: build bsp.c
	cd build && make -f Makefile && cp libbsp.a ..

build:
	mkdir build && cd build && cmake .. && cd -

clean:
	rm -rf build
	rm -f libbsp.a bs2_default.bin bs2_default.s
	find . -iname "*.o" -exec rm -f {} \;
