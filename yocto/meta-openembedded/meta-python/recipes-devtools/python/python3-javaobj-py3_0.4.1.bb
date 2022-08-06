SUMMARY = "Module for serializing and de-serializing Java objects."
DESCRIPTION = "python-javaobj is a python library that provides functions\
 for reading and writing (writing is WIP currently) Java objects serialized\
 or will be deserialized by ObjectOutputStream. This form of object\
 representation is a standard data interchange format in Java world."
HOMEPAGE = "https://github.com/tcalmant/python-javaobj"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=d273d63619c9aeaf15cdaf76422c4f87"

SRC_URI[md5sum] = "47632071c3c3ca14b6c42f2a4e2e1309"
SRC_URI[sha256sum] = "419ff99543469e68149f875abb0db5251cecd350c03d2bfb4c94a5796f1cbc14"

inherit pypi setuptools3

BBCLASSEXTEND = "native nativesdk"
