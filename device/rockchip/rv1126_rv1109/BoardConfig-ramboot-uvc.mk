#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=rv1126-ramboot
# Uboot image format type: fit(flattened image tree)
export RK_UBOOT_FORMAT_TYPE=fit
# Uboot SPL ini config
export RK_SPL_INI_CONFIG=RV1126MINIALL_RAMBOOT.ini
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rv1126_defconfig
# Kernel dts
#export RK_KERNEL, which must modify cmdline
export RK_KERNEL_DTS=rv1126-evb-ddr3-v13
#export RK_KERNEL_DTS=rv1126-ai-cam-ddr3-v1
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm/boot/zImage
# kernel image format type: fit(flattened image tree)
export RK_KERNEL_FIT_ITS=boot.its
# parameter for GPT table
export RK_PARAMETER=parameter-buildroot-fit.txt
# Recovery image format type: fit(flattened image tree)
export RK_RECOVERY_FIT_ITS=boot4recovery.its
# ramboot config
export RK_CFG_BUILDROOT=rockchip_rv1126_rv1109_ramboot_uvcc
export RK_RAMBOOT=true
export RK_ROOTFS_TYPE=cpio.gz
# target chip
export RK_CHIP=rv1126_rv1109
# Set rootfs type, including ext2 ext4 squashfs
export RK_ROOTFS_TYPE=cpio.gz
# Define package-file for update.img
#export RK_PACKAGE_FILE=rv1126_rv1109-package-file-uvc
# <dev>:<mount point>:<fs type>:<mount flags>:<source dir>:<image size(M|K|auto)>:[options]
export RK_EXTRA_PARTITIONS="oem:/oem:ext2:defaults:oem_uvcc:auto:resize@userdata:/userdata:ext2:defaults:userdata_normal:auto:resize"
