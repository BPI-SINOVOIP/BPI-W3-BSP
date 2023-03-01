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
# Kernel defconfig fragment
export RK_KERNEL_DEFCONFIG_FRAGMENT="rv1126-uvc-spi-nand.config"
# Kernel dts
export RK_KERNEL_DTS=rv1126-ai-cam-ddr3-v1-spi-nand
#export RK_KERNEL_DTS=rv1126-ai-cam-audio-ddr3-v1-spi-nand
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm/boot/zImage
# parameter for GPT table
#export RK_PARAMETER=parameter-fit-128M.txt
export RK_PARAMETER=parameter-fit-128M-ab.txt
# Buildroot config
export RK_CFG_BUILDROOT=rockchip_rv1126_rv1109_uvcc_spi_nand
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
# Define package-file for update_ab.img
export RK_PACKAGE_FILE_AB=rv1126-package-file-spi-nand-uvc-ab
# Define package-file for update_ota.img
export RK_PACKAGE_FILE_OTA=rv1126-package-file-spi-nand-uvc-ota

##########################################################
### enable build update_sdcard.img
### Detail to see docs/Linux/Recovery/Rockchip_Developer_Guide_Linux_Upgrade_CN.pdf
# export RK_UPDATE_SDCARD_ENABLE_FOR_AB=true
### Recovery image format type: fit(flattened image tree)
# export RK_RECOVERY_FIT_ITS=boot4recovery.its
##########################################################
# <dev>:<mount point>:<fs type>:<mount flags>:<source dir>:<image size(M|K|auto)>:[options]
export RK_EXTRA_PARTITIONS="userdata:/userdata:ubi:defaults:userdata_empty:6656K:fixed"
