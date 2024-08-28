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
 *   File:   OamSAOIProxy.cc
 *
 *   Author: eozasaf
 *
 *   Date:   2011-09-19
 *
 *   This file defines the functions and external variables of the
 *   Object Implementer (OI) inside ComSA
 *
 *   Modify: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *   Modify: uabjoy  2012-09-18 Correct HQ23654
 *   Modify: xduncao 2013-01-22 SDP875 Changed GetErrMsgsFromThContext()
 *   Modify: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 *****************************************************************************/

#ifndef OAMSAOIPROXY_H_
#define OAMSAOIPROXY_H_

#include "saAis.h"
#include "saImmOm.h"
#include "saImmOi.h"

#include "MafMgmtSpiCommon.h"

//explicit declaration
extern SaImmOiHandleT immOiHandle;
extern SaSelectionObjectT immSelectionObject;
extern SaImmHandleT immOmHandle;
// Handle to the object access API
extern SaImmAccessorHandleT accessorHandleOI;

#ifdef __cplusplus
// This constant defines the time in seconds that a transaction towards
// a COM Object Implementer is alive.
extern const unsigned int TransactionTimeoutValue;	// default value is 3600;
extern "C" {
#endif

MafReturnT ObjImp_init_imm(bool loadImmOm);
void ObjImp_finalize_imm(bool unLoadImmOm, bool cancelDispatchThread);
MafReturnT GetSet_ErrorMessages(SaImmOiCcbIdT ccbId);
MafReturnT GetErrMsgsFromThContext(const char*** messages, ThreadContextHandle_2T *msgHandle);
void PrepareErrMsgsForAdmOpResult(char** messages, SaImmAdminOperationParamsT_2** returnParams);

#ifdef __cplusplus
}
#endif

#endif /* OAMSAOIPROXY_H_ */
