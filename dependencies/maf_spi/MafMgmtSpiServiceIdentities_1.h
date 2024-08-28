#ifndef MafMgmtSpiServicesId
#define MafMgmtSpiServicesId

/**
 * Identities definition for the managment services.
 *
 * @file MafMgmtSpiServiceIdentities_1.h
 * @ingroup MafMgmtSpi
 *
 * The id definition is a convenient way for the user of an interface
 * to specify the in-argument when fetching the interface from the
 * MafMgmtSpiInterfacePortal.
 */
#include <MafMgmtSpiInterface_1.h>

/**
 * Identity of the Component Interface SPI.
 */
#define MafMgmtSpiComponentInterfaceName_1 "MafMgmtSpiComponent"
#define MafMgmtSpiComponentInterfaceVersion_1 "1"
#define MafMgmtSpiComponentInterfaceVersion_2 "2"


/**
 * Identity of the Session Management SPI
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafMgmtSpiSessionManagement_1Id = {"MafMgmtSpiSessionManagementService", "MafMgmtSpiSession", "1"};
#endif
#ifndef __cplusplus
#define MafMgmtSpiSessionManagement_1Id (MafMgmtSpiInterface_1T){"MafMgmtSpiSessionManagementService", "MafMgmtSpiSession", "1"}
#endif

#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafMgmtSpiThreadContextService_2Id = {"MafMgmtSpiThreadContextService", "MafMgmtSpiThreadContext", "2"};
#endif
#ifndef __cplusplus
#define MafMgmtSpiThreadContextService_2Id (MafMgmtSpiInterface_1T){"MafMgmtSpiThreadContextService", "MafMgmtSpiThreadContext", "2"}
#endif

/**
 * Identity of the Thread Context SPI
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafMgmtSpiThreadContext_2Id = {"MafMgmtSpiThreadContextService", "MafMgmtSpiThreadContext", "2"};
#endif
#ifndef __cplusplus
#define MafMgmtSpiThreadContext_2Id (MafMgmtSpiInterface_1T){"MafMgmtSpiThreadContextService", "MafMgmtSpiThreadContext", "2"}
#endif

#endif
