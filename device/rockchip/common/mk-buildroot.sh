#!/bin/bash

set -e

CONFIG=$1
OUTPUT_DIR="$2"
BUILDROOT_DIR="${BUILDROOT_DIR:-$(pwd)/buildroot}"

source "$BUILDROOT_DIR"/build/envsetup.sh $CONFIG

# Use buildroot images dir as image output dir
IMAGE_DIR="$TARGET_OUTPUT_DIR"/images
rm -rf "$OUTPUT_DIR"
mkdir -p "$IMAGE_DIR"
ln -rsf "$IMAGE_DIR" "$OUTPUT_DIR"
cd "$OUTPUT_DIR"

LOG_FILE="$(pwd)/br.log"

if "$BUILDROOT_DIR"/utils/brmake -C "$BUILDROOT_DIR"; then
	echo "Log saved on $LOG_FILE"
	echo "Generated images:"
	ls rootfs.*
else
	echo "Failed to build $CONFIG:"
	tail -n 100 "$LOG_FILE"
	echo "Please check details in $LOG_FILE"
	exit 1
fi
