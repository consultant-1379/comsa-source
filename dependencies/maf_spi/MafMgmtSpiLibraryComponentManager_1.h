#ifndef MafMgmtSpiLibraryComponentManagerInterface_1_h
#define MafMgmtSpiLibraryComponentManagerInterface_1_h
#include <MafMgmtSpiInterface_1.h>
#include <MafMgmtSpiInterfacePortalAccessor.h>
/**
 * Library component manager.
 *
 * @file MafMgmtSpiLibraryComponentManager_1.h
 * @ingroup MafMgmtSpi
 *
 * The MAF Library Component Manager Interface is implemented by a library
 * provider, which is delivered in a dynamic library. The dynamic library
 * contains one or more components which have to be managed by the MAF Process.
 *
 * The library supplier implements the functions of the MAF Library Component
 * Manager Interface. The library contains one and only one implementation of these functions.
 * The functions are not versioned. If they need to be changed in the future the function
 * names must be changed.
 *
 * The LCM string in the function names is an acronym for LibraryComponentManager.
 * @see MafMgmtSpiInterface_1 for interface example
 */


#ifdef __cplusplus
extern "C"
#endif
/**
 * The function initiates all components according to the supplied
 * configuration in the config argument. If a component has interfaces
 * then they must be registered in the MAF Interface Portal.
 * This function implementation must not depend on other services, that is,
 * must not assume that a service is started or running
 * The function is called by the MAF after opening the dynamic
 * library containing the components that provide the services.
 *
 * @param[in] accessor Interface for retrieving the portal.
 *
 * @param[in] config Configuration for this library.
 *
 * @return MafOk, or MafFailure.
 *
 * @n
 * The following example shows a library with one component which includes one interface
 * @n
 * @b Example:
 * \code
 * #include <MyInterface_1.h> ... // defines MyInterface_1Id
 * #include <MafMgmtSpiInterface_1.h>
 * #include <MafMgmtSpiComponent_1.h>
 * #include <SomeOtherComponentInterface_1.h>  // defines the id
 *
 * MafMgmtSpiInterfacePortal_1T * portal;
 *
 * // declare service interface
 * MyInterface_1T mif = { MyInterface_1Id, myInterfaceFunction };
 *
 * MyInterface_1T *ifArray[] = {(MafMgmtSpiInterface_1T*)&mif, 0};
 *
 * // declare dependency to interfaces
 * MafMgmtSpiInterface_1T dep1 = SomeOtherInterface_1Id;
 * MafMgmtSpiInterface_1T *depsArray[] = {&dep1, 0};
 *
 * // declare component management interface
 * MafMgmtSpiComponent_1T comp1 = {
 *      myComponentInterface_1Id,
 *      &ifArray,
 *      &depsArray,
 *      myCompStartFunction,
 *      myCompStopFunction };
 *
 *
 * MafReturn mafCLMinit(MafMgmtSpiInterfacePortalAccessorT *accessor, const char * config){
 *     // ...
 *     // read out information from the config
 *     // ...
 *
 *     portal = (MafMgmtSpiInterfacePortal_1T*)accessor->getPortal("1");
 *     if(!portal){
 *         // error handling
 *     };
 *     portal->registerComponent(&comp1);
 *     // ...
 *     // more things to do
 * }
 * @endcode
 */
MafReturnT mafLCMinit(MafMgmtSpiInterfacePortalAccessorT
                      *accessor, const char * config);

/**
 * Function pointer type definition used by the COM.
 */
typedef MafReturnT mafLCMinit_t(MafMgmtSpiInterfacePortalAccessorT *accessor, const char * config);

#ifdef __cplusplus
extern "C"
#endif
/**
 * When terminate is called all services included in this library
 * must shut down gracefully and all components and threads
 * must terminate gracefully.
 *
 * Possible reasons: @n
 * &nbsp; 1) The MAF process is about to restart. @n
 * &nbsp; 2) The component is about to be upgraded to a new version. @n
 * &nbsp; 3) The component is about to be permanently decommissioned. @n
 *
 * After this the Library Component Manager is prepared to
 * accept a call to mafLCMinit (all services are started again)
 * or to die (the process terminates or the library is unloaded).
 * The function must always succeed.
 */
void mafLCMterminate();

/**
 * Function pointer type definition used by the MAF.
 */
typedef void mafLCMterminate_t();

#endif
