#!/bin/bash

# VC4ASM
vc4asm -c mulshader.c -h mulshader.h vector-multiply.qasm
vc4asm -c addshader.c -h addshader.h parallel-add.qasm
vc4asm -c simpleshader.c -h simpleshader.h deadbeef.qasm
vc4asm -c mandelbrotshader.c -h mandelbrotshader.h mandelbrot.qasm

# run tests
make run