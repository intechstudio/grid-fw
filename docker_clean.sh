#!/bin/sh

if command -v docker 1> /dev/null; then
	CONTAINER_CMD="docker"
elif command -v podman 1> /dev/null; then
	CONTAINER_CMD="podman"
else
	echo "Neither docker nor podman found. Please install either to proceed."
	exit 1
fi

IMAGE=${IMAGE:-grid-fw-build}

# Check that the image exists
if [ -z "$(sudo docker images -q "$IMAGE")" ] ; then
	echo "Image does not seem to exist."
	exit 1
fi

# Remove containers that share the image as an ancestor
CONTAINER_IDS="$($CONTAINER_CMD ps -a -q --filter ancestor="$IMAGE")"
if [ -n "$CONTAINER_IDS" ] ; then
	echo "$CONTAINER_IDS"
	$CONTAINER_CMD rm -f $CONTAINER_IDS
fi

# Remove image
$CONTAINER_CMD rmi "$IMAGE"
