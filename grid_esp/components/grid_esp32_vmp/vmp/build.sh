#!/bin/sh

mkdir -p build
gcc -flto -O2 -g -o build/proc proc.c vmp_def.c ../../../../vmp/vmp.c -I ./ -I ../../../../vmp
