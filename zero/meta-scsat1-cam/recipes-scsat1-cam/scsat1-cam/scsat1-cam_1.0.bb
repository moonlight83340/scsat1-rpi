DESCRIPTION = "scsat1 camera test application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://camera2jpeg.c \
           file://yuyv_to_rgb.h \
           file://Makefile"

S = "${WORKDIR}"

EXTRA_OEMAKE = "DESTDIR=${D} BINDIR=${bindir}"

do_compile:append() {
    oe_runmake
}

do_install:append() {
    oe_runmake install
}

DEPENDS += "jpeg"