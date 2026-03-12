#!/bin/sh

if command -v docker &> /dev/null; then
	CONTAINER_CMD="docker"
elif command -v podman &> /dev/null; then
	CONTAINER_CMD="podman"
else
	echo "Neither docker nor podman found. Please install either to proceed."
	exit 1
fi

$CONTAINER_CMD build --squash -t grid-fw-build .
