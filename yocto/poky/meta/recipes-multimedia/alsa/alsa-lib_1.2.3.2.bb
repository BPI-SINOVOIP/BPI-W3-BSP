SUMMARY = "ALSA sound library"
HOMEPAGE = "http://www.alsa-project.org"
BUGTRACKER = "http://alsa-project.org/main/index.php/Bug_Tracking"
SECTION = "libs/multimedia"
LICENSE = "LGPLv2.1 & GPLv2+"
LIC_FILES_CHKSUM = "file://COPYING;md5=a916467b91076e631dd8edb7424769c7 \
                    file://src/socket.c;md5=285675b45e83f571c6a957fe4ab79c93;beginline=9;endline=24 \
                    "

SRC_URI = "https://www.alsa-project.org/files/pub/lib/${BP}.tar.bz2"
SRC_URI[sha256sum] = "e81fc5b7afcaee8c9fd7f64a1e3043e88d62e9ad2c4cff55f578df6b0a9abe15"

inherit autotools pkgconfig

EXTRA_OECONF += " \
    ${@bb.utils.contains('TARGET_FPU', 'soft', '--with-softfloat', '', d)} \
    --disable-python \
"

PACKAGES =+ "alsa-server alsa-conf libatopology"

FILES_alsa-server = "${bindir}/*"
FILES_alsa-conf = "${datadir}/alsa/"
FILES_libatopology = "${libdir}/libatopology.so.*"

RDEPENDS_${PN}_class-target = "alsa-conf alsa-ucm-conf"
RDEPENDS_libatopology_class-target = "alsa-topology-conf"

# upgrade path
RPROVIDES_${PN} = "libasound"
RREPLACES_${PN} = "libasound"
RCONFLICTS_${PN} = "libasound"

RPROVIDES_${PN}-dev = "alsa-dev"
RREPLACES_${PN}-dev = "alsa-dev"
RCONFLICTS_${PN}-dev = "alsa-dev"

RPROVIDES_alsa-conf = "alsa-conf-base"
RREPLACES_alsa-conf = "alsa-conf-base"
RCONFLICTS_alsa-conf = "alsa-conf-base"

BBCLASSEXTEND = "native nativesdk"
