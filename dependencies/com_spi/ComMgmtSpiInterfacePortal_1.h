#ifndef ComMgmtSpiInterfacePortal_1_h
#define ComMgmtSpiInterfacePortal_1_h
#include <ComMgmtSpiCommon.h>
#include <ComMgmtSpiInterface_1.h>
#include <ComMgmtSpiComponent_1.h>
/**
 * Interface portal.
 *
 * @file ComMgmtSpiInterfacePortal_1.h
 *
 * ComMgmtSpiInterfacePortal_1 contains function pointers for accessing the
 * COM Interface Portal Service.
 *
 * The portal enables components to publish and retrieve service interfaces.
 *
 * The only way for a component to get ComMgmtSpiInterfacePortal is through the
 * non-versioned ComMgmtSpiInterfacePortalAccessor.
 * The only way for a component to get the accessor is through the comLCMinit
 * function which is called by COM when the component library is initiated.
 * Thus, the Library Component Manager must at initiation store a pointer to the portal
 * interface locally if it or its components needs to use the portal later.
 */


/**
 * Interface portal.
 *
 * ComMgmtSpiInterfacePortal struct contains the "base class" interface and
 * the function pointers that the interface provides.
 */
typedef struct ComMgmtSpiInterfacePortal_1 {
    /**
     * The "base class" for this interface.
     */
    ComMgmtSpiInterface_1T comInterface;
    /**
     * Retrieve an interface that has been registered in the COM Portal Service.
     *
     * @param[in] interfaceId Id definition provided in the include file of the interface.
     * If the caller retrieving the interface does not know or care which component that implements
     * the interface then the caller can set componentName to null in interfaceId struct.
     * If so, the dependency to this interface must be specified in the same way in the call
     * to registerComponent.
     *
     * @param[out] result Location referenced by result is set to point to the interface.
     *
     * @return ComOk, or @n
     * ComNotExist if the interface does not exist, or @n
     * ComInvalidArgument if a parameter is null (except the component name), or @n
     * one of the other ComReturnT return codes.
     *
     * @n @b Example:
     * \code
     * #include <SomeInterface_1.h> .. provides SomeInterface_1Id
     * ComMgmtSpiInterfacePortal_1T *portal; ..set in ComCLMinit function
     * ...
     * SomeInterface_1T *si;
     * if(ComOk != portal->get(SomeInterface_1Id,
     *     (ComMgmtSpiInterface*)si){ ..error handling }
     * si->someFunction();
     * @endcode
     */
    ComReturnT (*getInterface)( ComMgmtSpiInterface_1T interfaceId, ComMgmtSpiInterface_1T **result);
    /**
     * Retrieve a null terminated array containing all the registered interfaces that match with interface
     * and version.
     *
     * @param[in] interface Name of the interface.
     *
     * @param[in] version Version of the interface.
     *
     * @param[out] result Location referenced by result is set to a
     *       null-terminated array of ComMgmtSpiInterface_1
     *       pointers.
     *
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getInterfaceArray) (const char *interface, const char * version, ComMgmtSpiInterface_1T ***result);
    /**
     * Registers a component in the portal, all the interfaces it
     * provides and all dependencies it has to other interfaces.
     * The component must stay registered until it is terminated.
     * The interfaces instances provided must exist until the component is terminated.
     *
     * @param[in] component Pointer to the component interface that is to be registered.
     * The registered ComMgmtSpiComponent struct must exist until the component is unregistered.
     *
     * @return  ComOK, or @n
     * ComInvalidArgument if a parameter is null, or @n
     * one of the other ComReturnT return codes.
     *
     * @n See the ComLibrarComponentManager function comLCMinit for a code example.
     */
    ComReturnT (*registerComponent)(ComMgmtSpiComponent_1T *component);
    /**
     * Unregister a component.
     *
     * @param[in] component Pointer to the component interface that is to
     * be unregistered (the same pointer that was previously registered).
     *
     * @return  ComOk, or @n
     * ComNotExist if the component is not registered, or @n
     * ComInvalidArgument if a parameter is null, or @n
     * one of the other ComReturnT return codes.
     */
    ComReturnT (*unregisterComponent)(ComMgmtSpiComponent_1T *component);
} ComMgmtSpiInterfacePortal_1T;

#endif
