SUMMARY = "ComSA is COM Service Agent"
DESCRIPTION = "ComSA - Ericsson Common Operations and Maintenance Service Agent"
SECTION = "Ericsson"

# Compile time dependencies
DEPENDS = "coremw"
DEPENDS += "com"
DEPENDS += "libxml2"
DEPENDS += "${@base_contains('DISTRO_FEATURES', 'cba-feature-lttng', 'lttng-ust', '', d)}"

# Run time dependencies
RDEPENDS_${PN} = "coremw"
RDEPENDS_${PN} += "com"
RDEPENDS_${PN} += "libxml2"
RDEPENDS_${PN} += "${@base_contains('DISTRO_FEATURES', 'cba-feature-lttng', 'lttng-ust', '', d)}"

HOMEPAGE = "http://www.ericsson.com"
LICENSE = "CLOSED"

# Package Revision, Update this whenever you change the recipe, not the revision of the source
# Should be reset to r0 when changing filename, due to bringing in new version of the source.
PR = "r21"

COMSASRCURI = "git://gerritforge.lmera.ericsson.se:29418/comsa-source;protocol=ssh"
SRCREV = "R1A06"
SRC_URI = "${COMSASRCURI}"
SRC_URI += "file://top_makefile.patch"
SRC_URI += "file://single_system_model.xml"
SRC_URI += "file://dual_system_model.xml"

PARALLEL_MAKE = ""
require conf/cba_pso.conf
require conf/cba_comp_rstate.conf
require conf/cluster_size.conf

S = "${WORKDIR}/git"

EXTRA_OEMAKE = "'COMSA_DEV_DIR=${S}'"
# Don't build Performance Management Transfer service agent
EXTRA_OEMAKE += "${@base_contains('DISTRO_FEATURES', 'cba-feature-disable-pm', 'DISABLE_PMT_SA=YES', 'DISABLE_PMT_SA=NO', d)}"
EXTRA_OEMAKE += "'SA_VERSION=${cba_com-sa_rstate}'"
EXTRA_OEMAKE += "'ARCHITECTURE=${TARGET_ARCH}'"
EXTRA_OEMAKE += "'CC=${CC}'"
EXTRA_OEMAKE += "'CXX=${CXX}'"
EXTRA_OEMAKE += "'CFLAGS=${CFLAGS} -Wall -fPIC -g -pthread'"
EXTRA_OEMAKE += "'CXXFLAGS=${CXXFLAGS} -Wall -fPIC -g -pthread'"
EXTRA_OEMAKE += "'CPPFLAGS=${CPPFLAGS} \
                           -I${STAGING_INCDIR}/inc \
                           -I${STAGING_INCDIR}/libxml2'"
EXTRA_OEMAKE += "'LDFLAGS=-L${STAGING_LIBDIR} -lpthread'"
EXTRA_OEMAKE += "'COMSA_RELEASE=${S}/out'"
EXTRA_OEMAKE += "'LSB_SHAREDLIBPATH=${STAGING_LIBDIR}'"
EXTRA_OEMAKE += "'COM_SA_RESULT=${S}/buildOutputComSa'"
EXTRA_OEMAKE += "'SAF_INCL= ' 'COM_INCL= ' 'MAF_INCL= '"
EXTRA_OEMAKE += "'STORAGE_CONFIG=${cba_pso_path_config}' \
                 'STORAGE_SOFTWARE=${cba_pso_path_software}' \
                 'STORAGE_CLEAR=${cba_pso_path_clear}' \
                 'STORAGE_NOBACKUP=${cba_pso_path_no-backup}'"
EXTRA_OEMAKE += "'DATADIR=${datadir}'"
EXTRA_OEMAKE += "'BINDIR=${bindir}'"
EXTRA_OEMAKE += "'COMCOMPDIR= /opt/com/lib/comp'"
EXTRA_OEMAKE += "'PN=${PN}'"
EXTRA_OEMAKE += "${@base_contains('DISTRO_FEATURES', 'cba-feature-lttng', 'LTT_LIB_DIR=-L${STAGING_LIBDIR}', '', d)}"
EXTRA_OEMAKE += "${@base_contains('DISTRO_FEATURES', 'cba-feature-lttng', 'TRACEPROBE_LDFLAGS= -llttng-ust -llttng-ust-fork', '', d)}"

do_install() {
    oe_runmake install DESTDIR=${D}

    # Until com-sa install do it themselves
    install -d ${D}${bindir}
    install -d ${D}${datadir}/${PN}/imm_objects

    if [ ${cluster_size} -eq 1 ] ; then
        install -m 644 ${WORKDIR}/single_system_model.xml ${D}${datadir}/${PN}/imm_objects
        sed -i "s,@cba_com-sa_rstate@,${cba_com-sa_rstate},g"  ${D}${datadir}/${PN}/imm_objects/single_system_model.xml
    else
        install -m 644 ${WORKDIR}/dual_system_model.xml ${D}${datadir}/${PN}/imm_objects
        sed -i "s,@cba_com-sa_rstate@,${cba_com-sa_rstate},g"  ${D}${datadir}/${PN}/imm_objects/dual_system_model.xml
    fi
}

FILES_${PN} += "/opt/com/*"
FILES_${PN} += "/var/opt/comsa"
FILES_${PN} += "${cba_pso_path_software}/*"
FILES_${PN} += "${cba_pso_path_config}/*"
FILES_${PN} += "${cba_pso_path_clear}/*"
FILES_${PN} += "${cba_pso_path_no-backup}/*"

FILES_${PN}-dbg = "/opt/com/lib/comp/.debug"
FILES_${PN}-src = "/usr/src/*"

INSANE_SKIP_${PN} = "dev-so"

PACKAGES =  "${PN}-dbg"
PACKAGES += "${PN}-src"
PACKAGES += "${PN}"
