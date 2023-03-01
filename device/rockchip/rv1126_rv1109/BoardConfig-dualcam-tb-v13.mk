#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=rv1126-emmc-tb
# Uboot image format type: fit(flattened image tree)
export RK_UBOOT_FORMAT_TYPE=fit
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rv1126_defconfig
# kernel defconfig fragment
export RK_KERNEL_DEFCONFIG_FRAGMENT="rv1126-tb.config rv1126-facial-gate.config"
# Kernel dts
export RK_KERNEL_DTS=rv1126-evb-ddr3-v13-dualcam-tb-emmc
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm/boot/Image
# parameter for GPT table
export RK_PARAMETER=parameter-tb.txt
# ramboot config
export RK_CFG_BUILDROOT=rockchip_rv1126_evb_dualcam_tb
export RK_RAMBOOT=true
export RK_ROOTFS_TYPE=romfs
# ramboot idt config
export RK_RECOVERY_FIT_ITS=boot-tb.its
# target chip
export RK_CHIP=rv1126_rv1109
# Define package-file for update.img
export RK_PACKAGE_FILE=rv1126-package-file-emmc-dualcam-tb
# Define WiFi BT chip
# Compatible with Realtek and AP6XXX WiFi : RK_WIFIBT_CHIP=ALL_AP
# Compatible with Realtek and CYWXXX WiFi : RK_WIFIBT_CHIP=ALL_CY
# Single WiFi configuration: AP6256 or CYW43455: RK_WIFIBT_CHIP=AP6256
export RK_WIFIBT_CHIP=ALL_AP
# Define BT ttySX
export RK_WIFIBT_TTY=ttyS0
# <dev>:<mount point>:<fs type>:<mount flags>:<source dir>:<image size(M|K|auto)>:[options]
export RK_EXTRA_PARTITIONS="userdata:/userdata:ext2:defaults:userdata_empty:auto:resize"
