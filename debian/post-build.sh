#!/bin/bash -e

TARGET_DIR=$1
shift

FSTAB="${TARGET_DIR}/etc/fstab"
OS_RELEASE="${TARGET_DIR}/etc/os-release"

RK_LEGACY_PARTITIONS=" \
    ${RK_OEM_FS_TYPE:+oem:/oem:${RK_OEM_FS_TYPE}}
    ${RK_USERDATA_FS_TYPE:+userdata:/userdata:${RK_USERDATA_FS_TYPE}}
"

# <dev>:<mount point>:<fs type>:<mount flags>:<source dir>:<image size(M|K|auto)>:[options]
# for example:
# RK_EXTRA_PARTITIONS="oem:/oem:ext2:defaults:oem_normal:256M:fixed
# userdata:/userdata:vfat:errors=remount-ro:userdata_empty:auto"
RK_EXTRA_PARTITIONS=${RK_EXTRA_PARTITIONS:-${RK_LEGACY_PARTITIONS}}

function fixup_root()
{
    echo "Fixing up rootfs type: $1"

    FS_TYPE=$1
    sed -i "s#\([[:space:]]/[[:space:]]\+\)\w\+#\1${FS_TYPE}#" "$FSTAB"
}

function fixup_part()
{
    echo "Fixing up partition: ${@//: }"

    SRC="$1"
    MOUNT="$2"
    FS_TYPE="$3"
    MOUNT_OPTS="$4"
    PASS="$5"

    # Remove old entries with same mountpoint
    sed -i "/[[:space:]]${MOUNT//\//\\\/}[[:space:]]/d" "$FSTAB"

    if [ "$SRC" != tmpfs ]; then
        # Remove old entries with same source
        sed -i "/^${SRC//\//\\\/}[[:space:]]/d" "$FSTAB"
    fi

    # Append new entry
    echo -e "${SRC}\t${MOUNT}\t${FS_TYPE}\t${MOUNT_OPTS}\t0 $PASS" >> "$FSTAB"

    mkdir -p "${TARGET_DIR}/${MOUNT}"
}

function fixup_basic_part()
{
    echo "Fixing up basic partition: $@"

    FS_TYPE="$1"
    MOUNT="$2"
    MOUNT_OPTS="${3:-defaults}"

    fixup_part "$FS_TYPE" "$MOUNT" "$FS_TYPE" "$MOUNT_OPTS" 0
}

function partition_arg() {
    PART="$1"
    I="$2"
    DEFAULT="$3"

    ARG=$(echo $PART | cut -d':' -f"$I")
    echo ${ARG:-$DEFAULT}
}

function fixup_device_part()
{
    echo "Fixing up device partition: ${@//: }"

    DEV="$(partition_arg "$*" 1)"

    # Dev is either <name> or /dev/.../<name>
    [ "$DEV" ] || return 0
    echo $DEV | grep -qE "^/" || DEV="LABEL=$DEV"

    MOUNT="$(partition_arg "$*" 2 "/${DEV##*[/=]}")"
    FS_TYPE="$(partition_arg "$*" 3 ext2)"
    MOUNT_OPTS="$(partition_arg "$*" 4 defaults)"

    fixup_part "$DEV" "$MOUNT" "$FS_TYPE" "$MOUNT_OPTS" 2
}

function fixup_fstab()
{
    echo "Fixing up /etc/fstab..."

    case "${RK_ROOTFS_TYPE}" in
        ext[234])
            fixup_root ${RK_ROOTFS_TYPE}
            ;;
        *)
            fixup_root auto
            ;;
    esac

    fixup_basic_part proc /proc
    fixup_basic_part devtmpfs /dev
    fixup_basic_part devpts /dev/pts mode=0620,ptmxmode=0666,gid=5
    fixup_basic_part tmpfs /dev/shm nosuid,nodev,noexec
    fixup_basic_part sysfs /sys
    fixup_basic_part debugfs /sys/kernel/debug
    fixup_basic_part pstore /sys/fs/pstore

    if echo $TARGET_DIR | grep -qE "_recovery/target/*$"; then
        fixup_device_part "/dev/sda1:/mnt/udisk:auto:defaults::"
        fixup_device_part "/dev/mmcblk1p1:/mnt/sdcard:auto:defaults::"
    fi

    for part in ${RK_EXTRA_PARTITIONS//@/ }; do
        fixup_device_part $part
    done
}

function add_build_info()
{
    [ -f "$OS_RELEASE" ] && sed -i "/^BUILD_ID=/d" "$OS_RELEASE"

    echo "Adding build-info to /etc/os-release..."
    echo "BUILD_INFO=\"$(whoami)@$(hostname) $(date)${@:+ - $@}\"" >> \
        "$OS_RELEASE"
}

function add_dirs_and_links()
{
    echo "Adding dirs and links..."

    cd "$TARGET_DIR"

    rm -rf mnt/* udisk sdcard data
    mkdir -p mnt/sdcard mnt/udisk
    ln -sf udisk mnt/usb_storage
    ln -sf sdcard mnt/external_sd
    ln -sf mnt/udisk udisk
    ln -sf mnt/sdcard sdcard
    ln -sf userdata data
}

echo "Executing $(basename $0)..."

add_build_info $@
[ -f "$FSTAB" ] && fixup_fstab
add_dirs_and_links

exit 0
