#!/bin/bash

set -e

build_normal_security_boot()
{
	echo "[$0] Build ramdisk with sha256 digest"
	DIGEST=$(dirname "$ROOTFS_IMG")/ramdisk.gz.digest

	openssl dgst -sha256 -binary -out "$DIGEST" "$ROOTFS_IMG"
	DIGEST_SIZE=$(stat -c "%s" "$ROOTFS_IMG")

	if [ "$RK_KERNEL_ARCH" == "arm" ]; then
		ITS="kernel/arch/arm/boot/dts/$RK_KERNEL_DTS.dts"
	else
		ITS="kernel/arch/arm64/boot/dts/rockchip/$RK_KERNEL_DTS.dts"
	fi

	cp $ITS $ITS.backup
cat << EOF >> $ITS
&ramdisk_c {
	size = <$DIGEST_SIZE>;
	hash {
		algo = "sha256";
		value = /incbin/("$DIGEST");
	};
};
EOF
	./build.sh kernel
	mv $ITS.backup $ITS
}

ROOTFS_IMG="$1"
TARGET_IMG="$2"
ITS="$3"
if [ ! -f "$ROOTFS_IMG" ]; then
	echo "Source ($ROOTFS_IMG) doesn't exist"
	exit 0
fi

KERNEL_IMG="$RK_KERNEL_IMG"
KERNEL_DTB=kernel/resource.img

if [ ! -f "$KERNEL_IMG" ]; then
	echo "kernel doesn't exist, build it!"
	./build.sh kernel
fi

if echo "$ROOTFS_IMG" | grep -qw "romfs$"; then
	# Do compress for tinyrootfs
	cat "$ROOTFS_IMG" | gzip -n -f -9 > "$ROOTFS_IMG.gz"
	cat "$KERNEL_IMG" | gzip -n -f -9 > "$KERNEL_IMG.gz"
	ROOTFS_IMG="$ROOTFS_IMG.gz"
	KERNEL_IMG="$KERNEL_IMG.gz"
fi

echo -n "Packing $ROOTFS_IMG to $TARGET_IMG..."
if [ -f "$ITS" ]; then
	if [ "$RK_RAMDISK_SECURITY_BOOTUP" = "true" ]; then
		if [ -z "$RK_SYSTEM_CHECK_METHOD" ]; then
			build_normal_security_boot $0
		fi
	fi

	device/rockchip/common/mk-fitimage.sh "$TARGET_IMG" "$ITS" \
		"$KERNEL_IMG" "$ROOTFS_IMG"
else
	kernel/scripts/mkbootimg --kernel "$KERNEL_IMG" --ramdisk "$ROOTFS_IMG" --second "$KERNEL_DTB" -o "$TARGET_IMG"
fi
echo "done."
