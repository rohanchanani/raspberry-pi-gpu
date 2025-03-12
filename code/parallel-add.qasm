::smi_start

# Read uniforms into registers
mov   ra0, unif #BLOCKS    
mov   ra1, unif #A
mov   ra2, unif #B
mov   ra3, unif #C
mov rb6, 256

    #-----------------------------------------------------
    # 1) DMA read of A
    #-----------------------------------------------------
:next

:loop

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

 
    add ra1, ra1, rb6
    add ra2, ra2, rb6


    #-----------------------------------------------------
    # 3) In each lane: load A[lane], load B[lane], add
    #    (no 'inner loop' needed - each lane sees its own elem_num)
    #-----------------------------------------------------
    # Read this lane's A from VPM row=elem_num
    mov vr_setup, vpm_setup(8, 1, h32(0))
    mov vw_setup, vpm_setup(4, 1, h32(8))
    mov ra10, 4

:inner_loop

    mov ra11, vpm
    mov -, vw_wait

    mov rb12, vpm   
    mov -, vw_wait


    # Sum = A[lane] + B[lane]
    add ra11, ra11, rb12

    mov vpm, ra11
    mov -, vw_wait

    sub.setf ra10, ra10, 1
    brr.anynz -, :inner_loop

    mov vw_setup, vdw_setup_0(4, 16, dma_h32(8,0))
    mov vw_addr, ra3
    mov -, vw_wait

    add ra3, ra3, rb6


    sub.setf ra0, ra0, 1
    brr.anynz -, :loop


# End of kernel
:end
thrend
mov interrupt, 1
nop

