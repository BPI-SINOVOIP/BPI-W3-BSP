#!/bin/bash

# Target arch
export RK_KERNEL_ARCH=arm64
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=rknpu-lion
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rk3399pro_npu_pcie_defconfig
# Kernel dts
export RK_KERNEL_DTS=rk3399pro-npu-evb-v10-multi-cam
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm64/boot/Image
# parameter for GPT table
export RK_PARAMETER=parameter-npu.txt
# ramboot config
export RK_CFG_BUILDROOT=rockchip_rk3399pro-npu-multi-cam
export RK_RAMBOOT=true
export RK_ROOTFS_TYPE=cpio.gz
# target chip
export RK_CHIP=rk3399pro
