#makefile for resume1.code
always  := resume1.code
targets := resume1.elf

GCOV_PROFILE=no

#use "-Os" flags.
#Don't use "-O2" flags.
KBUILD_CFLAGS := -g -c -nostdlib -march=armv7-a -D__LINUX_ARM_ARCH__=7 -marm -fno-unwind-tables -fno-jump-tables -fno-asynchronous-unwind-tables -mlittle-endian -O2 -mno-unaligned-access

#Include the cur dir.
KBUILD_CPPFLAGS += -I.

LD_FLAGS = -static
LIBS =

INCLUDE   = -I.                              \
            -I$(KDIR)/include                \
            -I$(KDIR)/drivers/soc/allwinner/pm

resume1-y := resume1_head.o                  \
             resume1_entry.o                 \
             resume1_sram.o

resume1-y := $(addprefix $(obj)/,$(resume1-y))

$(obj)/resume1.code: $(obj)/resume1.bin
	$(Q)$(obj)/gen_check_code $(obj)/resume1.bin $(obj)/resume1.code

$(obj)/resume1.bin: $(obj)/resume1.elf FORCE
	$(Q)$(CROSS_COMPILE)objcopy -O binary $(obj)/resume1.elf $(obj)/resume1.bin

ifneq ($(strip $(CONFIG_ARCH_SUN8I)),)
$(obj)/resume1.elf: $(obj)/sun8i_resume1_scatter.scat $(resume1-y)
	$(Q)$(CROSS_COMPILE)ld -T $(obj)/sun8i_resume1_scatter.scat  -EL  $(resume1-y) -o $(obj)/resume1.elf  -Map $(obj)/resume1.map
	$(Q)$(CROSS_COMPILE)objdump -D $(obj)/resume1.elf > $(obj)/resume1.lst
endif

#$(call if_changed,ld)

clean:
	-rm $(obj) *.code *.bin *.map *.lst *.elf
