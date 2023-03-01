#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=rv1126
# Uboot defconfig fragment, config rk-sfc.config if sdcard upgrade, rv1126-ab.config for AB system bootup
export RK_UBOOT_DEFCONFIG_FRAGMENT="rv1126-ab.config rk-sfc.config"
# Uboot update loader (spl)
# Uboot image format type: fit(flattened image tree)
export RK_UBOOT_FORMAT_TYPE=fit
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rv1126_defconfig
# Kernel dts
export RK_KERNEL_DTS=rv1109-38-v10-spi-nand
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image format type: fit(flattened image tree)
export RK_KERNEL_FIT_ITS=boot.its
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm/boot/zImage
# parameter for GPT table
#export RK_PARAMETER=parameter-fit-128M.txt
export RK_PARAMETER=parameter-fit-nand-256M-ab.txt
# Buildroot config
export RK_CFG_BUILDROOT=rockchip_rv1126_rv1109_spi_nand
# target chip
export RK_CHIP=rv1126_rv1109
# Set rootfs type, including ext2 ext4 squashfs ubi
export RK_ROOTFS_TYPE=ubi
#
# Set ubifs page size, 2048(2KB) or 4096(4KB)
# Option.
export RK_UBI_PAGE_SIZE=2048
#
# Set ubifs block size, 0x20000(128KB) or 0x40000(256KB)
# Option.
export RK_UBI_BLOCK_SIZE=0x20000
#misc image
export RK_MISC=blank-misc.img
# Define package-file for update_ab.img
export RK_PACKAGE_FILE_AB=rv1126-package-file-spi-nand-256MB-ab
# Define package-file for ota update_ota.img
export RK_PACKAGE_FILE_OTA=rv1126-package-file-spi-nand-256MB-ota

##########################################################
### enable build update_sdcard.img
### Detail to see docs/Linux/Recovery/Rockchip_Developer_Guide_Linux_Upgrade_CN.pdf
# export RK_UPDATE_SDCARD_ENABLE_FOR_AB=true
### Recovery config
export RK_UPDATE_SDCARD_CFG_RECOVERY=rockchip_rv1126_rv1109_spi_nand_recovery
### Recovery image format type: fit(flattened image tree)
export RK_RECOVERY_FIT_ITS=boot4recovery.its
##########################################################
# Define WiFi BT chip
# Compatible with Realtek and AP6XXX WiFi : RK_WIFIBT_CHIP=ALL_AP
# Compatible with Realtek and CYWXXX WiFi : RK_WIFIBT_CHIP=ALL_CY
# Single WiFi configuration: AP6256 or CYW43455: RK_WIFIBT_CHIP=AP6256
export RK_WIFIBT_CHIP=ALL_AP
# Define BT ttySX
export RK_WIFIBT_TTY=ttyS0
