#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=rv1126-bat-spi-nor-tb
# Uboot image format type: fit(flattened image tree)
export RK_UBOOT_FORMAT_TYPE=fit
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rv1126_defconfig
# kernel defconfig fragment
export RK_KERNEL_DEFCONFIG_FRAGMENT="rv1126-tb.config rv1126-trailcamera.config"
# Kernel dts
export RK_KERNEL_DTS=rv1126-trailcamera
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm/boot/Image
# parameter for GPT table
export RK_PARAMETER=parameter-trailcamera.txt
# ramboot config
export RK_CFG_BUILDROOT=rockchip_rv1126_trailcamera
export RK_RAMBOOT=true
export RK_ROOTFS_TYPE=romfs
# ramboot idt config
export RK_RECOVERY_FIT_ITS=boot-tb.its
# target chip
export RK_CHIP=rv1126_rv1109
# Define package-file for update.img
export RK_PACKAGE_FILE=rv1126-package-file-spi-nor-tb
