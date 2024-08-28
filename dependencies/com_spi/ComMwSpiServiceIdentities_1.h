#ifndef ComMwSpiServicesId
#define ComMwSpiServicesId



/**
 * Identities definition for the middleware services.
 *
 * @file ComMwSpiServiceIdentities_1.h
 *
 * This file specifies the interface identities used when fetching
 * the interface handles from the ComMgmtSpiInterfacePortal.
 */
#include <ComMgmtSpiInterface_1.h>
#ifndef __cplusplus
#include <stddef.h>
#endif

/**
 * Identity of the Availability Controller (AC) SPI implemented by the MW Support Agent (SA).
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComMwSpiAvailabilityController_1Id = { 0, "ComMwSpiAvailabilityController", "1" };
#endif
#ifndef __cplusplus
#define ComMwSpiAvailabilityController_1Id (ComMgmtSpiInterface_1T){NULL, "ComMwSpiAvailabilityController", "1"}
#endif

/**
 * Identity of the Log SPI implemented by the MW SA.
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComMwSpiLog_1Id = { 0, "ComMwSpiLog", "1" };
#endif
#ifndef __cplusplus
#define ComMwSpiLog_1Id (ComMgmtSpiInterface_1T){NULL, "ComMwSpiLog", "1"}
#endif

/**
 * Identity of the Trace SPI implemented by the MW SA.
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComMwSpiTrace_1Id = { 0, "ComMwSpiTrace", "1" };
#endif
#ifndef __cplusplus
#define ComMwSpiTrace_1Id (ComMgmtSpiInterface_1T){NULL, "ComMwSpiTrace", "1"}
#endif

/**
 * Identity of the replicated list SPI implemented by the MW SA.
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComMwSpiReplicatedList_1Id = { 0, "ComMwSpiReplicatedList", "1" };
#endif
#ifndef __cplusplus
#define ComMwSpiReplicatedList_1Id (ComMgmtSpiInterface_1T){NULL, "ComMwSpiReplicatedList", "1"}
#endif

/**
 * Identity of the Access Management SPI
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComMwSpiAccessManagement_1Id = {0, "ComMwSpiAccessManagement", "1"};
#endif
#ifndef __cplusplus
#define ComMwSpiAccessManagement_1Id (ComMgmtSpiInterface_1T){0, "ComMwSpiAccessManagement", "1"}
#endif

#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComMwSpiAccessManagement_2Id = {0, "ComMwSpiAccessManagement", "2"};
#endif
#ifndef __cplusplus
#define ComMwSpiAccessManagement_2Id (ComMgmtSpiInterface_1T){0, "ComMwSpiAccessManagement", "2"}
#endif


/**
 * Identity of the Crypto SPI.
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComMwSpiCrypto_1Id = { 0, "ComMwSpiCrypto", "1" };
#endif
#ifndef __cplusplus
#define ComMwSpiCrypto_1Id (ComMgmtSpiInterface_1T){NULL, "ComMwSpiCrypto", "1"}
#endif

#endif
