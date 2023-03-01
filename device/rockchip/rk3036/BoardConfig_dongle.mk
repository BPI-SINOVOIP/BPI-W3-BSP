#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=rk3036
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rk3036_dongle_linux_defconfig
# Kernel dts
export RK_KERNEL_DTS=rk3036-dongle
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm/boot/zImage
# parameter for GPT table
export RK_PARAMETER=parameter-buildroot-128M.txt
# Buildroot config
export RK_CFG_BUILDROOT=rockchip_rk3036
# Recovery config
export RK_CFG_RECOVERY=rockchip_rk3036_recovery
# Pcba config
export RK_CFG_PCBA=rockchip_rk3036_pcba
# target chip
export RK_CHIP=rk3036
# Set rootfs type, including ext2 ext4 squashfs
export RK_ROOTFS_TYPE=squashfs
#misc image
export RK_MISC=wipe_all-misc.img
# Define WiFi BT chip
# Compatible with Realtek and AP6XXX WiFi : RK_WIFIBT_CHIP=ALL_AP
# Compatible with Realtek and CYWXXX WiFi : RK_WIFIBT_CHIP=ALL_CY
# Single WiFi configuration: AP6256 or CYW43455: RK_WIFIBT_CHIP=AP6256
export RK_WIFIBT_CHIP=ALL_AP
# <dev>:<mount point>:<fs type>:<mount flags>:<source dir>:<image size(M|K|auto)>:[options]
export RK_EXTRA_PARTITIONS="oem:/oem:ext2:defaults:oem_empty:auto:resize@userdata:/userdata:ext2:defaults:userdata_empty:auto:resize"
