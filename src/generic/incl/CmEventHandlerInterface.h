#ifndef CM_EVENT_HANDLER_INTERFACE_H
#define CM_EVENT_HANDLER_INTERFACE_H

/**
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
 *   File:   CmEventHandlerInterface.h
 *
 *   Author: eaparob
 *
 *   Date:   2013-02-05
 *
 *   This file declares a C-style function for use outside CmEventHandler object
 *   structure.
 *
 */

#include "MafMgmtSpiInterfacePortal_3.h"
#include "saAis.h"
#include "MafOamSpiCmEvent_1.h"
#include "saImmOi.h"

#ifdef  __cplusplus
extern "C" {
#endif

extern SaUint64T maxSAMemoryForCMEvents;

/*
 *  Creates the CmEventHandler object and starts it
 */
MafReturnT start_CmEventHandler(MafMgmtSpiInterfacePortal_3T *portal_MAF, SaAisErrorT (*subscriberFunction)(void), SaAisErrorT (*unsubscriberFunction)(void));
MafReturnT push_CmEventHandler(MafOamSpiCmEvent_Notification_1T *mafCmNot);
MafReturnT stop_CmEventHandler(void);

/*
 * Functions to CmEventHandler object to access _discardedCcbList
 */
bool isDiscardedCcb(SaImmOiCcbIdT ccbId);
void removeDiscardedCcb(SaImmOiCcbIdT ccbId);

/*
 * Functions to CM Notification Cache
 */
MafReturnT addToCmCache(SaImmOiCcbIdT ccbId, const char *attrName, const char *immRdn, const char *memberName, MafMoAttributeValueContainer_3T *memberValue);
MafReturnT setDnInCmCache(SaImmOiCcbIdT ccbId, MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator, const char *dn, MafOamSpiCmEvent_EventType_1T eventType, bool ccbLast);

/*
 * General utility functions which are used commonly by ComSA NTF and CM Event Handler
 */
uint64_t getTime(void);

/*
 * General utility functions which are used commonly by ComSA NTF and CM Event Producer
 */
void freeAVC(MafMoAttributeValueContainer_3T *avc);

#ifdef  __cplusplus
}
#endif
#endif
