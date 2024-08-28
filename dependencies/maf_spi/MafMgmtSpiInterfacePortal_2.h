#ifndef MafMgmtSpiInterfacePortal_2_h
#define MafMgmtSpiInterfacePortal_2_h
#include <MafMgmtSpiCommon.h>
#include <MafMgmtSpiInterface_1.h>
#include <MafMgmtSpiComponent_2.h>
/**
 * Interface portal.
 *
 * @file MafMgmtSpiInterfacePortal_2.h
 * @ingroup MafMgmtSpi
 *
 * MafMgmtSpiInterfacePortal_2 contains function pointers for accessing the
 * MAF Interface Portal Service.
 *
 * The portal enables components to publish and retreive service interfaces.
 *
 * The only way for a component to get MafMgmtSpiInterfacePortal is through the
 * non-versioned MafMgmtSpiInterfacePortalAccessor.
 * The only way for a component to get the accessor is through the mafLCMinit
 * function which is called by MAF when the component library is initiated.
 * Thus, the Library Component Manager must at initiation store a pointer to the portal
 * interface locally if it or its components needs to use the portal later.
 */


/**
 * Interface portal.
 *
 * MafMgmtSpiInterfacePortal struct contains the "base class" interface and
 * the function pointers that the interface provides.
 */
typedef struct MafMgmtSpiInterfacePortal_2 {
    /**
     * The "base class" for this interface.
     */
    MafMgmtSpiInterface_1T mafInterface;
    /**
     * Retrieve an interface that has been registered in the MAF Portal Service.
     *
     * @param[in] interfaceId Id definition provided in the include file of the interface.
     * If the caller retrieving the interface does not know or care which component that implements
     * the interface then the caller can set componentName to null in interfaceId struct.
     * If so, the dependency to this interface must be specified in the same way in the call
     * to registerComponent.
     *
     * @param[out] result Location referenced by result is set to point to the interface.
     *
     * @return MafOk, or @n
     * MafNotExist if the interface does not exist, or @n
     * MafInvalidArgument if a parameter is null (except the component name), or @n
     * one of the other MafReturnT return codes.
     *
     * @n @b Example:
     * \code
     * #include <SomeInterface_1.h> .. provides SomeInterface_1Id
     * MafMgmtSpiInterfacePortal_1T *portal; ..set in MafCLMinit function
     * ...
     * SomeInterface_1T *si;
     * if(MafOk != portal->get(SomeInterface_1Id,
     *     (MafMgmtSpiInterface*)si){ ..error handling }
     * si->someFunction();
     * @endcode
     */
    MafReturnT (*getInterface)( MafMgmtSpiInterface_1T interfaceId, MafMgmtSpiInterface_1T **result);
    /**
     * Retrieve a null terminated array containing all the registered interfaces that match with interface
     * and version.
     *
     * @param[in] interface Name of the interface.
     *
     * @param[in] version Version of the interface.
     *
     * @param[out] result Location referenced by result is set to a
     *       null-terminated array of MafMgmtSpiInterface_1
     *       pointers.
     *
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getInterfaceArray) (const char *interface, const char * version, MafMgmtSpiInterface_1T ***result);
    /**
     * Registers a component in the portal, all the interfaces it
     * provides and all dependencies it has to other interfaces.
     * The component must stay registered until it is terminated.
     * The interfaces instances provided must exist until the component is terminated.
     *
     * @param[in] component Pointer to the component interface that is to be registered.
     * The registered MafMgmtSpiComponent struct must exist until the component is unregistered.
     *
     * @return  MafOK, or @n
     * MafInvalidArgument if a parameter is null, or @n
     * one of the other MafReturnT return codes.
     *
     * @n See the MafLibrarComponentManager function mafLCMinit for a code example.
     */
    MafReturnT (*registerComponent)(MafMgmtSpiComponent_2T *component);
    /**
     * Unregister a component.
     *
     * @param[in] component Pointer to the component interface that is to
     * be unregistered (the same pointer that was previously registered).
     *
     * @return  MafOk, or @n
     * MafNotExist if the component is not registered, or @n
     * MafInvalidArgument if a parameter is null, or @n
     * one of the other MafReturnT return codes.
     */
    MafReturnT (*unregisterComponent)(MafMgmtSpiComponent_2T *component);
} MafMgmtSpiInterfacePortal_2T;

#endif
