#!/bin/sh

if command -v docker &> /dev/null; then
	CONTAINER_CMD="docker"
	ARGS="--privileged"
elif command -v podman &> /dev/null; then
	CONTAINER_CMD="podman"
	ARGS="--group-add keep-groups --security-opt label=disable"
else
	echo "Neither docker nor podman found. Please install either to proceed."
	exit 1
fi

$CONTAINER_CMD run $ARGS --network=host -it -v /dev:/dev -v $PWD:/project -w /project/ grid-fw-build
