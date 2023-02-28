#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=rk3126
# SPL INI
export RK_SPL_INI_CONFIG=RK3126MINIALL_WO_FTL.ini
# Uboot defconfig fragment,config rk-nand.config
export RK_UBOOT_DEFCONFIG_FRAGMENT=rk312x-slc-nand.config
# Trust ini config
export RK_TRUST_INI_CONFIG=RK3126TOS_LADDR.ini
# Uboot size
export RK_UBOOT_SIZE_CONFIG=1024\ 2
# Trust size
export RK_TRUST_SIZE_CONFIG=1024\ 2
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rockchip_linux_defconfig
# Kernel defconfig fragment
export RK_KERNEL_DEFCONFIG_FRAGMENT=rk3126_linux_slc_nand.config
# Kernel dts
export RK_KERNEL_DTS=rk3126c-evb-ddr3-v10-linux-slc
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm/boot/zImage
# parameter for GPT table
export RK_PARAMETER=parameter-buildroot-rk3126-slc.txt
# Buildroot config
export RK_CFG_BUILDROOT=rockchip_rk3126c
# Recovery config
export RK_CFG_RECOVERY=rockchip_rk312x_recovery
# Pcba config
export RK_CFG_PCBA=rockchip_rk3126c_pcba
# target chip
export RK_CHIP=rk312x
# Set rootfs type, including ext2 ext4 squashfs
export RK_ROOTFS_TYPE=ubi
#misc image
export RK_MISC=wipe_all-misc.img

# <dev>:<mount point>:<fs type>:<mount flags>:<source dir>:<image size(M|K|auto)>:[options]
export RK_EXTRA_PARTITIONS="oem:/oem:ubi:defaults:oem_normal:20480K:fixed@userdata:/userdata:ubi:defaults:userdata_normal:184320K:fixed"
