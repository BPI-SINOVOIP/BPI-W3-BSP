#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=rv1126-ab
# Uboot image format type: fit(flattened image tree)
export RK_UBOOT_FORMAT_TYPE=fit
# Uboot update loader (spl)
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rv1126_defconfig
# Kernel dts
export RK_KERNEL_DTS=rv1126-evb-ddr3-v13
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm/boot/zImage
# kernel image format type: fit(flattened image tree)
export RK_KERNEL_FIT_ITS=boot.its
# parameter for GPT table
export RK_PARAMETER=parameter-ab-fit.txt
# Buildroot config
export RK_CFG_BUILDROOT=rockchip_rv1126_rv1109
# target chip
export RK_CHIP=rv1126_rv1109
# Set rootfs type, including ext2 ext4 squashfs
export RK_ROOTFS_TYPE=ext4
#misc image
export RK_MISC=blank-misc.img
# Define package-file for AB system update_ab.img
export RK_PACKAGE_FILE_AB=rv1126_rv1109-package-file-2-ab
# Define package-file for ota update_ota.img
export RK_PACKAGE_FILE_OTA=rv1126_rv1109-package-file-2-ota

##########################################################
### enable build update_sdcard.img
### Detail to see docs/Linux/Recovery/Rockchip_Developer_Guide_Linux_Upgrade_CN.pdf
# export RK_UPDATE_SDCARD_ENABLE_FOR_AB=true
### Recovery config
export RK_UPDATE_SDCARD_CFG_RECOVERY=rockchip_rv1126_rv1109_recovery
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
# <dev>:<mount point>:<fs type>:<mount flags>:<source dir>:<image size(M|K|auto)>:[options]
export RK_EXTRA_PARTITIONS="oem:/oem:ext2:defaults:oem_ipc:auto:resize@userdata:/userdata:ext2:defaults:userdata_normal:auto:resize"
