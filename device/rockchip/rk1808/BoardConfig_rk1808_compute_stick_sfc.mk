#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm64
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=rk1808
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rk1808_linux_defconfig
# Kernel dts
export RK_KERNEL_DTS=rk1808-compute-v10
# boot image type
export RK_BOOT_IMG=boot.img
# parameter for GPT table
export RK_PARAMETER=parameter-compute-stick.txt
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm64/boot/Image
# ramboot config
export RK_CFG_BUILDROOT=rockchip_rk1808_compute_stick
export RK_RAMBOOT=true
export RK_ROOTFS_TYPE=cpio.gz
# Pcba config
export RK_CFG_PCBA=rockchip_rk1808_pcba
# target chip
export RK_CHIP=rk1808
