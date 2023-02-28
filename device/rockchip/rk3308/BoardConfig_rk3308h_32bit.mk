#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=rk3308-aarch32
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rk3308_linux_aarch32_defconfig
# Kernel dts
export RK_KERNEL_DTS=rk3308-voice-module-board-v11-aarch32
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm/boot/zImage
# parameter for GPT table
export RK_PARAMETER=parameter-32bit.txt
# Buildroot config
export RK_CFG_BUILDROOT=rockchip_rk3308_h_32_release
# Recovery config
export RK_CFG_RECOVERY=rockchip_rk3308_recovery
# Pcba config
export RK_CFG_PCBA=rockchip_rk3308_pcba
# target chip
export RK_CHIP=rk3308
# Set rootfs type, including ext2 ext4 squashfs
export RK_ROOTFS_TYPE=squashfs
MIC_NUM=6
#misc image
export RK_MISC=wipe_all-misc.img
# Define WiFi BT chip
# # Compatible with Realtek and AP6XXX WiFi : RK_WIFIBT_CHIP=ALL_AP
# # Compatible with Realtek and CYWXXX WiFi : RK_WIFIBT_CHIP=ALL_CY
# # Single WiFi configuration: AP6256 or CYW43455: RK_WIFIBT_CHIP=AP6256
export RK_WIFIBT_CHIP=RTL8189FS
# # Define BT ttySX
export RK_WIFIBT_TTY=ttyS0

##########################################################
### enable build update_sdcard.img
### Detail to see docs/Linux/Recovery/Rockchip_Developer_Guide_Linux_Upgrade_CN.pdf
# export RK_UPDATE_SDCARD_ENABLE_FOR_AB=true
##########################################################
# <dev>:<mount point>:<fs type>:<mount flags>:<source dir>:<image size(M|K|auto)>:[options]
export RK_EXTRA_PARTITIONS="oem:/oem:ext2:defaults:oem_empty:auto:resize@userdata:/userdata:ext2:defaults:userdata_empty:auto:resize"
