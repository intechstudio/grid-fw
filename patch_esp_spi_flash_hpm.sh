#!/bin/sh

set -e

if [ -z "$IDF_PATH" ]; then
    echo "Error: IDF_PATH is not set" >&2
    exit 1
fi

FILEPATH=$IDF_PATH/components/spi_flash/spi_flash_hpm_enable.c
sed -i "/#warning High Performance Mode/d" "$FILEPATH"
