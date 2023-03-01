#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm64
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=rknpu-lion
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rk1808_x4_linux_defconfig
# main board kernel dts
export RK_KERNEL_DTS=rk1808-evb-x4
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm64/boot/Image
# parameter for GPT table
export RK_PARAMETER=parameter-buildroot.txt
# Buildroot config
export RK_CFG_BUILDROOT=rockchip_rk1808-multi
# target chip
export RK_CHIP=rk1808
#enable multi-npu-boot image auto-build
export RK_MULTINPU_BOOT=y
