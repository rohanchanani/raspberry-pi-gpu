# An implementation of the BLAS GEMM function for single-precision floats
# on the Raspberry Pi QPU.
# By Pete Warden, pete@jetpac.com

define(`MUTEX_ACQUIRE',     `or ra39, ra51, rb39;           nop')
define(`MUTEX_RELEASE',     `or ra51, ra39, ra39;           nop')

# Hardwired IO registers
define(`rVpmWriteFifo', `rb48')
define(`rVpmReadFifo', `ra48')
define(`raReadUniform', `ra32')
define(`rbReadUniform', `rb32')
define(`raZero', `ra39')
define(`rbZero', `rb39')
define(`raTmu0S', `ra56')
define(`rbTmu0S', `rb56')
define(`raTmu1S', `ra60')
define(`rbTmu1S', `rb60')
define(`raTmuNoSwap', `ra36')
define(`rbTmuNoSwap', `rb36')

# Macro argument constants
define(`MODEW_32_BIT', 0)
define(`MODEW_16_BIT_OFFSET_0', 2)
define(`MODEW_16_BIT_OFFSET_1', 3)
define(`MODEW_8_BIT_OFFSET_0', 4)
define(`MODEW_8_BIT_OFFSET_1', 5)
define(`MODEW_8_BIT_OFFSET_2', 6)
define(`MODEW_8_BIT_OFFSET_3', 7)
define(`SIZE_8_BIT', 0)
define(`SIZE_16_BIT', 1)
define(`SIZE_32_BIT', 2)
define(`IS_HORIZ', 1)
define(`NOT_HORIZ', 0)
define(`IS_VERT', 1)
define(`NOT_VERT', 0)
define(`IS_LANED', 1)
define(`NOT_LANED', 0)

# VPM_BLOCK_WRITE_SETUP
# ~~~~~~~~~~~~~~~~~~~~~
# Sets up things so writes go into the small VPM data cache.
# Once the data's been written (by outputting repeatedly to the VPM_WRITE_FIFO
# register rb48), you then call VPM_DMA_WRITE_SETUP to configure the main
# memory destination and writing pattern.
# Arguments:
#  STRIDE: 0-64 - How much to increment the ADDR after each write.
#  HORIZ: 0 or 1 - Whether the layout is horizontal (1) or vertical (0).
#  LANED: 0 or 1 - Whether the layout is laned (1) or packed (0).
#  SIZE: 0, 1, 2 - The data unit size, 8-bit (0), 16-bit(1), or 32-bit (2).
#  ADDR: 0-255 - Packed address, meaning depends on exact unit size and mode.
# See http://www.broadcom.com/docs/support/videocore/VideoCoreIV-AG100-R.pdf page 57
define(`VPM_BLOCK_WRITE_SETUP_ID_SHIFT', 30)
define(`VPM_BLOCK_WRITE_SETUP_STRIDE_SHIFT', 12)
define(`VPM_BLOCK_WRITE_SETUP_HORIZ_SHIFT', 11)
define(`VPM_BLOCK_WRITE_SETUP_LANED_SHIFT', 10)
define(`VPM_BLOCK_WRITE_SETUP_SIZE_SHIFT', 8)
define(`VPM_BLOCK_WRITE_SETUP_ADDR_SHIFT', 0)
define(`VPM_BLOCK_WRITE_SETUP_VALUE', `eval(
(0<<VPM_BLOCK_WRITE_SETUP_ID_SHIFT)|
($1<<VPM_BLOCK_WRITE_SETUP_STRIDE_SHIFT)|
($2<<VPM_BLOCK_WRITE_SETUP_HORIZ_SHIFT)|
($3<<VPM_BLOCK_WRITE_SETUP_LANED_SHIFT)|
($4<<VPM_BLOCK_WRITE_SETUP_SIZE_SHIFT)|
($5<<VPM_BLOCK_WRITE_SETUP_ADDR_SHIFT))')
define(`VPM_BLOCK_WRITE_SETUP', `ldi rb49, VPM_BLOCK_WRITE_SETUP_VALUE($1, $2, $3, $4, $5)')

# VPM_BLOCK_READ_SETUP
# ~~~~~~~~~~~~~~~~~~~~
# Controls how values are read from the VPM data cache into the QPU.
# Arguments:
#  NUM: 0-16 - How many elements to read at a time.
#  STRIDE: 0-64 - The amount to increment the address by after each read.
#  HORIZ: 0 or 1 - Whether the layour is horizontal (1) or vertical (0).
#  LANED: 0 or 1 - Whether the layout is laned (1) or packed (0).
#  SIZE: 0, 1, 2 - The data unit size, 8-bit (0), 16-bit(1), or 32-bit (2).
#  ADDR: 0-255 - Packed address, meaning depends on exact unit size and mode.
# See http://www.broadcom.com/docs/support/videocore/VideoCoreIV-AG100-R.pdf page 58
define(`VPM_BLOCK_READ_SETUP_ID_SHIFT', 30)
define(`VPM_BLOCK_READ_SETUP_NUM_SHIFT', 20)
define(`VPM_BLOCK_READ_SETUP_STRIDE_SHIFT', 12)
define(`VPM_BLOCK_READ_SETUP_HORIZ_SHIFT', 11)
define(`VPM_BLOCK_READ_SETUP_LANED_SHIFT', 10)
define(`VPM_BLOCK_READ_SETUP_SIZE_SHIFT', 8)
define(`VPM_BLOCK_READ_SETUP_ADDR_SHIFT', 0)
define(`VPM_BLOCK_READ_SETUP_VALUE', `eval(
(0<<VPM_BLOCK_READ_SETUP_ID_SHIFT)|
($1<<VPM_BLOCK_READ_SETUP_NUM_SHIFT)|
($2<<VPM_BLOCK_READ_SETUP_STRIDE_SHIFT)|
($3<<VPM_BLOCK_READ_SETUP_HORIZ_SHIFT)|
($4<<VPM_BLOCK_READ_SETUP_LANED_SHIFT)|
($5<<VPM_BLOCK_READ_SETUP_SIZE_SHIFT)|
($6<<VPM_BLOCK_READ_SETUP_ADDR_SHIFT))')
define(`VPM_BLOCK_READ_SETUP', `ldi ra49, VPM_BLOCK_READ_SETUP_VALUE($1, $2, $3, $4, $5, $6)')

# VPM_DMA_STORE_SETUP
# ~~~~~~~~~~~~~~~~~~~
# Configures the DMA controller to transfer data from the VPM cache to main memory.
# Once the setup's been done, you then need to call VPM_DMA_STORE_START to kick
# off the transfer.
# Arguments:
#  UNITS: 0-128 - Number of rows of 2D block in memory.
#  DEPTH: 0-128 - How long each row is (in bytes?).
#  HORIZ: 0 or 1 - Whether the layout is horizontal (1) or vertical (0).
#  ADDRY: The Y coordinate of the address in the VPM space to start from.
#  ADDRX: The X coordinate of the address in the VPM space to start from.
#  MODEW: 0-7 : 0 is 32-bit, 2-3 is 16-bit with offset, 4-7 is 8-bit with offset.
# See http://www.broadcom.com/docs/support/videocore/VideoCoreIV-AG100-R.pdf page 58
define(`VPM_DMA_STORE_SETUP_ID_SHIFT', 30)
define(`VPM_DMA_STORE_SETUP_UNITS_SHIFT', 23)
define(`VPM_DMA_STORE_SETUP_DEPTH_SHIFT', 16)
define(`VPM_DMA_STORE_SETUP_HORIZ_SHIFT', 14)
define(`VPM_DMA_STORE_SETUP_ADDRY_SHIFT', 7)
define(`VPM_DMA_STORE_SETUP_ADDRX_SHIFT', 3)
define(`VPM_DMA_STORE_SETUP_MODEW_SHIFT', 0)
define(`VPM_DMA_STORE_SETUP_VALUE', `eval(
(2<<VPM_DMA_STORE_SETUP_ID_SHIFT)|
($1<<VPM_DMA_STORE_SETUP_UNITS_SHIFT)|
($2<<VPM_DMA_STORE_SETUP_DEPTH_SHIFT)|
($3<<VPM_DMA_STORE_SETUP_HORIZ_SHIFT)|
($4<<VPM_DMA_STORE_SETUP_ADDRY_SHIFT)|
($5<<VPM_DMA_STORE_SETUP_ADDRX_SHIFT)|
($6<<VPM_DMA_STORE_SETUP_MODEW_SHIFT))')
define(`VPM_DMA_STORE_SETUP', `ldi rb49, VPM_DMA_STORE_SETUP_VALUE($1, $2, $3, $4, $5, $6)')

# VPM_DMA_STORE_START
# ~~~~~~~~~~~~~~~~~~~
# Kicks off the transfer of data from the local VPM data cache to main memory.
# It will use the settings from VPM_DMA_STORE_SETUP to control the copy process.
# Arguments:
#  address: A register name that holds the address in main memory to write to.
define(`VPM_DMA_STORE_START', `or rb50, $1, 0;          nop')

# VPM_DMA_STORE_WAIT_FOR_COMPLETION
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Pause until the previous DMA store operation has finished.
define(`VPM_DMA_STORE_WAIT_FOR_COMPLETION', `or rb39, rb50, rb50;       nop')

# VPM_DMA_LOAD_SETUP
# ~~~~~~~~~~~~~~~~~~
# Initializes the settings for transfering data from main memory into the VPM cache.
# Arguments:
#  MODEW: 0-7 : 0 is 32-bit, 2-3 is 16-bit with offset, 4-7 is 8-bit with offset.
#  MPITCH: 0-15: The amount to increment the memory pointer between rows, calculated as 8*2^MPITCH bytes.
#  ROWLEN: 0-15: The number of elements in each row in main memory.
#  NROWS: 0-15: How many rows to read from memory.
#  VPITCH: 0-15: How much to increment the VPM address by after each row is loaded.
#  VERT: 0 or 1 - Whether the layout is vertical (1) or horizontal (0). Be careful, this is inverted compared to normal.
#  ADDRY: 0-64 - The Y coordinate of the address in the VPM space to start loading into.
#  ADDRX: 0-16 - The X coordinate of the address in the VPM space to start loading into.
define(`VPM_DMA_LOAD_SETUP_ID_SHIFT', 31)
define(`VPM_DMA_LOAD_SETUP_MODEW_SHIFT', 28)
define(`VPM_DMA_LOAD_SETUP_MPITCH_SHIFT', 24)
define(`VPM_DMA_LOAD_SETUP_ROWLEN_SHIFT', 20)
define(`VPM_DMA_LOAD_SETUP_NROWS_SHIFT', 16)
define(`VPM_DMA_LOAD_SETUP_VPITCH_SHIFT', 12)
define(`VPM_DMA_LOAD_SETUP_VERT_SHIFT', 11)
define(`VPM_DMA_LOAD_SETUP_ADDRY_SHIFT', 4)
define(`VPM_DMA_LOAD_SETUP_ADDRX_SHIFT', 0)
define(`VPM_DMA_LOAD_SETUP_VALUE', `eval(
(1<<VPM_DMA_LOAD_SETUP_ID_SHIFT)|
($1<<VPM_DMA_LOAD_SETUP_MODEW_SHIFT)|
($2<<VPM_DMA_LOAD_SETUP_MPITCH_SHIFT)|
($3<<VPM_DMA_LOAD_SETUP_ROWLEN_SHIFT)|
($4<<VPM_DMA_LOAD_SETUP_NROWS_SHIFT)|
($5<<VPM_DMA_LOAD_SETUP_VPITCH_SHIFT)|
($6<<VPM_DMA_LOAD_SETUP_VERT_SHIFT)|
($7<<VPM_DMA_LOAD_SETUP_ADDRY_SHIFT)|
($8<<VPM_DMA_LOAD_SETUP_ADDRX_SHIFT))')
define(`VPM_DMA_LOAD_SETUP', `ldi ra49, VPM_DMA_LOAD_SETUP_VALUE($1, $2, $3, $4, $5, $6, $7, $8)')

# VPM_DMA_LOAD_START
# ~~~~~~~~~~~~~~~~~~~
# Kicks off the transfer of data from main memory to the local VPM data cache.
# It will use the settings from VPM_DMA_LOAD_SETUP to control the copy process.
# Arguments:
#  address: A register name that holds the address in main memory to read from.
define(`VPM_DMA_LOAD_START', `or ra50, $1, 0;          nop')

# VPM_DMA_LOAD_WAIT_FOR_COMPLETION
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Pause until the previous DMA load operation has finished.
define(`VPM_DMA_LOAD_WAIT_FOR_COMPLETION', `or rb39, ra50, ra50;       nop')

# END_PROGRAM
# ~~~~~~~~~~~
# Triggers a host interrupt to transfer control back to the main CPU.
define(`END_PROGRAM_HARD', `
or rb38, r0, 1;       nop
nop.tend ra39, ra39, ra39;       nop rb39, rb39, rb39
nop ra39, ra39, ra39;       nop rb39, rb39, rb39
nop ra39, ra39, ra39;       nop rb39, rb39, rb39')

define(`END_PROGRAM_SOFT', `
nop.tend ra39, ra39, ra39;      nop rb39, rb39, rb39
NOP
NOP
')

# NOP
# ~~~
# Do nothing on both pipes for a cycle
define(`NOP', `nop ra39, ra39, ra39;       nop rb39, rb39, rb39')

# Constants we'll use later on
define(`VECTORS_PER_PASS', 1)
define(`ELEMENTS_PER_PASS', `eval(VECTORS_PER_PASS * 16)')
define(`ELEMENTS_PER_PASS_MINUS_ONE', `eval(ELEMENTS_PER_PASS - 1)')
define(`A_BYTES_PER_PASS', `eval(ELEMENTS_PER_PASS * 4)')
define(`B_BYTES_PER_PASS', `eval(ELEMENTS_PER_PASS * 4)')
define(`ELEMENTS_PER_FINISH_PASS', 16)
define(`ELEMENTS_PER_FINISH_PASS_MINUS_ONE', `eval(ELEMENTS_PER_FINISH_PASS - 1)')
define(`A_BYTES_PER_FINISH_PASS', `eval(ELEMENTS_PER_FINISH_PASS * 4)')
define(`B_BYTES_PER_FINISH_PASS', `eval(ELEMENTS_PER_FINISH_PASS * 4)')
define(`VPM_ROWS_PER_PASS', 1)
define(`NUM_QPUS', 12)
define(`ALL_DONE_SEMA', 0)
define(`SHOULD_DISABLE_TMU_SWAPPING', 1)

# Registers used to hold uniforms
define(`rM', ra0)
define(`rN', ra1)
define(`rK', ra2)
define(`rAlpha', ra3)
define(`rAAddress', ra4)
define(`rAMin', ra5)
define(`rARange', ra6)
define(`rLDA', ra7)
define(`rBAddress', ra8)
define(`rLDB', ra9)
define(`rBeta', ra10)
define(`rCAddress', ra11)
define(`rLDC', ra12)
define(`rDebugAddress', ra24)
define(`rWhichQPU', ra26)

# Registers used to hold working values
define(`rI', ra13)
define(`rJ', ra14)
define(`rL', ra15)
define(`rCurrentA', ra16)
define(`rCurrentB', ra17)
define(`rCurrentC', ra20)
define(`rElementsToRead', ra23)
define(`rDebugOutput', ra25)
define(`rDMALoadAddrY', ra27)
define(`rVPMReadAddr', ra28)
define(`rVPMWriteAddr', ra29)
define(`rDMAStoreAddrY', ra30)
define(`rAVPMReadAddr', ra31)
# Warning - overloading raMisc register, beware of clashes if the scope expands
define(`raMisc', `rCurrentC')
define(`rA0to15', rb0)
define(`rA16to31', rb1)
define(`rA32to47', rb2)
define(`rA48to63', rb3)
define(`rA64to79', rb4)
define(`rA80to95', rb5)
define(`rA96to111', rb6)
define(`rA112to127', rb7)
define(`rLinearRamp', rb8)
define(`rElementCountMask', rb9)
define(`rRowsToLoad', rb10)
define(`rElementsRemaining', rb11)
define(`rMaskShift', rb12)
define(`rElementsPerVector', rb13)

# The special accumulator registers, heavily reused so generally not named
define(`rAccum0', r0)
define(`rAccum1', r1)
define(`rAccum2', r2)
define(`rTotal', r3)

# Load uniform arguments
or rM, raReadUniform, 0; nop
or rN, raReadUniform, 0; nop
or rK, raReadUniform, 0; nop
or rAlpha, raReadUniform, 0; nop
or rAAddress, raReadUniform, 0; nop
or rAMin, raReadUniform, 0; nop
or rARange, raReadUniform, 0; nop
or rLDA, raReadUniform, 0; nop
or rBAddress, raReadUniform, 0; nop
or rLDB, raReadUniform, 0; nop
or rBeta, raReadUniform, 0; nop
or rCAddress, raReadUniform, 0; nop
or rLDC, raReadUniform, 0; nop
or rDebugAddress, raReadUniform, 0; nop
or rWhichQPU, raReadUniform, 0; nop

# Store 0.5f in the debug output register, so we can tell if it's been untouched
ldi rDebugOutput, 0x3f000000

or raTmu0S, rDebugAddress, 0; nop
or.ldtmu0 ra39, ra39, ra39; nop
add rDebugOutput, r4, 1; nop

# Turn off the automatic switching of TMU0/1 behind the scenes since we're
# going to explicitly control calling each TMU unit.
ldi raTmuNoSwap, SHOULD_DISABLE_TMU_SWAPPING

# Set up our working area of memory in the shared VPM space, based on the
# QPU number we've been given. The VPM can be viewed as a 2d table, 16 floats
# wide and 64 rows high. In our case, we use 12 QPUs, and give each one a single
# row in the VPM table.
nop rb39, r0, r0; mul24 rTotal, rWhichQPU, VPM_ROWS_PER_PASS
ldi rAccum0, VPM_DMA_LOAD_SETUP_ADDRY_SHIFT
shl rDMALoadAddrY, rTotal, rAccum0; nop
ldi rAccum0, VPM_BLOCK_READ_SETUP_ADDR_SHIFT
shl rVPMReadAddr, rTotal, rAccum0; nop
ldi rAccum0, VPM_BLOCK_WRITE_SETUP_ADDR_SHIFT
shl rVPMWriteAddr, rTotal, rAccum0; nop
ldi rAccum0, VPM_DMA_STORE_SETUP_ADDRY_SHIFT
shl rDMAStoreAddrY, rTotal, rAccum0; nop

# Create a special vector that we'll use to mask out unwanted components
# in the 16-wide vectors that we load. The result should be:
# [0, 1, 2, ..., 14, 15]
ldi rAccum0, [0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3]
ldi rAccum1, [0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3]
add rAccum0, rAccum0, rAccum1; nop
ldi rAccum1, [0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3]
add rAccum0, rAccum0, rAccum1; nop
ldi rAccum1, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3]
add rAccum0, rAccum0, rAccum1; nop
ldi rAccum1, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3]
add rLinearRamp, rAccum0, rAccum1; nop

or rI, rWhichQPU, 0; nop
loop_i:
or rAccum0, rM, 0; nop
sub ra39, rI, rAccum0; nop
brr.ne ra39, loop_i_break
#NOP # Commented out NOPs after branches are optimized out because the following
#NOP # instructions don't have side-effects
#NOP

ldi rJ, 0
loop_j:
or rAccum0, rN, 0; nop
sub ra39, rJ, rAccum0; nop
brr.ne ra39, loop_j_break
#NOP
#NOP
#NOP

# Set up the reading addresses for the A and B matrices
shl rAccum0, rLDA, 2; nop
nop rb39, r0, r0; mul24 rAccum0, rI, rAccum0
add rCurrentA, rAAddress, rAccum0; nop

shl rAccum0, rLDB, 2; nop
nop rb39, r0, r0; mul24 rAccum0, rJ, rAccum0
add rCurrentB, rBAddress, rAccum0; nop

ldi rTotal, 0

# Constants we use for address calculations inside the loop
or rAccum1, rLinearRamp, rLinearRamp; nop
shl rAccum1, rAccum1, 2; nop
ldi rAccum2, 64

# Kick off eight vector fetches (each of 16 floats) through the TMUs.
# We explicitly control which TMU is used, so four are fired off on TMU 0, and
# four on TMU1. This is the maximum number we can keep in-flight at a time.
add raTmu0S, rCurrentA, rAccum1; nop
add raTmu1S, rCurrentB, rAccum1; nop

add rCurrentA, rCurrentA, rAccum2; nop
add rCurrentB, rCurrentB, rAccum2; nop

add raTmu0S, rCurrentA, rAccum1; nop
add raTmu1S, rCurrentB, rAccum1; nop

add rCurrentA, rCurrentA, rAccum2; nop
add rCurrentB, rCurrentB, rAccum2; nop

add raTmu0S, rCurrentA, rAccum1; nop
add raTmu1S, rCurrentB, rAccum1; nop

add rCurrentA, rCurrentA, rAccum2; nop
add rCurrentB, rCurrentB, rAccum2; nop

add raTmu0S, rCurrentA, rAccum1; nop
add raTmu1S, rCurrentB, rAccum1; nop

add rCurrentA, rCurrentA, rAccum2; nop
add rCurrentB, rCurrentB, rAccum2; nop

ldi rL, 0

# Do an initial check if we have too few elements for a full pass.
ldi rAccum0, ELEMENTS_PER_PASS_MINUS_ONE
sub rAccum0, rK, rAccum0; nop
sub ra39, rL, rAccum0; nop
brr.ne ra39, main_loop_l_break
NOP
NOP
NOP

# This is the section that handles multiplying the A and B vectors together,
# and adding them to the total.
main_loop_l:

# We read a pending A result from the queue, and then immediately fire off the
# next memory fetch, to get the maximum concurrency.
or.ldtmu0 ra39, ra39, ra39; nop
add raTmu0S, rCurrentA, rAccum1; nop
or rA0to15, r4, 0; nop

# Now we pull the values from B, and fire off the next fetch.
add.ldtmu1 rCurrentA, rCurrentA, rAccum2; nop
add raTmu1S, rCurrentB, rAccum1; nop
add rCurrentB, rCurrentB, rAccum2; fmul rAccum0, rA0to15, r4
fadd rTotal, rTotal, rAccum0; nop

sub rL, rL, -16; nop

sub rAccum0, rK, 15; nop
sub ra39, rL, rAccum0; nop
brr.ns ra39, main_loop_l
NOP
NOP
NOP

main_loop_l_break:

# Set up a count of how many elements we have left
or rAccum0, rK, 0; nop
sub rAccum0, rAccum0, rL; nop
or rElementsRemaining, rAccum0, rAccum0; nop

ldi rAccum0, 64
or rAccum1, rLinearRamp, rLinearRamp; nop
shl rAccum1, rAccum1, 2; nop

# We pull the next two fetches from A and B, and later we'll mask the unneeded
# elements of the vectors out, to handle row lengths that aren't multiples of 16.
add.ldtmu0 rCurrentA, rCurrentA, rAccum2; nop
or rA0to15, r4, 0; nop
or.ldtmu1 ra39, ra39, ra39; nop
add rCurrentB, rCurrentB, rAccum2; fmul rA0to15, rA0to15, r4

# We actually have been firing off memory fetches ahead of where we are, so we
# need to consume and discard six vectors. This means we're reading off the end
# of the matrix on the last row, which in theory could cause a memory fault,
# but because we're dealing with contiguous physical memory addresses at the
# hardware level, in practice it's not a problem.
or.ldtmu0 ra39, ra39, ra39; nop
or.ldtmu1 ra39, ra39, ra39; nop

or.ldtmu0 ra39, ra39, ra39; nop
or.ldtmu1 ra39, ra39, ra39; nop

or.ldtmu0 ra39, ra39, ra39; nop
or.ldtmu1 ra39, ra39, ra39; nop

ldi rMaskShift, 31
ldi rElementsPerVector, 16

or rAccum1, rElementsRemaining, rElementsRemaining; nop
sub rElementsRemaining, rAccum1, rElementsPerVector; nop
sub rAccum0, rLinearRamp, rAccum1; nop
asr rAccum1, rAccum0, rMaskShift; nop
and rAccum2, rA0to15, rAccum1; nop
fadd rTotal, rTotal, rAccum2; nop

finish_loop_l_break:

# Take the 16-component-wide result vector and sum it into a single value
or r0, rTotal, 0; nop
or r3, rTotal, 0; nop
nop rb39, r0, <<1; v8max r0, r0, r0
fadd rTotal, rTotal, r0; nop
or r0, rTotal, 0; nop
or r3, rTotal, 0; nop
nop rb39, r0, <<2; v8max r0, r0, r0
fadd rTotal, rTotal, r0; nop
or r0, rTotal, 0; nop
or r3, rTotal, 0; nop
nop rb39, r0, <<4; v8max r0, r0, r0
fadd rTotal, rTotal, r0; nop
or r0, rTotal, 0; nop
or r3, rTotal, 0; nop
nop rb39, r0, <<8; v8max r0, r0, r0
fadd rTotal, rTotal, r0; nop

nop rb39, r0, r0; fmul rTotal, rTotal, rAlpha;

# Store the result into VPM memory
define(`STRIDE', 1)
define(`ADDR', 0)
ldi rAccum0, VPM_BLOCK_WRITE_SETUP_VALUE(STRIDE, IS_HORIZ, NOT_LANED, SIZE_32_BIT, ADDR)
or rb49, rAccum0, rVPMWriteAddr; nop

or rVpmWriteFifo, rTotal, 0; nop

shl rAccum0, rLDC, 2; nop
nop rb39, r0, r0; mul24 rAccum0, rJ, rAccum0
add rCurrentC, rCAddress, rAccum0; nop
shl rAccum0, rI, 2; nop
add rCurrentC, rCurrentC, rAccum0; nop

# DMA the result into main memory from the VPM
define(`UNITS', 1)
define(`DEPTH', 1)
define(`ADDRY', 0)
define(`ADDRX', 0)
ldi rAccum0, VPM_DMA_STORE_SETUP_VALUE(UNITS, DEPTH, IS_HORIZ, ADDRY, ADDRX, MODEW_32_BIT)
or rb49, rAccum0, rDMAStoreAddrY; nop

MUTEX_ACQUIRE()
VPM_DMA_STORE_START(rCurrentC)
MUTEX_RELEASE()

add rJ, rJ, 1; nop

brr ra39, loop_j
VPM_DMA_STORE_WAIT_FOR_COMPLETION()
#NOP
NOP
NOP

loop_j_break:

add rI, rI, NUM_QPUS; nop
brr ra39, loop_i
NOP
NOP
NOP

loop_i_break:

# This block will write out the 16 values in the rDebugOutput register to main
# memory if it's uncommented. This is my main debugging tool, you can examine
# intermediate values by storing them into this register.
define(`STRIDE', 1)
define(`ADDR', 0)
ldi rAccum0, VPM_BLOCK_WRITE_SETUP_VALUE(STRIDE, IS_HORIZ, NOT_LANED, SIZE_32_BIT, ADDR)
or rb49, rAccum0, rVPMWriteAddr; nop

or rVpmWriteFifo, rDebugOutput, 0; nop

define(`UNITS', 1)
define(`DEPTH', 16)
define(`ADDRY', 0)
define(`ADDRX', 0)
ldi rAccum0, VPM_DMA_STORE_SETUP_VALUE(UNITS, DEPTH, IS_HORIZ, ADDRY, ADDRX, MODEW_32_BIT)
or rb49, rAccum0, rDMAStoreAddrY; nop

MUTEX_ACQUIRE()
VPM_DMA_STORE_START(rDebugAddress)
MUTEX_RELEASE()
VPM_DMA_STORE_WAIT_FOR_COMPLETION()

# We need to coordinate the execution of all the QPUs, so that the program end
# isn't signaled before they're all done. To handle this, first each program
# raises a semaphore to say that it's done, and then the master QPU (given the
# number 0 in its rWhichQPU uniform) pulls the semaphore down eight times to
# ensure all the others are done, before signaling back to the main CPU.
sema up, ALL_DONE_SEMA

or rb39, rWhichQPU, 0; nop
brr.zc rb39, non_master_finish
NOP
NOP
NOP

# The number of 'down's must match the number of QPUs being run
sema down, ALL_DONE_SEMA
sema down, ALL_DONE_SEMA
sema down, ALL_DONE_SEMA
sema down, ALL_DONE_SEMA
sema down, ALL_DONE_SEMA
sema down, ALL_DONE_SEMA
sema down, ALL_DONE_SEMA
sema down, ALL_DONE_SEMA
sema down, ALL_DONE_SEMA
sema down, ALL_DONE_SEMA
sema down, ALL_DONE_SEMA
sema down, ALL_DONE_SEMA

END_PROGRAM_HARD()

non_master_finish:

END_PROGRAM_SOFT()