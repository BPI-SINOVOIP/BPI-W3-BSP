#!/bin/bash

set -e

TARGET_IMG="$1"
ITS="$2"
KERNEL_IMG="$3"
RAMDISK_IMG="$4"
RESOURCE_IMG=kernel/resource.img

if [ ! -f $ITS ]; then
	echo "No its $ITS ..."
	exit -1
fi

if [ "$RK_KERNEL_ARCH" == "arm" ]; then
	KERNEL_DTB="kernel/arch/arm/boot/dts/$RK_KERNEL_DTS.dtb"
else
	KERNEL_DTB="kernel/arch/arm64/boot/dts/rockchip/$RK_KERNEL_DTS.dtb"
fi

TMP_ITS=$(mktemp)
cp "$ITS" "$TMP_ITS"

if [ "$RK_RAMDISK_SECURITY_BOOTUP" = "true" ]; then
	echo "Security boot enabled, removing uboot-ignore ..."
	sed -i "/uboot-ignore/d" "$TMP_ITS"
fi

sed -i -e "s~@KERNEL_DTB@~$(realpath -q "$KERNEL_DTB")~" \
	-e "s~@KERNEL_IMG@~$(realpath -q "$KERNEL_IMG")~" \
	-e "s~@RAMDISK_IMG@~$(realpath -q "$RAMDISK_IMG")~" \
	-e "s~@RESOURCE_IMG@~$(realpath -q "$RESOURCE_IMG")~" "$TMP_ITS"

rkbin/tools/mkimage -f "$TMP_ITS"  -E -p 0x800 "$TARGET_IMG"
