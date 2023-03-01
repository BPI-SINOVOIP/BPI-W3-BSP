#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=rv1126
# Uboot defconfig fragment, config rk-sfc.config if sdcard upgrade
export RK_UBOOT_DEFCONFIG_FRAGMENT=rk-sfc.config
# Uboot update loader (spl)
# Uboot image format type: fit(flattened image tree)
export RK_UBOOT_FORMAT_TYPE=fit
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rv1126_defconfig
# Kernel dts
export RK_KERNEL_DTS=rv1126-evb-ddr3-v12-spi-nand
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm/boot/zImage
# kernel image format type: fit(flattened image tree)
export RK_KERNEL_FIT_ITS=boot.its
# parameter for GPT table
export RK_PARAMETER=parameter-fit-nand-256M.txt
# Buildroot config
export RK_CFG_BUILDROOT=rockchip_rv1126_rv1109_spi_nand
# Recovery config
export RK_CFG_RECOVERY=rockchip_rv1126_rv1109_spi_nand_recovery
# Recovery image format type: fit(flattened image tree)
export RK_RECOVERY_FIT_ITS=boot4recovery.its
# target chip
export RK_CHIP=rv1126_rv1109
# Set rootfs type, including ext2 ext4 squashfs ubi
export RK_ROOTFS_TYPE=ubi
#
# Set ubifs page size, 2048(2KB) or 4096(4KB)
# Option.
# export RK_UBI_PAGE_SIZE=2048
#
# Set ubifs block size, 0x20000(128KB) or 0x40000(256KB)
# Option.
# export RK_UBI_BLOCK_SIZE=0x20000
#
#misc image
export RK_MISC=blank-misc.img
# Define package-file for update.img
export RK_PACKAGE_FILE=rv1126-package-file-spi-nand
# Define WiFi BT chip
# Compatible with Realtek and AP6XXX WiFi : RK_WIFIBT_CHIP=ALL_AP
# Compatible with Realtek and CYWXXX WiFi : RK_WIFIBT_CHIP=ALL_CY
# Single WiFi configuration: AP6256 or CYW43455: RK_WIFIBT_CHIP=AP6256
export RK_WIFIBT_CHIP=ALL_AP
# Define BT ttySX
export RK_WIFIBT_TTY=ttyS0
