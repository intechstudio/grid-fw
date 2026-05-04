#!/bin/sh

mkdir wasm/build

emcmake cmake -S wasm -B wasm/build && cmake --build ./wasm/build
