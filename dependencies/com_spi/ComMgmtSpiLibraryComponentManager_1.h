#ifndef ComMgmtSpiLibraryComponentManagerInterface_1_h
#define ComMgmtSpiLibraryComponentManagerInterface_1_h
#include <ComMgmtSpiInterface_1.h>
#include <ComMgmtSpiInterfacePortalAccessor.h>
/**
 * Library component manager.
 *
 * @file ComMgmtSpiLibraryComponentManager_1.h
 *
 * The COM Library Component Manager Interface is implemented by a library
 * provider, which is delivered in a dynamic library. The dynamic library
 * contains one or more components which have to be managed by the COM Process.
 *
 * The library supplier implements the functions of the COM Library Component
 * Manager Interface. The library contains one and only one implementation of these functions.
 * The functions are not versioned. If they need to be changed in the future the function
 * names must be changed.
 *
 * The LCM string in the function names is an acronym for LibraryComponentManager.
 * @see ComMgmtSpiInterface_1 for interface example
 */


#ifdef __cplusplus
extern "C"
#endif
/**
 * The function initiates all components according to the supplied
 * configuration in the config argument. If a component has interfaces
 * then they must be registered in the COM Interface Portal.
 * This function implementation must not depend on other services, that is,
 * must not assume that a service is started or running
 * the function is called by the COM after opening the dynamic
 * library containing the components that provide the services.
 *
 * @param[in] accessor Interface for retrieving the portal.
 *
 * @param[in] config Configuration for this library.
 *
 * @return ComOk, or ComFailure.
 *
 * @n
 * The following example shows a library with one component which includes one interface
 * @n
 * @b Example:
 * \code
 * #include <MyInterface_1.h> ... // defines MyInterface_1Id
 * #include <ComMgmtSpiInterface_1.h>
 * #include <ComMgmtSpiComponent_1.h>
 * #include <SomeOtherComponentInterface_1.h>  // defines the id
 *
 * ComMgmtSpiInterfacePortal_1T * portal;
 *
 * // declare service interface
 * MyInterface_1T mif = { MyInterface_1Id, myInterfaceFunction };
 *
 * MyInterface_1T *ifArray[] = {(ComMgmtSpiInterface_1T*)&mif, 0};
 *
 * // declare dependency to interfaces
 * ComMgmtSpiInterface_1T dep1 = SomeOtherInterface_1Id;
 * ComMgmtSpiInterface_1T *depsArray[] = {&dep1, 0};
 *
 * // declare component management interface
 * ComMgmtSpiComponent_1T comp1 = {
 *      myComponentInterface_1Id,
 *      &ifArray,
 *      &depsArray,
 *      myCompStartFunction,
 *      myCompStopFunction };
 *
 *
 * ComReturn comCLMinit(ComMgmtSpiInterfacePortalAccessorT *accessor, const char * config){
 *     // ...
 *     // read out information from the config
 *     // ...
 *
 *     portal = (ComMgmtSpiInterfacePortal_1T*)accessor->getPortal("1");
 *     if(!portal){
 *         // error handling
 *     };
 *     portal->registerComponent(&comp1);
 *     // ...
 *     // more things to do
 * }
 * @endcode
 */
ComReturnT comLCMinit(ComMgmtSpiInterfacePortalAccessorT
                      *accessor, const char * config);

/**
 * Function pointer type definition used by the COM.
 */
typedef ComReturnT comLCMinit_t(ComMgmtSpiInterfacePortalAccessorT *accessor, const char * config);

#ifdef __cplusplus
extern "C"
#endif
/**
 * When terminate is called all services included in this library
 * must shut down gracefully and all components and threads
 * must terminate gracefully.
 *
 * Possible reasons: @n
 * &nbsp; 1) The COM process is about to restart. @n
 * &nbsp; 2) The component is about to be upgraded to a new version. @n
 * &nbsp; 3) The component is about to be permanently decommissioned. @n
 *
 * After this the Library Component Manager is prepared to
 * accept a call to comLCMinit (all services are started again)
 * or to die (the process terminates or the library is unloaded).
 * The function must always succeed.
 */
void comLCMterminate();

/**
 * Function pointer type definition used by the COM.
 */
typedef void comLCMterminate_t();

#endif
