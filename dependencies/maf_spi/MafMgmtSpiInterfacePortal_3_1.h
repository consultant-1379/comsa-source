/*
 * MafMgmtSpiInterfacePortal_3_1.h
 *
 *  Created on: Sep 1, 2014
 *      Author: xseemah
 */

#ifndef MafMgmtSpiInterfacePortal_3_1_h
#define MafMgmtSpiInterfacePortal_3_1_h
#include <MafMgmtSpiInterfacePortal_3.h>
/**
 * Interface portal.
 *
 * @file MafMgmtSpiInterfacePortal_3_1.h
 * @ingroup MafMgmtSpi
 *
 *
 * Contains optional function pointers for accessing the
 * MAF Interface Portal Service.
 *
 * The portal enables components to publish and retrieve service interfaces.
 *
 */

/**
 * Interface portal.
 *
 * MafMgmtSpiInterfacePortal struct contains the "base class" interface and
 * the function pointers that the interface provides.
 */
typedef struct MafMgmtSpiInterfacePortal_3_1 {
    /**
     * Common interface description. The "base class" for this
     * interface contains the component name, interface name, and
     * interface version.
     */
    MafMgmtSpiInterface_1T mafInterface;

    /**
     * Retrieve a null terminated array containing all the registered interfaces that match
     * with component name and interface.
     *
     * The returned array is allocated by the Portal using malloc(). The caller is
     * responsible to free the array after usage. The caller must use free() for the
     * deallocation.
     *
     * @param[in] componentName Name of the Component.
     *
     * @param[in] interface Name of the interface. If the input value is NULL, then all the
     *            interfaces of the specified 'componentName' argument  will be returned.
     *            Otherwise only the given input match will be returned.
     *
     * @param[out] result Location referenced by result is set to a
     *             null-terminated array of MafMgmtSpiInterface_1
     *             pointers. It is modified only if the return value is MafOk.
     *
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getComponentInterfaceArray) (const char * componentName,
            const char * interface,
            MafMgmtSpiInterface_1T ***result);

} MafMgmtSpiInterfacePortal_3_1T;

#endif
