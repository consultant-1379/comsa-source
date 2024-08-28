#ifndef ComMgmtSpiServicesId
#define ComMgmtSpiServicesId

/**
 * Identities definition for the management services.
 *
 * @file ComMgmtSpiServiceIdentities_1.h
 *
 * The id definition is a convenient way for the user of an interface
 * to specify the in-argument when fetching the interface from the
 * ComMgmtSpiInterfacePortal.
 */
#include <ComMgmtSpiInterface_1.h>

/**
 * Identity of the Component Interface SPI.
 */
#define ComMgmtSpiComponentInterfaceName_1 "ComMgmtSpiComponent"
#define ComMgmtSpiComponentInterfaceVersion_1 "1"


/**
 * Identity of the Session Management SPI
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComMgmtSpiSessionManagement_1Id = {"ComMgmtSpiSessionManagementService", "ComMgmtSpiSession", "1"};
#endif
#ifndef __cplusplus
#define ComMgmtSpiSessionManagement_1Id (ComMgmtSpiInterface_1T){"ComMgmtSpiSessionManagementService", "ComMgmtSpiSession", "1"}
#endif

#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComMgmtSpiThreadContextService_2Id = {"ComMgmtSpiThreadContextService", "ComMgmtSpiThreadContext", "2"};
#endif
#ifndef __cplusplus
#define ComMgmtSpiThreadContextService_2Id (ComMgmtSpiInterface_1T){"ComMgmtSpiThreadContextService", "ComMgmtSpiThreadContext", "2"}
#endif

/**
 * Identity of the Thread Context SPI
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComMgmtSpiThreadContext_2Id = {"ComMgmtSpiThreadContextService", "ComMgmtSpiThreadContext", "2"};
#endif
#ifndef __cplusplus
#define ComMgmtSpiThreadContext_2Id (ComMgmtSpiInterface_1T){"ComMgmtSpiThreadContextService", "ComMgmtSpiThreadContext", "2"}
#endif

#endif
