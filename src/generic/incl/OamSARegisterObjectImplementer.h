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
 *   File:   OamSARegisterObjectImplementer.h
 *
 *   Author: eozasaf
 *
 *   Date:   2010-09-01
 *
 *   This file declares the needed functions, structurs and variables for the OamSARegisterObjectImplementer interface
 *
 *   Modified: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *   Modified: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *   Modify: xthabui 2015-08-05 MR36067-Improved OI/SPI
 *
 */

#ifndef _OAMSA_REGISTEROBJECTIMPLEMENTER_H
#define _OAMSA_REGISTEROBJECTIMPLEMENTER_H
#include "MafOamSpiRegisterObjectImplementer_2.h"
#include "MafOamSpiRegisterObjectImplementer_3.h"


#ifdef  __cplusplus
	extern "C" {
#endif

	/**
     *  Returns a pointer to a struct describing the interface and containing the
     *  function pointer to use in calling the services
     *
     *  @return pointer to a struct of type MafOamSpiRegisterObjectImplementer_2T
     */
	MafOamSpiRegisterObjectImplementer_2T* ExportOamSARegisterObjectImplementerInterface_2(void);

	/**
	 *  Returns a pointer to a struct describing the interface and containing the
	 *  function pointer to use in calling the services
	 *
	 *  @return pointer to a struct of type MafOamSpiRegisterObjectImplementer_3T
	 */
	MafOamSpiRegisterObjectImplementer_3T* ExportOamSARegisterObjectImplementerInterface(void);

#ifdef  __cplusplus
	}
#endif

#endif
