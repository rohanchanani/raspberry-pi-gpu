.include "../share/vc4inc/vc4.qinc"
mov   ra0, unif #BLOCKS    
mov   ra1, unif #A
mov   ra2, unif #B
mov   ra3, unif #C
mov r3, 64

:next

:loop

    mov vr_setup, vdr_setup_0(0, 16, 1, vdr_h32(2, 0, 0))
    mov vr_addr, ra1
    mov -, vr_wait

    mov vr_setup, vdr_setup_0(0, 16, 1, vdr_h32(2, 1, 0))
    mov vr_addr, ra2
    mov -, vr_wait

 
    add ra1, ra1, r3
    add ra2, ra2, r3


    mov vr_setup, vpm_setup(2, 1, h32(0))
    mov vw_setup, vpm_setup(1, 1, h32(0))

    mov r1, vpm
    mov -, vw_wait

    mov r2, vpm   
    mov -, vw_wait
    # Sum = A[lane] + B[lane]
    add r1, r1, r2
    mov vpm, r1
    mov -, vw_wait

    mov vw_setup, vdw_setup_0(1, 16, dma_h32(0,0))
    mov vw_addr, ra3
    mov -, vw_wait

    add ra3, ra3, r3
    

    sub.setf ra0, ra0, 1
    brr.anynz -, :loop

    nop
    nop
    nop


# End of kernel
:end
thrend
mov interrupt, 1
nop
nop