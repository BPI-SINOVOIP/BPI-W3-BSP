#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=rv1126
# Uboot image format type: fit(flattened image tree)
export RK_UBOOT_FORMAT_TYPE=fit
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rv1126_defconfig
# EMMC EVB BOARD Kernel dts
export RK_KERNEL_DTS=rv1126-evb-ddr3-v13-uvc
# Logic/npu/vepu merge emmc board kernel dts
#export RK_KERNEL_DTS=rv1126-ai-cam-ddr3-v1
# NPU 800m+ logic separate from npu/vepu emmc board kernel dts
#export RK_KERNEL_DTS=rv1126-ai-cam-plus
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm/boot/zImage
# parameter for GPT table
export RK_PARAMETER=parameter-buildroot-fit.txt
# Buildroot config
export RK_CFG_BUILDROOT=rockchip_rv1126_rv1109_uvcc
# target chip
export RK_CHIP=rv1126_rv1109
# Set rootfs type, including ext2 ext4 squashfs
export RK_ROOTFS_TYPE=ext4
# Define package-file for update.img
export RK_PACKAGE_FILE=rv1126_rv1109-package-file-uvc
# <dev>:<mount point>:<fs type>:<mount flags>:<source dir>:<image size(M|K|auto)>:[options]
export RK_EXTRA_PARTITIONS="oem:/oem:ext2:defaults:oem_uvcc:auto:resize@userdata:/userdata:ext2:defaults:userdata_normal:auto:resize"
