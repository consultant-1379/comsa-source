#ifndef MafMgmtSpiInterfacePortalAccessor_h
#define MafMgmtSpiInterfacePortalAccessor_h
/**
 * Interface portal accessor.
 *
 * @file MafMgmtSpiInterfacePortalAccessor.h
 * @ingroup MafMgmtSpi
 *
 * MafMgmtSpiInterfacePortalAccessor provides the access to MafMgmtSpiInterfacePortal.
 * The only way for a component to get the accessor is through the mafCLMinit function.
 * The interface is not versioned.
 */


/**
 * Interface portal accessor.
 */
typedef struct MafMgmtSpiInterfacePortalAccessor {
    /**
     * Get portal.
     *
     * @param[in] version Version of MafMgmtSpiInterfacePortal.
     *
     * @return Pointer to a version of MafMgmtSpiInterfacePortal,
     * or NULL if the version does not exist.
     *
     * @n @b Example:
     * @code
     *
     * // creating the component
     * MafMgmtSpiComponent_1 myComp = ...
     *
     * // initializing the component
     * MafReturn mafCLMinit(MafMgmtSpiInterfacePortalAccessor *accessor, const char * config){
     *
     * // get the version "1" of the portal
     * portal = (MafMgmtSpiInterfacePortal_1T*)accessor->getPortal("1");
     *
     * if(!portal) {
     *      // handling the case when the version "1" of the portal
     *      // is not supported
     * } else {
     *      // continue with registring the component
     *      portal->register(&myComp);
     * }
     * @endcode
     */
    void * (*getPortal)(const char *version);
} MafMgmtSpiInterfacePortalAccessorT;

#endif
