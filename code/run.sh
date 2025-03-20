#!/bin/bash

vc4asm -c mulshader.c -h mulshader.h vector-multiply.qasm
vc4asm -c addshader.c -h addshader.h parallel-add.qasm

make run