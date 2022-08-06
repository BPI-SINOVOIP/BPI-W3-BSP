SUMMARY = "GTK+ icon theme"
HOMEPAGE = "https://gitlab.gnome.org/GNOME/adwaita-icon-theme"
BUGTRACKER = "https://gitlab.gnome.org/GNOME/adwaita-icon-theme/issues"
SECTION = "x11/gnome"

LICENSE = "LGPL-3.0 | CC-BY-SA-3.0"
LIC_FILES_CHKSUM = "file://COPYING;md5=c84cac88e46fc07647ea07e6c24eeb7c \
                    file://COPYING_CCBYSA3;md5=96143d33de3a79321b1006c4e8ed07e7 \
                    file://COPYING_LGPL;md5=e6a600fd5e1d9cbde2d983680233ad02"

inherit allarch autotools pkgconfig gettext gtk-icon-cache upstream-version-is-even

MAJ_VER = "${@oe.utils.trim_version("${PV}", 2)}"
SRC_URI = "${GNOME_MIRROR}/${BPN}/${MAJ_VER}/${BPN}-${PV}.tar.xz \
           file://0001-Don-t-use-AC_CANONICAL_HOST.patch \
           file://0001-Run-installation-commands-as-shell-jobs.patch \
           "

SRC_URI[md5sum] = "c61ca9d6b3ce70a9ab66dcff945923ff"
SRC_URI[sha256sum] = "e498518627044dfd7db7d79a5b3d437848caf1991ef4ef036a2d3a2ac2c1f14d"

DEPENDS += "librsvg-native"

PACKAGES = "${PN}-cursors ${PN}-symbolic-hires ${PN}-symbolic ${PN}-hires ${PN}"

RREPLACES_${PN} = "gnome-icon-theme"
RCONFLICTS_${PN} = "gnome-icon-theme"
RPROVIDES_${PN} = "gnome-icon-theme"

FILES_${PN}-cursors = "${prefix}/share/icons/Adwaita/cursors/"
FILES_${PN}-symbolic-hires = "${prefix}/share/icons/Adwaita/96x96/*/*.symbolic.png \
                              ${prefix}/share/icons/Adwaita/64x64/*/*.symbolic.png \
                              ${prefix}/share/icons/Adwaita/48x48/*/*.symbolic.png \
                              ${prefix}/share/icons/Adwaita/32x32/*/*.symbolic.png"
FILES_${PN}-symbolic = "${prefix}/share/icons/Adwaita/16x16/*/*.symbolic.png \
                        ${prefix}/share/icons/Adwaita/24x24/*/*.symbolic.png \
                        ${prefix}/share/icons/Adwaita/scalable/*/*-symbolic*.svg"
FILES_${PN}-hires = "${prefix}/share/icons/Adwaita/256x256/ \
                     ${prefix}/share/icons/Adwaita/512x512/"
FILES_${PN} = "${prefix}/share/icons/Adwaita/ \
               ${prefix}/share/pkgconfig/adwaita-icon-theme.pc"

BBCLASSEXTEND = "native nativesdk"
