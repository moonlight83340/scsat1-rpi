LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2915dc85ab8fd26629e560d023ef175c"

SRCREV = "${AUTOREV}"
SRCBRANCH = "develop"
SRC_URI = "git://github.com/libcsp/libcsp.git;protocol=https;branch=${SRCBRANCH};"

DEPENDS += "libsocketcan"

PACKAGES = "${PN} ${PN}-dbg ${PN}-dev"

FILES:${PN}-dbg += "${libdir}/.debug"
FILES:${PN}     += "${libdir}/libcsp.so"
FILES:${PN}-dev += "${includedir}/csp"

S = "${WORKDIR}/git"

inherit cmake pkgconfig

EXTRA_OECMAKE += "-DCSP_USE_RTABLE=ON"

do_install:append() {
    chmod 644 ${D}${libdir}/libcsp.so
}
