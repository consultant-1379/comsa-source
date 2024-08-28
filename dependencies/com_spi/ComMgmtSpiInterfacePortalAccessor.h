#ifndef ComMgmtSpiInterfacePortalAccessor_h
#define ComMgmtSpiInterfacePortalAccessor_h
/**
 * Interface portal accessor.
 *
 * @file ComMgmtSpiInterfacePortalAccessor.h
 *
 * ComMgmtSpiInterfacePortalAccessor provides the access to ComMgmtSpiInterfacePortal.
 * The only way for a component to get the accessor is through the comCLMinit function.
 * The interface is not versioned.
 */


/**
 * Interface portal accessor.
 */
typedef struct ComMgmtSpiInterfacePortalAccessor {
    /**
     * Get portal.
     *
     * @param[in] version Version of ComMgmtSpiInterfacePortal.
     *
     * @return Pointer to a version of ComMgmtSpiInterfacePortal,
     * or NULL if the version does not exist.
     *
     * @n @b Example:
     * @code
     *
     * // creating the component
     * ComMgmtSpiComponent_1 myComp = ...
     *
     * // initializing the component
     * ComReturn comCLMinit(ComMgmtSpiInterfacePortalAccessor *accessor, const char * config){
     *
     * // get the version "1" of the portal
     * portal = (ComMgmtSpiInterfacePortal_1T*)accessor->getPortal("1");
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
} ComMgmtSpiInterfacePortalAccessorT;

#endif
