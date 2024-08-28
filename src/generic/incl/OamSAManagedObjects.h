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
 *   File:   OamSAManagedObjects.h
 * 
 *   Author: egorped
 *
 *   Date:   2010-05-21
 * 
 *   This file declares the need functions for Managed Objects.
 * 
 *   Reviewed: efaiami 2010-06-15
 *
 *   Modified: xnikvap 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *   Modified: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 * 
 */
 
#ifndef _OAMSA_MANAGEDOBJECT_H
#define _OAMSA_MANAGEDOBJECT_H
#include "MafOamSpiManagedObject_3.h"
#ifdef  __cplusplus
extern "C" {
#endif

	/**
     *  Returns a pointer to a struct describing the interface and containing the 
     *  function pointer to use in calling the services
     *
     *  @return pointer to a struct of type MafMgmtSpiInterface_3T
     */
MafOamSpiManagedObject_3T* ExportOamManagedObjectInterface(void);

	/**
     *  Sets up things needed for the service and calls
     *  saImmOmInitialize, saImmOmSelectionObjectGet 
     *  to enable use of the SA Forum log service
     *  and openlog to enable use of the Linux syslog
     * 
     *  @return MafOk or MafFailure.
     */
MafReturnT 		OamManagedObjectOpen(void);

	/**
	 *  Cleans things up. Calls saImmOmFinalize to end use of the SA Forum log service
	 *  and Linux syslog. 
     * 
     *  @return MafOk or MafFailure.
     */
MafReturnT 		OamManagedObjectClose(void);
#ifdef  __cplusplus
}
#endif
#endif
