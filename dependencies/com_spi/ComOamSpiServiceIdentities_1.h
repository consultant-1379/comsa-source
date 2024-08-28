#ifndef ComOamSpiServicesId
#define ComOamSpiServicesId

/**
 * Identities definition for the operation and maintenance services.
 *
 * @file ComOamSpiServiceIdentities_1.h
 *
 * This file specifies the interface identities used when fetching
 * the interface from the ComMgmtSpiInterfacePortal.
 *
 * Considering usability reasons, the version of this file might not be
 * incremented as long as the change introduced remains backwards compatible.
 */
#include <ComMgmtSpiInterface_1.h>
#include <MafMgmtSpiInterface_1.h>
#ifndef __cplusplus
#include <stddef.h>
#endif

/**
 * Identity of the Managed Object SPI implemented by the Ext CM Router service.
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiExtCmRouterService_1Id = {"ComOamSpiExtCmRouterService", "ComOamSpiManagedObject", "1"};
const ComMgmtSpiInterface_1T ComOamSpiExtCmRouterService_3Id = {"ComOamSpiExtCmRouterService", "ComOamSpiManagedObject", "3"};
const MafMgmtSpiInterface_1T ComOamSpiExtCmRouterService_3_1Id = {"ComOamSpiExtCmRouterService", "MafOamSpiManagedObject", "3_1"};
#endif
#ifndef __cplusplus
#define ComOamSpiExtCmRouterService_1Id (ComMgmtSpiInterface_1T){"ComOamSpiExtCmRouterService", "ComOamSpiManagedObject", "1"}
#define ComOamSpiExtCmRouterService_3Id (ComMgmtSpiInterface_1T){"ComOamSpiExtCmRouterService", "ComOamSpiManagedObject", "3"}
#define ComOamSpiExtCmRouterService_3_1Id (MafMgmtSpiInterface_1T){"ComOamSpiExtCmRouterService", "MafOamSpiManagedObject", "3_1"}
#endif

/**
 * Identity of the Managed Object SPI implemented by the CM Router service.
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiCmRouterService_1Id = {"ComOamSpiCmRouterService", "ComOamSpiManagedObject", "1"};
const ComMgmtSpiInterface_1T ComOamSpiCmRouterService_3Id = {"ComOamSpiCmRouterService", "ComOamSpiManagedObject", "3"};
//const MafMgmtSpiInterface_1T ComOamSpiCmRouterService_3_1Id = {"ComOamSpiCmRouterService", "MafOamSpiManagedObject", "3_1"};
#endif
#ifndef __cplusplus
#define ComOamSpiCmRouterService_1Id (ComMgmtSpiInterface_1T){"ComOamSpiCmRouterService", "ComOamSpiManagedObject", "1"}
#define ComOamSpiCmRouterService_3Id (ComMgmtSpiInterface_1T){"ComOamSpiCmRouterService", "ComOamSpiManagedObject", "3"}
//#define ComOamSpiCmRouterService_3_1Id (MafMgmtSpiInterface_1T){"ComOamSpiCmRouterService", "MafOamSpiManagedObject", "3_1"}
#endif

/**
 * Identity of the Managed Object SPI implemented by any service.
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiManagedObject_1Id = {0, "ComOamSpiManagedObject", "1"};
const ComMgmtSpiInterface_1T ComOamSpiManagedObject_3Id = {0, "ComOamSpiManagedObject", "3"};
const MafMgmtSpiInterface_1T ComOamSpiManagedObject_3_1Id = {0, "MafOamSpiManagedObject", "3_1"};
#endif
#ifndef __cplusplus
#define ComOamSpiManagedObject_1Id (ComMgmtSpiInterface_1T){NULL, "ComOamSpiManagedObject", "1"}
#define ComOamSpiManagedObject_3Id (ComMgmtSpiInterface_1T){NULL, "ComOamSpiManagedObject", "3"}
#define ComOamSpiManagedObject_3_1Id (MafMgmtSpiInterface_1T){NULL, "MafOamSpiManagedObject", "3_1"}
#endif


/**
 * Identity of the Model Repository SPI implemented by the Model Repository service.
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiModelRepository_1Id = {"ComOamSpiModelRepositoryService", "ComOamSpiModelRepository", "1"};
#endif
#ifndef __cplusplus
#define ComOamSpiModelRepository_1Id (ComMgmtSpiInterface_1T){"ComOamSpiModelRepositoryService", "ComOamSpiModelRepository", "1"}
#endif

/**
 * Identity of the Model Repository SPI II implemented by the Model Repository service.
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiModelRepository_2Id = {"ComOamSpiModelRepositoryService", "ComOamSpiModelRepository", "2"};
#endif
#ifndef __cplusplus
#define ComOamSpiModelRepository_2Id (ComMgmtSpiInterface_1T){"ComOamSpiModelRepositoryService", "ComOamSpiModelRepository", "2"}
#endif

/**
 * Identity of the Transaction SPI implemented by the Transaction service.
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiTransaction_1Id = {"ComOamSpiTransactionService", "ComOamSpiTransaction", "1"};
#endif
#ifndef __cplusplus
#define ComOamSpiTransaction_1Id (ComMgmtSpiInterface_1T){"ComOamSpiTransactionService", "ComOamSpiTransaction", "1"}
#endif
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiTransaction_2Id = {"ComOamSpiTransactionService", "ComOamSpiTransaction", "2"};
#endif
#ifndef __cplusplus
#define ComOamSpiTransaction_2Id (ComMgmtSpiInterface_1T){"ComOamSpiTransactionService", "ComOamSpiTransaction", "2"}
#endif

/**
 * Identity of the Transaction Master SPI implemented by the Transaction service.
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiTransactionMaster_1Id = {"ComOamSpiTransactionService", "ComOamSpiTransactionMaster", "1"};
#endif
#ifndef __cplusplus
#define ComOamSpiTransactionMaster_1Id (ComMgmtSpiInterface_1T){"ComOamSpiTransactionService", "ComOamSpiTransactionMaster", "1"}
#endif
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiTransactionMaster_2Id = {"ComOamSpiTransactionService", "ComOamSpiTransactionMaster", "2"};
#endif
#ifndef __cplusplus
#define ComOamSpiTransactionMaster_2Id (ComMgmtSpiInterface_1T){"ComOamSpiTransactionService", "ComOamSpiTransactionMaster", "2"}
#endif


/**
 * Identity of the Transaction Resource SPI implemented by any service.
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiTransactionalResource_1Id = {0, "ComOamSpiTransactionalResource", "1"};
#endif
#ifndef __cplusplus
#define ComOamSpiTransactionalResource_1Id (ComMgmtSpiInterface_1T){NULL, "ComOamSpiTransactionalResource", "1"}
#endif
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiTransactionalResource_2Id = {0, "ComOamSpiTransactionalResource", "2"};
#endif
#ifndef __cplusplus
#define ComOamSpiTransactionalResource_2Id (ComMgmtSpiInterface_1T){NULL, "ComOamSpiTransactionalResource", "2"}
#endif

/**
 * Identity of the Notification Producer SPI implemented by any service.
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiNotificationProducer_1Id = { 0, "ComOamSpiNotificationProducer", "1" };
#endif
#ifndef __cplusplus
#define ComOamSpiNotificationProducer_1Id (ComMgmtSpiInterface_1T){NULL, "ComOamSpiNotificationProducer", "1"}
#endif


/**
 * Identity of the Event service SPI
 */

#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiEventService_1Id = {"ComOamSpiEventService", "ComOamSpiEventRouter", "1"};
#endif
#ifndef __cplusplus
#define ComOamSpiEventService_1Id (ComMgmtSpiInterface_1T){"ComOamSpiEventService", "ComOamSpiEventRouter", "1"}
#endif

/**
 * Identity of the Security Management SPI
 */

#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiSecurityManagement_1Id = {"ComOamSpiSecurityManagementService", "ComOamSpiSecurityManagement", "1"};
#endif
#ifndef __cplusplus
#define ComOamSpiSecurityManagement_1Id (ComMgmtSpiInterface_1T){"ComOamSpiSecurityManagementService", "ComOamSpiSecurityManagement", "1"}
#endif


/**
 * Identity of the Register Object Implementer SPI implemented by any service.
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiRegisterObjectImplementer_2Id = {0, "ComOamSpiRegisterObjectImplementer", "2"};
const ComMgmtSpiInterface_1T ComOamSpiRegisterObjectImplementer_3Id = {0, "ComOamSpiRegisterObjectImplementer", "3"};
#endif
#ifndef __cplusplus
#define ComOamSpiRegisterObjectImplementer_2Id (ComMgmtSpiInterface_1T){NULL, "ComOamSpiRegisterObjectImplementer", "2"}
#define ComOamSpiRegisterObjectImplementer_3Id (ComMgmtSpiInterface_1T){NULL, "ComOamSpiRegisterObjectImplementer", "3"}
#endif

/**
 * Identity of the Visibility SPI.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T ComOamApiVisibility_1Id = {"VisibilityControllerComponent", "ComOamApiVisibility", "1" };
#endif
#ifndef __cplusplus
#define ComOamApiVisibility_1Id (MafMgmtSpiInterface_1T){"VisibilityControllerComponent", "ComOamApiVisibility", "1"}
#endif


/**
 * Identity of the Performance Management Measurement SPI implemented by any service.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T ComOamSpiPmMeasurementsInterface_1Id = { 0, "ComOamSpiPmMeasurements", "1"};
const MafMgmtSpiInterface_1T ComOamSpiPmMeasurementsInterface_2Id = { 0, "ComOamSpiPmMeasurements", "2"};
const MafMgmtSpiInterface_1T ComOamSpiPmMeasurementsInterface_3Id = { 0, "ComOamSpiPmMeasurements", "3"};
const MafMgmtSpiInterface_1T ComOamSpiPmMeasurementsInterface_4Id = { 0, "ComOamSpiPmMeasurements", "4"};
#endif
#ifndef __cplusplus
#define ComOamSpiPmMeasurementsInterface_1Id (MafMgmtSpiInterface_1T){ NULL, "ComOamSpiPmMeasurements", "1"}
#define ComOamSpiPmMeasurementsInterface_2Id (MafMgmtSpiInterface_1T){ NULL, "ComOamSpiPmMeasurements", "2"}
#define ComOamSpiPmMeasurementsInterface_3Id (MafMgmtSpiInterface_1T){ NULL, "ComOamSpiPmMeasurements", "3"}
#define ComOamSpiPmMeasurementsInterface_4Id (MafMgmtSpiInterface_1T){ NULL, "ComOamSpiPmMeasurements", "4"}
#endif





#endif
