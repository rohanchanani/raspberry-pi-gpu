/* linker.ld - Bare-metal linker script for Raspberry Pi A+ (BCM2835) */

ENTRY(_start)

SECTIONS
{
    /* The Raspberry Pi A+ boots the kernel at 0x8000 */
    . = 0x8000;

    /* Text (code) section */
    .text : {
        *(.text)
        *(.text.*)
    }

    /* Read-only data section */
    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    /* Data (initialized variables) */
    .data : {
        *(.data)
        *(.data.*)
    }

    /* BSS (uninitialized variables, zeroed at startup) */
    .bss : {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }
}

