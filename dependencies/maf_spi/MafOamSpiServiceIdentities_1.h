#ifndef MafOamSpiServicesId
#define MafOamSpiServicesId

/**
 * Identities definition for the operation and maintenance services.
 *
 * @file MafOamSpiServiceIdentities_1.h
 * @ingroup MafOamSpi
 *
 * This file specifies the interface identities used when fetching
 * the interface from the MafMgmtSpiInterfacePortal.
 *
 * Considering usability reasons, the version of this file might not be
 * incremented as long as the change introduced remains backwards compatible.
 */
#include <MafMgmtSpiInterface_1.h>


/**
 * Identity of the Managed Object SPI implemented by the CM Router service.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiCmRouterService_1Id = {"MafOamSpiCmRouterService", "MafOamSpiManagedObject", "1"};
#endif
#ifndef __cplusplus
#define MafOamSpiCmRouterService_1Id (MafMgmtSpiInterface_1T){"MafOamSpiCmRouterService", "MafOamSpiManagedObject", "1"}
#endif

#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiCmRouterService_2Id = {"MafOamSpiCmRouterService", "MafOamSpiManagedObject", "2"};
#endif
#ifndef __cplusplus
#define MafOamSpiCmRouterService_2Id (MafMgmtSpiInterface_1T){"MafOamSpiCmRouterService", "MafOamSpiManagedObject", "2"}
#endif

#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiCmRouterService_3Id = {"MafOamSpiCmRouterService", "MafOamSpiManagedObject", "3"};
#endif
#ifndef __cplusplus
#define MafOamSpiCmRouterService_3Id (MafMgmtSpiInterface_1T){"MafOamSpiCmRouterService", "MafOamSpiManagedObject", "3"}
#endif

#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiCmRouterService_3_1Id = {"MafOamSpiCmRouterService", "MafOamSpiManagedObject", "3_1"};
#endif
#ifndef __cplusplus
#define MafOamSpiCmRouterService_3_1Id (MafMgmtSpiInterface_1T){"MafOamSpiCmRouterService", "MafOamSpiManagedObject", "3_1"}
#endif

/**
 * Identity of the Model Repository SPI implemented by the Model Repository service.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiModelRepository_1Id = {"MafOamSpiModelRepositoryService", "MafOamSpiModelRepository", "1"};
#endif
#ifndef __cplusplus
#define MafOamSpiModelRepository_1Id (MafMgmtSpiInterface_1T){"MafOamSpiModelRepositoryService", "MafOamSpiModelRepository", "1"}
#endif

/**
 * Identity of the Model Repository SPI II implemented by the Model Repository service.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiModelRepository_2Id = {"MafOamSpiModelRepositoryService", "MafOamSpiModelRepository", "2"};
#endif
#ifndef __cplusplus
#define MafOamSpiModelRepository_2Id (MafMgmtSpiInterface_1T){"MafOamSpiModelRepositoryService", "MafOamSpiModelRepository", "2"}
#endif

/**
 * Identity of the Model Repository SPI 3 implemented by the Model Repository service.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiModelRepository_3Id = {"MafOamSpiModelRepositoryService", "MafOamSpiModelRepository", "3"};
#endif
#ifndef __cplusplus
#define MafOamSpiModelRepository_3Id (MafMgmtSpiInterface_1T){"MafOamSpiModelRepositoryService", "MafOamSpiModelRepository", "3"}
#endif

/**
 * Identity of the Model Repository SPI 4 implemented by the Model Repository service.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiModelRepository_4Id = {"MafOamSpiModelRepositoryService", "MafOamSpiModelRepository", "4"};
#endif
#ifndef __cplusplus
#define MafOamSpiModelRepository_4Id (MafMgmtSpiInterface_1T){"MafOamSpiModelRepositoryService", "MafOamSpiModelRepository", "4"}
#endif

/**
 * Identity of the Transaction SPI implemented by the Transaction service.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiTransaction_1Id = {"MafOamSpiTransactionService", "MafOamSpiTransaction", "1"};
#endif
#ifndef __cplusplus
#define MafOamSpiTransaction_1Id (MafMgmtSpiInterface_1T){"MafOamSpiTransactionService", "MafOamSpiTransaction", "1"}
#endif

/**
 * Identity of the Transaction Master SPI implemented by the Transaction service.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiTransactionMaster_1Id = {"MafOamSpiTransactionService", "MafOamSpiTransactionMaster", "1"};
#endif
#ifndef __cplusplus
#define MafOamSpiTransactionMaster_1Id (MafMgmtSpiInterface_1T){"MafOamSpiTransactionService", "MafOamSpiTransactionMaster", "1"}
#endif
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiTransaction_2Id = {"MafOamSpiTransactionService", "MafOamSpiTransaction", "2"};
#endif
#ifndef __cplusplus
#define MafOamSpiTransaction_2Id (MafMgmtSpiInterface_1T){"MafOamSpiTransactionService", "MafOamSpiTransaction", "2"}
#endif

/**
 * Identity of the Transaction Resource SPI implemented by any service.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiTransactionalResource_1Id = {0, "MafOamSpiTransactionalResource", "1"};
#endif
#ifndef __cplusplus
#define MafOamSpiTransactionalResource_1Id (MafMgmtSpiInterface_1T){NULL, "MafOamSpiTransactionalResource", "1"}
#endif

#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiTransactionMaster_2Id = {"MafOamSpiTransactionService", "MafOamSpiTransactionMaster", "2"};
#endif
#ifndef __cplusplus
#define MafOamSpiTransactionMaster_2Id (MafMgmtSpiInterface_1T){"MafOamSpiTransactionService", "MafOamSpiTransactionMaster", "2"}
#endif

/**
 * Identity of the Notification Producer SPI implemented by any service.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiNotificationProducer_1Id = { 0, "MafOamSpiNotificationProducer", "1" };
#endif
#ifndef __cplusplus
#define MafOamSpiNotificationProducer_1Id (MafMgmtSpiInterface_1T){NULL, "MafOamSpiNotificationProducer", "1"}
#endif
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiTransactionalResource_2Id = {0, "MafOamSpiTransactionalResource", "2"};
#endif
#ifndef __cplusplus
#define MafOamSpiTransactionalResource_2Id (MafMgmtSpiInterface_1T){NULL, "MafOamSpiTransactionalResource", "2"}
#endif


/**
 * Identity of the Event service SPI
 */

#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiEventService_1Id = {"MafOamSpiEventService", "MafOamSpiEventRouter", "1"};
#endif
#ifndef __cplusplus
#define MafOamSpiEventService_1Id (MafMgmtSpiInterface_1T){"MafOamSpiEventService", "MafOamSpiEventRouter", "1"}
#endif

/**
 * Identity of the Security Management SPI
 */

#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiSecurityManagement_1Id = {"MafOamSpiSecurityManagementService", "MafOamSpiSecurityManagement", "1"};
#endif
#ifndef __cplusplus
#define MafOamSpiSecurityManagement_1Id (MafMgmtSpiInterface_1T){"MafOamSpiSecurityManagementService", "MafOamSpiSecurityManagement", "1"}
#endif

/**
 * Identity of the Register Object Implementer SPI implemented by any service.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiRegisterObjectImplementer_2Id = {0, "MafOamSpiRegisterObjectImplementer", "2"};
#endif
#ifndef __cplusplus
#define MafOamSpiRegisterObjectImplementer_1Id (MafMgmtSpiInterface_1T){NULL, "MafOamSpiRegisterObjectImplementer", "2"}
#endif

#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiRegisterObjectImplementer_3Id = {0, "MafOamSpiRegisterObjectImplementer", "3"};
#endif
#ifndef __cplusplus
#define MafOamSpiRegisterObjectImplementer_3Id (MafMgmtSpiInterface_1T){NULL, "MafOamSpiRegisterObjectImplementer", "3"}
#endif
/**
 * Identity of the Managed Object SPI implemented by any
 * service.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiManagedObject_1Id = {0, "MafOamSpiManagedObject", "1"};
#endif
#ifndef __cplusplus
#define MafOamSpiManagedObject_1Id (MafMgmtSpiInterface_1T){NULL, "MafOamSpiManagedObject", "1"};
#endif

#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiManagedObject_2Id = {0, "MafOamSpiManagedObject", "2"};
#endif
#ifndef __cplusplus
#define MafOamSpiManagedObject_2Id (MafMgmtSpiInterface_1T){NULL, "MafOamSpiManagedObject", "2"};
#endif

#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiManagedObject_3Id = {0, "MafOamSpiManagedObject", "3"};
#endif
#ifndef __cplusplus
#define MafOamSpiManagedObject_3Id (MafMgmtSpiInterface_1T){NULL, "MafOamSpiManagedObject", "3"};
#endif

#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafOamSpiManagedObject_3_1Id = {0, "MafOamSpiManagedObject", "3_1"};
#endif
#ifndef __cplusplus
#define MafOamSpiManagedObject_3_1Id (MafMgmtSpiInterface_1T){NULL, "MafOamSpiManagedObject", "3_1"};
#endif

#endif
