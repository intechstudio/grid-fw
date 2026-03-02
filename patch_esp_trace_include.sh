#!/bin/sh

set -e

if [ -z "$IDF_PATH" ]; then
    echo "Error: IDF_PATH is not set" >&2
    exit 1
fi

FILEPATH=$IDF_PATH/components/freertos/config/include/freertos/FreeRTOSConfig.h
LINENO="10"
DIRECTIVE="#include \"../../../../../../../../../project/grid_esp/components/grid_esp32_trace/trace_hooks.h\""
sed -i "${LINENO}i ${DIRECTIVE}" "$FILEPATH"
