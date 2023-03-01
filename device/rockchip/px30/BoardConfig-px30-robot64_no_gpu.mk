#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm64
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=px30
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=px30_linux_robot_defconfig
# Kernel dts
export RK_KERNEL_DTS=px30-evb-ddr3-v10-robot-no-gpu-linux
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm64/boot/Image.lz4
# parameter for GPT table
export RK_PARAMETER=parameter-robot.txt
# Buildroot config
export RK_CFG_BUILDROOT=rockchip_px30_robot64_no_gpu
# Recovery config
export RK_CFG_RECOVERY=rockchip_px30_robot_recovery
# Pcba config
export RK_CFG_PCBA=rockchip_px30_pcba
# target chip
export RK_CHIP=px30
# Set rootfs type, including ext2 ext4 squashfs
export RK_ROOTFS_TYPE=squashfs
#misc image
export RK_MISC=wipe_all-misc.img
# Define WiFi BT chip
# Compatible with Realtek and AP6XXX WiFi : RK_WIFIBT_CHIP=ALL_AP
# Compatible with Realtek and CYWXXX WiFi : RK_WIFIBT_CHIP=ALL_CY
# Single WiFi configuration: AP6256 or CYW43455: RK_WIFIBT_CHIP=AP6256
export RK_WIFIBT_CHIP=AP6212A1
# Define BT ttySX
export RK_WIFIBT_TTY=ttyS1
# <dev>:<mount point>:<fs type>:<mount flags>:<source dir>:<image size(M|K|auto)>:[options]
export RK_EXTRA_PARTITIONS="oem:/oem:ext2:defaults:oem_normal:auto:resize@userdata:/userdata:ext2:defaults:userdata_normal:auto:resize"
