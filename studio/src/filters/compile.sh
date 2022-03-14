#!/bin/bash
# how to compile?
# type ./compile.sh sample_filter to compile sample_filter.c
gcc $1.c -c -g
gcc -shared $1.o -o $1.cfi

