SUMMARY = "HTTP/2-based RPC framework"
DESCRIPTION = "Google gRPC"
HOMEPAGE = "http://www.grpc.io/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=3b83ef96387f14655fc854ddc3c6bd57"

SRC_URI_append_class-target = " file://0001-setup.py-Do-not-mix-C-and-C-compiler-options.patch \
                                file://ppc-boringssl-support.patch \
                                file://riscv64_support.patch \
"
SRC_URI[md5sum] = "ccaf4e7eb4f031d926fb80035d193b98"
SRC_URI[sha256sum] = "a899725d34769a498ecd3be154021c4368dd22bdc69473f6ec46779696f626c4"

DEPENDS_append = " ${PYTHON_PN}-protobuf"

inherit pypi setuptools

RDEPENDS_${PN} = "\
    ${PYTHON_PN}-protobuf \
    ${PYTHON_PN}-setuptools \
    ${PYTHON_PN}-six \
    python-enum34 \
    python-futures \
"

export GRPC_PYTHON_DISABLE_LIBC_COMPATIBILITY = "1"

CLEANBROKEN = "1"

BBCLASSEXTEND = "native nativesdk"
