/*
 * SaInternalTrace.h
 *
 *  Created on: Jan 9, 2019
 *      Author: zyxxroj
 */

#ifndef SRC_GENERIC_TRACE_COMSATRACE_H_
#define SRC_GENERIC_TRACE_COMSATRACE_H_

/**
 * Log and error;
 */
#include <syslog.h>
#include "MafMwSpiLog_1.h"
#include "SA_Defines.h"
#include <saLog.h>

#ifdef __cplusplus
extern "C" {
#endif

MafReturnT initCompSaCfgLogStream();
void finalizeCompSaCfgLogStream();

void coremw_log(int priority, char const* fmt, ...)
   __attribute__ ((format(printf,2,3)));
void coremw_debug_log(int priority, char const* fmt, ...)
   __attribute__ ((format(printf,2,3)));

#ifdef __cplusplus
}
#endif

#endif /* SRC_GENERIC_TRACE_COMSATRACE_H_ */
