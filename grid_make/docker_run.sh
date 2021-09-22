#!/bin/bash

podman run -w /usr/grid_make/gcc -v ./:/usr/grid_make/ --rm -it $* armswdev/arm-tools:bare-metal-compilers

