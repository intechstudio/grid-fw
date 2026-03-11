#!/bin/sh

mkdir -p build
gcc -g -o build/proc proc.c vmp_def.c ../../../vmp/vmp.c -I ./ -I ../../../vmp
