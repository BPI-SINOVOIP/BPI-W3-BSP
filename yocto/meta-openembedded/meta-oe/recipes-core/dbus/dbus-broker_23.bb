SUMMARY = "dbus broker"
DESCRIPTION = "Drop-in replacement for dbus-daemon."

SECTION = "base"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=7b486c2338d225a1405d979ed2c15ce8"

SRC_URI = "https://github.com/bus1/dbus-broker/releases/download/v${PV}/dbus-broker-${PV}.tar.xz"
SRC_URI[sha256sum] = "95adfde56bce898c3b69eee0524732365e802348dd8189a35d5d00c30990dc81"

UPSTREAM_CHECK_URI = "https://github.com/bus1/${BPN}/releases"

inherit meson pkgconfig systemd features_check

DEPENDS = "expat systemd"
DEPENDS += " ${@bb.utils.contains('DISTRO_FEATURES', 'selinux', 'libselinux', '', d)}"
DEPENDS += " ${@bb.utils.contains('DISTRO_FEATURES', 'selinux', 'audit', '', d)}"

REQUIRED_DISTRO_FEATURES = "systemd"

SYSTEMD_SERVICE_${PN} = "${BPN}.service"

FILES_${PN} += "${systemd_system_unitdir}"
FILES_${PN} += "${systemd_user_unitdir}"
FILES_${PN} += "${nonarch_libdir}/systemd/catalog"

EXTRA_OEMESON += " -Dselinux=${@bb.utils.contains('DISTRO_FEATURES', 'selinux', 'true', 'false', d)}"
EXTRA_OEMESON += " -Daudit=${@bb.utils.contains('DISTRO_FEATURES', 'selinux', 'true', 'false', d)}"

