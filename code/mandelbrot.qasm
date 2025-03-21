#.include "../share/vc4inc/vc4.qinc"

# Read uniforms into registers
mov   ra0, unif #RESOLUTION    
mov   ra1, unif #1/RESOLUTION
mov   ra2, unif #MAX_ITER
mov   ra3, unif #NUM_QPU
mov   ra4, unif #QPU_NUM
mov   ra5, unif #ADDRESS

    #-----------------------------------------------------
    # 1) DMA read of A
    #-----------------------------------------------------

mov ra10, ra4 #i = QPU_NUM

:row_loop

shl ra11, ra0, 1 #m = 2*RESOLUTION
mov ra6, ra11 # 2*RESOLUTION

shl r1, ra0, 3 #bytes_per_row = 8*RESOLUTION
mul24 ra12, r1, ra4 #Array_idx = i * bytes_per_row

itof r1, ra10 #(float) i
itof r2, -1 #(float) -1
fmul r1, r1, ra1 #i * 1/RESOLUTION
fadd rb9, r1, r2 #y = -1 + i*1/resolution

#rb9 = y, rb8 = x

:column_loop

    mov r1, ra6 #2*RESOLUTION
    sub r1, r1, ra11 #j = 2*RESOLUTION - m
    add r1, r1, elem_num #j+= elem_num
    itof r1, r1 #(float) j
    itof r2, -1 #(float) -1
    fmul r1, r1, ra1 #x = j*1/RESOLUTION
    fadd rb8, r1, r2 #x = -1 + j*1/RESOLUTION

    mov r2, vpm_setup(1, 1, h32(0))
    add vw_setup, ra4, r2

    mov vpm, 55 #VPM WRITE
    mov -, vw_wait

    shl r1, ra4, 7
    mov r2, vdw_setup_0(1, 16, dma_h32(0,0))
    add vw_setup, r1, r2
    
	
    mov 

    mov vw_addr, ra5
    mov -, vw_wait
    
    sub.setf ra11, ra11, 16 #m -= 16
    brr.anynz -, :column_loop

    nop
    nop
    nop

    mov r1, ra3
    add ra10, ra10, r1 # i += NUM_QPU
    mov r1, ra6
    sub.setf r1, ra10, r1  #if i < 2*RESOLUTION

    brr.anyc -, :row_loop
    nop
    nop
    nop

# End of kernel
:end
thrend
mov interrupt, 1
nop

