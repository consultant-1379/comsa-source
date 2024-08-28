#ifndef COMSANTF_H
#define COMSANTF_H
/*******************************************************************************
* Copyright (C) 2010 by Ericsson AB
* S - 125 26  STOCKHOLM
* SWEDEN, tel int + 46 10 719 0000
*
* The copyright to the computer program herein is the property of
* Ericsson AB. The program may be used and/or copied only with the
* written permission from Ericsson AB, or in accordance with the terms
* and conditions stipulated in the agreement/contract under which the
* program has been supplied..
*
* All rights reserved.
*
* Author: uaberin
*
* Date:   2010-06-17
*
* This file declares the need functions for forwarding of notifications
* of type alarm and security alarm from a SAF notification service. The
* notifications are converted to a suitable format for COM.
*
*
* Reviewed: efaiami 2010-07-08
*
* Modify: eaparob 2012-05-24 update ComNtfServiceOpen call for SDP1120 - temporary change to provide the MAF portal to ComNtfServiceOpen to be able to use MAF service for SDP1120
*
* Modified:   xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
* Modified:   xadaleg 2014-01-03  MR-29443 - align to ECIM FM 4.0
* Modified:   xadaleg 2015-08-02  MR-42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12*
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <poll.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>

// CoreMW
#include "saNtf.h"
#include "saImmOm.h"
// COM
#include "MafOamSpiNotificationFm_3.h"
#include "MafOamSpiNotificationFm_4.h"
#include "MafOamSpiEvent_1.h"
#include "MafMgmtSpiCommon.h"
#include "MafMgmtSpiInterfacePortal_3.h"
#include "MafOamSpiServiceIdentities_1.h"
#include "MafOamSpiCmEvent_1.h"

// COMSA
#include "ComSA.h"
#include "SelectTimer.h"
#include "OamSATranslateImmName.h"
#include "trace.h"
#include "CmEventHandlerInterface.h"

#define OAM_SA_NTF_RETRY_SLEEP 100000
#define OAM_SA_RETRY_SLEEP 1000
#define OAM_SA_NTF_MAX_RETRIES 100
#define CM_NOTIFICATIONS_SUBS_ID 45
#define NANOS_PER_SEC 1000000000L
#define OAM_SA_NTF_READER_MAX_RETRIES 5

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
    UuidDisableHandle = 0,
    UuidMapToAddInfoName = 1,
    UuidMapValueToAddText = 2
}UuidCfg;

// The Maf Filter structure used to notify COM
typedef struct {
	MafOamSpiEventConsumerHandleT consumerHandle;
	const char *eventType;
	MafNameValuePairT ** filter;
	SaNtfSubscriptionIdT subscriptionId;
} MafFilterT;

// The linked list of V4 notification structures
typedef struct valueList4 {
	MafOamSpiNotificationFmStruct_4T *comNot;
	MafOamSpiNotificationFmStruct_2T *comNot_2;
	struct valueList4 *next;
} valueList4T;

typedef enum MafOamSpiNotificationFmVersion {
	MafOamSpiNotificationFm_3 = 3,
	MafOamSpiNotificationFm_4
} MafOamSpiNtfFmVersionT;

static MafOamSpiNtfFmVersionT _mafSpiNtfFmVersion;
static MafStateChangeReasonT _reason;

MafReturnT maf_ComNtfServiceOpen(MafMgmtSpiInterfacePortal_3T* portal_MAF);
MafReturnT maf_ComNtfServiceClose(void);
void setMafStateChangeReason(MafStateChangeReasonT reason);

#ifdef  __cplusplus
}
#endif
#endif
