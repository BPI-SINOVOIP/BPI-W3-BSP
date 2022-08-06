SUMMARY = "Library for reading extended image information (EXIF) from JPEG files"
HOMEPAGE = "https://libexif.github.io/"
SECTION = "libs"
LICENSE = "LGPLv2.1"
LIC_FILES_CHKSUM = "file://COPYING;md5=243b725d71bb5df4a1e5920b344b86ad"

def version_underscore(v):
    return "_".join(v.split("."))

SRC_URI = "https://github.com/libexif/libexif/releases/download/libexif-${@version_underscore("${PV}")}-release/libexif-${PV}.tar.xz \
           "

SRC_URI[sha256sum] = "5048f1c8fc509cc636c2f97f4b40c293338b6041a5652082d5ee2cf54b530c56"

UPSTREAM_CHECK_URI = "https://github.com/libexif/libexif/releases/"

inherit autotools gettext

EXTRA_OECONF += "--disable-docs"

BBCLASSEXTEND = "native nativesdk"
