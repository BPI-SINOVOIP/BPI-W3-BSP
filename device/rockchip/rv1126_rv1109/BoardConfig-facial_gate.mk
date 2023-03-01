#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=rv1126
# Uboot image format type: fit(flattened image tree)
export RK_UBOOT_FORMAT_TYPE=fit
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rv1126_defconfig
# Kernel defconfig fragment
export RK_KERNEL_DEFCONFIG_FRAGMENT=rv1126-facial-gate.config
# Kernel dts
export RK_KERNEL_DTS=rv1109-evb-ddr3-v13-facial-gate
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm/boot/zImage
# kernel image format type: fit(flattened image tree)
export RK_KERNEL_FIT_ITS=boot.its
# parameter for GPT table
export RK_PARAMETER=parameter-facial-gate.txt
# Buildroot config
export RK_CFG_BUILDROOT=rockchip_rv1126_rv1109_facial_gate
# Recovery config
export RK_CFG_RECOVERY=rockchip_rv1126_rv1109_recovery
# Recovery image format type: fit(flattened image tree)
export RK_RECOVERY_FIT_ITS=boot4recovery.its
# target chip
export RK_CHIP=rv1126_rv1109
# Set rootfs type, including ext2 ext4 squashfs
export RK_ROOTFS_TYPE=ext4
#misc image
export RK_MISC=wipe_all-misc.img
# Define package-file for update.img
export RK_PACKAGE_FILE=rv1126_rv1109-package-file
# <dev>:<mount point>:<fs type>:<mount flags>:<source dir>:<image size(M|K|auto)>:[options]
export RK_EXTRA_PARTITIONS="oem:/oem:ext2:defaults:oem_facial_gate:auto:resize@userdata:/userdata:ext2:defaults:userdata_normal:auto:resize"
