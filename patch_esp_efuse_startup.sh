#!/bin/sh

set -e

if [ -z "$IDF_PATH" ]; then
    echo "Error: IDF_PATH is not set" >&2
    exit 1
fi

FILEPATH=$IDF_PATH/components/efuse/src/esp_efuse_startup.c
LINENO=$(grep -n "ROM log scheme" "$FILEPATH")
LINENO=${LINENO%:*}
sed -i "$((LINENO))d" "$FILEPATH"
