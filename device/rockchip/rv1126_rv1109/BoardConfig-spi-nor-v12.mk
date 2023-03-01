#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm
# build idblock.bin and update SPL
export RK_IDBLOCK_UPDATE_SPL=true
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=rv1126-spi-nor-tiny
# Uboot defconfig fragment, config rk-sfc.config if sdcard upgrade
export RK_UBOOT_DEFCONFIG_FRAGMENT=rk-sfc.config
# Uboot image format type: fit(flattened image tree)
export RK_UBOOT_FORMAT_TYPE=fit
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rv1126_defconfig
# kernel defconfig fragment
export RK_KERNEL_DEFCONFIG_FRAGMENT=rv1126-spi-nor.config
# Kernel dts
export RK_KERNEL_DTS=rv1126-evb-ddr3-v12-spi-nor
# export RK_KERNEL_DTS=rv1126-38x38-v10-spi-nor
export RK_KERNEL_FIT_ITS=boot.its
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm/boot/zImage
# parameter for GPT table
export RK_PARAMETER=parameter-spi-nor-32M.txt
# Buildroot config
export RK_CFG_BUILDROOT=rockchip_rv1126_rv1109_tiny
# target chip
export RK_CHIP=rv1126_rv1109
# Set rootfs type, including squashfs jffs2
export RK_ROOTFS_TYPE=squashfs
# Define package-file for update.img
export RK_PACKAGE_FILE=rv1126-package-file-spi-nor-tiny
# Define WiFi BT chip
# Compatible with Realtek and AP6XXX WiFi : RK_WIFIBT_CHIP=ALL_AP
# Compatible with Realtek and CYWXXX WiFi : RK_WIFIBT_CHIP=ALL_CY
# Single WiFi configuration: AP6256 or CYW43455: RK_WIFIBT_CHIP=AP6256
export RK_WIFIBT_CHIP=AP6256
# Define BT ttySX
export RK_WIFIBT_TTY=ttyS0
