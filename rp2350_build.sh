#!/bin/sh

mkdir -p rp2350/build

rm -f rp2350/build/main/main.*

cmake -S rp2350 -B rp2350/build

if [ $? -ne 0 ] ; then
	exit 1
fi

make -C rp2350/build

if [ $? -ne 0 ] ; then
	exit 1
fi

echo "Built rp2350/build/main/main.uf2"
