LIBS += $(CS140E_2025_PATH)/lib/libgcc.a $(CS140E_2025_PATH)/libpi/libpi.a

PROGS := $(wildcard tests/4-*.c)

# PROGS := tests/3-test-fire.c

COMMON_SRC := mulshader.c mailbox.c addshader.c parallel-add.c vector-multiply.c mandelbrotshader.c 


COMMON_SRC += fat32/code/pi-sd.c fat32/code/mbr-helpers.c fat32/code/fat32-helpers.c fat32/code/fat32-lfn-helpers.c fat32/code/external-code/unicode-utf8.c fat32/code/external-code/emmc.c fat32/code/fat32.c fat32/code/mbr.c #  external-code/mbox.c
STAFF_OBJS += $(CS140E_2025_PATH)/libpi/staff-objs/staff-hw-spi.o
STAFF_OBJS += $(CS140E_2025_PATH)/libpi/staff-objs/kmalloc.o
#STAFF_OBJS += fat32/code/staff-fat32.o fat32/code/staff-mbr.o
# uncomment if you want it to automatically run.
RUN=1

BOOTLOADER = my-install
EXCLUDE ?= grep -v simple_boot
GREP_STR := 'HASH:\|ERROR:\|PANIC:\|SUCCESS:\|NRF:'
include $(CS140E_2025_PATH)/libpi/mk/Makefile.robust

checkoff:
	make -f ./Makefile PROGS="$(wildcard tests/[0123]-*.c)" check
	# make -f ./Makefile PROGS="$(ALL)" check
