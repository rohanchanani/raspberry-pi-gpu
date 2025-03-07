LIBS += $(CS140E_2025_PATH)/lib/libgcc.a $(CS140E_2025_PATH)/libpi/libpi.a

PROGS := parallel-add.c

COMMON_SRC := hello.c mailbox.c

STAFF_OBJS += $(CS140E_2025_PATH)/libpi/staff-objs/staff-hw-spi.o
STAFF_OBJS += $(CS140E_2025_PATH)/libpi/staff-objs/kmalloc.o
# uncomment if you want it to automatically run.
RUN=1

BOOTLOADER = my-install
EXCLUDE ?= grep -v simple_boot
GREP_STR := 'HASH:\|ERROR:\|PANIC:\|SUCCESS:\|NRF:'
include $(CS140E_2025_PATH)/libpi/mk/Makefile.robust

checkoff:
	make -f ./Makefile PROGS="$(wildcard tests/[0123]-*.c)" check
	# make -f ./Makefile PROGS="$(ALL)" check
