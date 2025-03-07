::smi_start

# Read uniforms into registers
mov   ra0, unif #BLOCKS    
mov   ra1, unif #A
mov   ra2, unif #B
mov   ra3, unif #C

    #-----------------------------------------------------
    # 1) DMA read of A
    #-----------------------------------------------------
:next
mov vr_setup, vdr_setup_1(64)
mov vr_setup, vdr_setup_0(0, 16, 4, vdr_h32(2, 0, 0))
mov vr_addr, ra1
mov -, vr_wait

    #-----------------------------------------------------
    # 2) DMA read of B
    #-----------------------------------------------------
mov vr_setup, vdr_setup_1(64)
mov vr_setup, vdr_setup_0(0, 16, 4, vdr_h32(2, 1, 0))
mov vr_addr, ra2
mov -, vr_wait

    #-----------------------------------------------------
    # 3) In each lane: load A[lane], load B[lane], add
    #    (no 'inner loop' needed - each lane sees its own elem_num)
    #-----------------------------------------------------
    # Read this lane's A from VPM row=elem_num
mov vr_setup, vpm_setup(8, 1, h32(0))
mov vw_setup, vpm_setup(4, 1, h32(8))
nop
nop
nop

:loop
    mov r1, vpm
    mov r2, vpm   

    # Sum = A[lane] + B[lane]
    add r1, r1, r2

    mov vpm, r4

    sub.setf ra0, ra0, 1
    brr.anynz -, :loop

mov vw_setup, vdw_setup_0(4, 16, vdr_h32(1,8, 0))
mov vw_addr, ra3
mov -, vw_wait


# End of kernel
:end
thrend
mov interrupt, 1
nop

