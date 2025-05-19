#!/bin/bash


# VC4ASM
echo "ASSEMBLING QASM"
vc4asm -c addshader.c -h addshader.h parallel-add.qasm

# run tests
echo "RUNNING MAKE"
make