#!/bin/bash
CONTAINER_TOOL=$(command -v docker || command -v podman)
$CONTAINER_TOOL rm -f $($CONTAINER_TOOL ps -a -q --filter ancestor=idf-pico-merged) 2>/dev/null; $CONTAINER_TOOL rmi idf-pico-merged
