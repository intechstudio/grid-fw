#!/bin/sh

FILEPATH=/opt/esp/idf/components/freertos/config/include/freertos/FreeRTOSConfig.h
LINENO="10"
DIRECTIVE="#include \"../../../../../../../../../project/grid_esp/components/grid_esp32_trace/trace_hooks.h\""
sed -i "${LINENO}i ${DIRECTIVE}" "$FILEPATH"
