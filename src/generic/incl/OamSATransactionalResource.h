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
 *   File:   OamSATransactionalResource.h
 * 
 *   Author: efaiami & egorped
 *
 *   Date:   2010-05-21
 * 
 *   This file declares the need functions for Transaction services.
 * 
 *   Reviewed: efaiami 2010-06-15
 * 
 *   Modify: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *   Modify: xjonbuc 2014-07-07  Implement MR-20275 support for explicit ccb validate() & abort()
 *   Modify: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 */

#ifndef _OAMSA_TRANSACTIONALRESOURCE_H
#define _OAMSA_TRANSACTIONALRESOURCE_H
#include <MafOamSpiTransaction_2.h>
#include <MafOamSpiTransactionalResource_2.h>
#ifdef  __cplusplus
extern "C" {
#endif


MafOamSpiTransactionalResource_2T* ExportOamSATransactionalResourceInterface_V2(void);


	/**
     *  Sets up things needed for the service and calls
     *  saImmOmInitialize, saImmOmSelectionObjectGet 
     *  to enable use of the SA Forum log service
     *  and openlog to enable use of the Linux syslog.
     * 
     *  @return MafOk or MafFailure.
     */
MafReturnT 		OamSATransactionalResourceOpen(void);
	
	/**
	 *  Cleans things up. Calls saImmOmFinalize to end use of the SA Forum log service
	 *  and Linux syslog.
     * 
     *  @return MafOk or MafFailure.
     */
MafReturnT 		OamSATransactionalResourceClose(void);

#ifdef  __cplusplus
}
#endif
#endif
