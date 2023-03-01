#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=evb-px3se
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rockchip_linux_defconfig
# Kernel dts
export RK_KERNEL_DTS=px3se-evb
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm/boot/zImage
# parameter for GPT table
export RK_PARAMETER=parameter-nand.txt
# Buildroot config
export RK_CFG_BUILDROOT=rockchip_px3se
# Recovery config
export RK_CFG_RECOVERY=rockchip_px3se_recovery
# Pcba config
export RK_CFG_PCBA=rockchip_px3se_pcba
# target chip
export RK_CHIP=px3se
# Set rootfs type, including ext2 ext4 squashfs
export RK_ROOTFS_TYPE=squashfs
#misc image
export RK_MISC=wipe_all-misc.img
# <dev>:<mount point>:<fs type>:<mount flags>:<source dir>:<image size(M|K|auto)>:[options]
export RK_EXTRA_PARTITIONS="oem:/oem:ext2:defaults:oem_empty:auto:resize@userdata:/userdata:ext2:defaults:userdata_empty:auto:resize"
