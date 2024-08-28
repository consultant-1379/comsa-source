/******************************************************************************
 *   Copyright (C) 2010 by Ericsson AB
 *   S - 125 26  STOCKHOLM
 *   SWEDEN, tel int + 46 10 719 0000
 *
 *   The copyright to the computer program herein is the property of
 *   Ericsson AB. The program may be used and/or copied only with the
 *   written permission from Ericsson AB, or in accordance with the terms
 *   and conditions stipulated in the agreement/contract under which the
 *   program has been supplied.
 *
 *   All rights reserved.
 *
 *
 *   File:   OamSAOIProxyStructCache.h
 *
 *   Author: emilzor
 *
 *   Date:   2012-02-28
 *
 *   This file implements handling arrays of complex type and
 *   sending data to COM
 *
 *   Reviewed: efaiami 2012-04-21
 *
 *   Modified: xnikvap 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *   Modified: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *   Modified: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 ******************************************************************************/

#ifndef OIPROXYSTRUCTCACHE_H_
#define OIPROXYSTRUCTCACHE_H_

#include "MafOamSpiManagedObject_2.h"
#include "MafOamSpiManagedObject_3.h"

#ifndef UNIT_TEST
#include "saImmOi.h"
#include "MafMgmtSpiCommon.h"
#else
//__extension__ typedef unsigned long long int	uint64_t;
#include <stdint.h> // A bit more "type-safe" than doing it wrong
//typedef uint64_t SaUint64T;
typedef SaUint64T SaImmOiCcbIdT;
#define ENTER()
#define LEAVE()
#endif

MafReturnT OIProxy_cache_existCacheData(SaImmOiCcbIdT ccbId);
MafMoAttributeValueContainer_3T *OIProxy_cache_getCacheData(SaImmOiCcbIdT ccbId);
MafOamSpiManagedObject_3T *OIProxy_cache_getCacheMOIf(SaImmOiCcbIdT ccbId);
MafReturnT OIProxy_cache_getAllCacheData(SaImmOiCcbIdT ccbId, char **the3gppDn, char **attrName, MafMoAttributeValueContainer_3T **container, MafOamSpiManagedObject_3T **moIf);
MafReturnT OIProxy_cache_sendCache(MafOamSpiManagedObject_3T *moIf, MafOamSpiTransactionHandleT txHandle, SaImmOiCcbIdT ccbId);

MafReturnT OIProxy_cache_setMoAttribute(MafOamSpiManagedObject_3T *moIf, MafOamSpiTransactionHandleT txHandle, SaImmOiCcbIdT ccbId, const char *the3gpp, const char *attrName, MafOamSpiMoAttributeType_3T attrType, MafMoAttributeValueContainer_3T *attrValue, bool isMoSpiVersion2 = false);
MafReturnT OIProxy_cache_deleteMo(MafOamSpiManagedObject_3T *moIf, MafOamSpiTransactionHandleT txHandle, SaImmOiCcbIdT ccbId, const char *the3gpp, bool isMoSpiVersion2 = false);
MafReturnT OIProxy_cache_createMo(MafOamSpiManagedObject_3T *moIf, MafOamSpiTransactionHandleT txHandle, SaImmOiCcbIdT ccbId, const char *the3gpp, const char *className, const char *keyAttrName, const char *keyAttrValue, bool isMoSpiVersion2 = false);
MafReturnT OIProxy_cache_ccbApply(MafOamSpiManagedObject_3T *moIf, MafOamSpiTransactionHandleT txHandle, SaImmOiCcbIdT ccbId);
MafReturnT OIProxy_cache_ccbAbort(SaImmOiCcbIdT ccbId);
MafReturnT OIProxy_cache_ccbFinalize(SaImmOiCcbIdT ccbId);

#endif /* OIPROXYSTRUCTCACHE_H_ */
