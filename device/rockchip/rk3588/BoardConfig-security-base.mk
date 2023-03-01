#!/bin/bash
# ramboot config
export RK_CFG_BUILDROOT=rockchip_rk3588_ramboot
export RK_RAMBOOT=true
export RK_ROOTFS_TYPE=cpio.gz
# Define enable RK SECUREBOOT
export RK_RAMDISK_SECURITY_BOOTUP=true
export RK_SECURITY_OTP_DEBUG=true
export RK_SYSTEM_CHECK_METHOD=DM-V
