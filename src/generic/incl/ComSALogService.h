#ifndef COMSALOGSERVICE_H_
#define COMSALOGSERVICE_H_
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
 *   File:   ComSALogService.h
 *
 *   Author: egorped & efaiami
 *
 *   Date:   2010-05-21
 *
 *   This file declares the need functions for COM_SA Log Services.
 *
 *   Reviewed: efaiami 2010-07-08
 *
 *   Modify:   xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *   Modify:   xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 */

#include "MafMwSpiLog_1.h"

#ifdef  __cplusplus
extern "C" {
#endif
/**
 * Returns a pointer to a struct describing the interface and containing the
 * function pointer to use in calling the services
 *
 * @return pointer to a struct of type MafMwSpiLog_T
 */

//TBD re-include when fix the header conflict
//MafMwSpiLog_1T* maf_ExportLogServiceInterface(void);
MafMwSpiLog_1T* maf_ExportLogServiceInterface(void);

/**
 * Sets up things needed for the service and calls
 * saLogInitialize, saLogSelectionObjectGet and saLogStreamOpen_2
 * to enable use of the SA Forum log service
 * and openlog to enable use of the Linux syslogd
 *
 * @return MafOk or MafFailure.
 */

MafReturnT maf_ComLogServiceOpen(void);


/**
 * Cleans things up. Calls saLogFinalize to end use of the SA Forum Llog service
 * and closelog to detach from the Linux syslogd
 *
 * @return MafOk or MafFailure.
 */
MafReturnT ComLogServiceClose(void);

MafReturnT AlarmAndAlertLogServiceOpen(void);

MafReturnT AlarmAndAlertLogServiceClose(void);

MafReturnT cmdLogServiceClose(void);

MafReturnT cmdLogServiceOpen(void);

MafReturnT secLogServiceClose(void);

MafReturnT secLogServiceOpen(void);

/*
 * typedefs
 */

typedef struct {
	MwSpiSeverityT	MwSeverity;
	int SysLogLevel;
} SeverityTrTableEnt;


#ifdef  __cplusplus
}
#endif
#endif /*COMSALOGSERVICE_H_*/
