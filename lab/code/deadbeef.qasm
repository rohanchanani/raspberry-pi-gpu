.include "../share/vc4inc/vc4.qinc"


ldi vw_setup, 0x101a00
ldi vpm, 0xfaded070
mov.never -, vw_wait
ldi vw_setup, 0x80844000
mov r0, unif
mov vw_addr, r0
mov.never -, vw_wait
nop;  thrend
nop
nop
