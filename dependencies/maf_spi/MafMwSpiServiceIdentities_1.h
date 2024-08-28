#ifndef MafMwSpiServicesId
#define MafMwSpiServicesId

/**
 * Identities definition for the middleware services.
 *
 * @file MafMwSpiServiceIdentities_1.h
 * @ingroup MafMwSpi
 *
 * This file specifies the interface identities used when fetching
 * the interface handles from the MafMgmtSpiInterfacePortal.
 */
#include <MafMgmtSpiInterface_1.h>

/**
 * Identity of the Availability Controller (AC) SPI implemented by the MW Support Agent (SA).
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafMwSpiAvailabilityController_1Id = { 0, "MafMwSpiAvailabilityController", "1" };
#endif
#ifndef __cplusplus
#define MafMwSpiAvailabilityController_1Id (MafMgmtSpiInterface_1T){NULL, "MafMwSpiAvailabilityController", "1"}
#endif

/**
 * Identity of the Log SPI implemented by the MW SA.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafMwSpiLog_1Id = { 0, "MafMwSpiLog", "1" };
#endif
#ifndef __cplusplus
#define MafMwSpiLog_1Id (MafMgmtSpiInterface_1T){NULL, "MafMwSpiLog", "1"}
#endif

/**
 * Identity of the Trace SPI implemented by the MW SA.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafMwSpiTrace_1Id = { 0, "MafMwSpiTrace", "1" };
#endif
#ifndef __cplusplus
#define MafMwSpiTrace_1Id (MafMgmtSpiInterface_1T){NULL, "MafMwSpiTrace", "1"}
#endif

#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafMwSpiTrace_2Id = { 0, "MafMwSpiTrace", "2" };
#endif
#ifndef __cplusplus
#define MafMwSpiTrace_2Id (MafMgmtSpiInterface_1T){NULL, "MafMwSpiTrace", "2"}
#endif

/**
 * Identity of the replicated list SPI implemented by the MW SA.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafMwSpiReplicatedList_1Id = { 0, "MafMwSpiReplicatedList", "1" };
#endif
#ifndef __cplusplus
#define MafMwSpiReplicatedList_1Id (MafMgmtSpiInterface_1T){NULL, "MafMwSpiReplicatedList", "1"}
#endif

/**
 * Identity of the Access Management SPI
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafMwSpiAccessManagement_1Id = {0, "MafMwSpiAccessManagement", "1"};
#endif
#ifndef __cplusplus
#define MafMwSpiAccessManagement_1Id (MafMgmtSpiInterface_1T){0, "MafMwSpiAccessManagement", "1"}
#endif

#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafMwSpiAccessManagement_2Id = {0, "MafMwSpiAccessManagement", "2"};
#endif
#ifndef __cplusplus
#define MafMwSpiAccessManagement_2Id (MafMgmtSpiInterface_1T){0, "MafMwSpiAccessManagement", "2"}
#endif
/**
 * Identity of the Crypto SPI.
 */
#ifdef __cplusplus
const MafMgmtSpiInterface_1T MafMwSpiCrypto_1Id = { 0, "MafMwSpiCrypto", "1" };
#endif
#ifndef __cplusplus
#define MafMwSpiCrypto_1Id (MafMgmtSpiInterface_1T){NULL, "MafMwSpiCrypto", "1"}
#endif

#endif
