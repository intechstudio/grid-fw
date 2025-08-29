#!/bin/sh

FILEPATH=/opt/esp/idf/components/efuse/src/esp_efuse_startup.c
LINENO=$(grep -n "ROM log scheme" "$FILEPATH")
LINENO=${LINENO%:*}
sed -i "$((LINENO))d" "$FILEPATH"
