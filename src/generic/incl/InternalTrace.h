/********************************************************************************************
 *   Copyright (C) 2014 by Ericsson AB
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
 *   File:   InternalTrace.h
 *
 *   Author: eerwest
 *
 *   Date:   2014-03-20
 *
 *   This file handles the internal trace macros that COM SA uses.
 *
 *   Reviewed:
 *   Modify:
 *
 *****************************************************************************************/

#ifndef __INTERNAL_TRACE_H_
#define __INTERNAL_TRACE_H_
#include "com_ericsson_common_comsa.h"
#include "com_ericsson_common_comsa_imm.h"
#include "com_ericsson_common_comsa_imm_oi.h"
#include "com_ericsson_common_comsa_imm_om.h"
#include "com_ericsson_common_comsa_mwsa.h"
#include "com_ericsson_common_comsa_mwsa_ac.h"
#include "com_ericsson_common_comsa_mwsa_general.h"
#include "com_ericsson_common_comsa_mwsa_log.h"
#include "com_ericsson_common_comsa_mwsa_replist.h"
#include "com_ericsson_common_comsa_mwsa_trace.h"
#include "com_ericsson_common_comsa_oamsa.h"
#include "com_ericsson_common_comsa_oamsa_cmevent.h"
#include "com_ericsson_common_comsa_oamsa_alarm.h"
#include "com_ericsson_common_comsa_oamsa_translations.h"
#include "com_ericsson_common_comsa_oiproxy.h"
#include "com_ericsson_common_comsa_pmtsa_event.h"
#include "com_ericsson_common_comsa_undefined.h"

#include "ComSA.h"
#include "SaInternalTrace.h"

/* A temporary GCC 4.8.2 compiler issue only on DEK server is causing coredump in
   UnittestOIproxy if the value is 1024 or bigger */
#ifdef UNIT_TEST
#define TRACE_BUFFER_SIZE 256
#else
#define TRACE_BUFFER_SIZE 4096
#endif

static inline char* createString(char * buffer, char const* fmt, ...)
{
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(buffer, TRACE_BUFFER_SIZE-1, fmt, ap);
		buffer[TRACE_BUFFER_SIZE-1] = 0;
		va_end(ap);
		return buffer;
}

/* IMM_OI TRACE DOMAIN */

#define ERR_IMM_OI(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_imm_oi, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN_IMM_OI(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_imm_oi, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG_IMM_OI(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_imm_oi, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define ENTER_IMM_OI() do { \
	tracepoint(com_ericsson_common_comsa_imm_oi, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)

#define LEAVE_IMM_OI() do { \
	tracepoint(com_ericsson_common_comsa_imm_oi, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
	if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
	    enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
	} \
} while(0)

#define DEBUG_IMM_OI(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_imm_oi, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)

/* IMM_OM TRACE DOMAIN */

#define ERR_IMM_OM(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_imm_om, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN_IMM_OM(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_imm_om, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG_IMM_OM(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_imm_om, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define ENTER_IMM_OM() do { \
	tracepoint(com_ericsson_common_comsa_imm_om, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)

#define LEAVE_IMM_OM() do { \
	tracepoint(com_ericsson_common_comsa_imm_om, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
	if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
	    enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
	} \
} while(0)

#define DEBUG_IMM_OM(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_imm_om, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)

/* IMM TRACE DOMAIN */

#define ERR_IMM(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_imm, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN_IMM(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_imm, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG_IMM(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_imm, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define ENTER_IMM() do { \
	tracepoint(com_ericsson_common_comsa_imm, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)

#define LEAVE_IMM() do { \
	tracepoint(com_ericsson_common_comsa_imm, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
	if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
	    enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
	} \
} while(0)

#define DEBUG_IMM(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_imm, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)

/* MWSA_GENERAL TRACE DOMAIN */

#define ERR_MWSA_GENERAL(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_general, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN_MWSA_GENERAL(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_general, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG_MWSA_GENERAL(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_general, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define ENTER_MWSA_GENERAL() do { \
	tracepoint(com_ericsson_common_comsa_mwsa_general, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)

#define LEAVE_MWSA_GENERAL() do { \
	tracepoint(com_ericsson_common_comsa_mwsa_general, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
	if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
	    enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
	} \
} while(0)

#define DEBUG_MWSA_GENERAL(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_general, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)

/* MWSA_LOG TRACE DOMAIN */

#define ERR_MWSA_LOG(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_log, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN_MWSA_LOG(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_log, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG_MWSA_LOG(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_log, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define ENTER_MWSA_LOG() do { \
	tracepoint(com_ericsson_common_comsa_mwsa_log, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)

#define LEAVE_MWSA_LOG() do { \
	tracepoint(com_ericsson_common_comsa_mwsa_log, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
	if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
	    enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
	} \
} while(0)

#define DEBUG_MWSA_LOG(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_log, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)

/* MWSA_AC TRACE DOMAIN */

#define ERR_MWSA_AC(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_ac, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN_MWSA_AC(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_ac, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG_MWSA_AC(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_ac, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define ENTER_MWSA_AC() do { \
	tracepoint(com_ericsson_common_comsa_mwsa_ac, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)

#define LEAVE_MWSA_AC() do { \
	tracepoint(com_ericsson_common_comsa_mwsa_ac, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
	if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
	    enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
	} \
} while(0)

#define DEBUG_MWSA_AC(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_ac, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)

/* MWSA_REPLIST TRACE DOMAIN */

#define ERR_MWSA_REPLIST(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_replist, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN_MWSA_REPLIST(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_replist, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG_MWSA_REPLIST(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_replist, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define ENTER_MWSA_REPLIST() do { \
	tracepoint(com_ericsson_common_comsa_mwsa_replist, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)

#define LEAVE_MWSA_REPLIST() do { \
	tracepoint(com_ericsson_common_comsa_mwsa_replist, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
	if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
	    enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
	} \
} while(0)

#define DEBUG_MWSA_REPLIST(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_replist, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)

/* MWSA_TRACE TRACE DOMAIN */

#define ERR_MWSA_TRACE(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_trace, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN_MWSA_TRACE(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_trace, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG_MWSA_TRACE(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_trace, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define ENTER_MWSA_TRACE() do { \
	tracepoint(com_ericsson_common_comsa_mwsa_trace, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)

#define LEAVE_MWSA_TRACE() do { \
	tracepoint(com_ericsson_common_comsa_mwsa_trace, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
	if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
	    enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
	} \
} while(0)

#define DEBUG_MWSA_TRACE(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa_trace, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)

/* MWSA TRACE DOMAIN */

#define ERR_MWSA(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN_MWSA(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG_MWSA(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define ENTER_MWSA() do { \
	tracepoint(com_ericsson_common_comsa_mwsa, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)

#define LEAVE_MWSA() do { \
	tracepoint(com_ericsson_common_comsa_mwsa, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
	if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
	    enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
	} \
} while(0)

#define DEBUG_MWSA(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_mwsa, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)

/* OAMSA_ALARM TRACE DOMAIN */

#define ERR_OAMSA_ALARM(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oamsa_alarm, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN_OAMSA_ALARM(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oamsa_alarm, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG_OAMSA_ALARM(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oamsa_alarm, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define ENTER_OAMSA_ALARM() do { \
	tracepoint(com_ericsson_common_comsa_oamsa_alarm, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)

#define LEAVE_OAMSA_ALARM() do { \
	tracepoint(com_ericsson_common_comsa_oamsa_alarm, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
	if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
	    enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
	} \
} while(0)

#define DEBUG_OAMSA_ALARM(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oamsa_alarm, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)

/* OAMSA_CMEVENT TRACE DOMAIN */

#define ERR_OAMSA_CMEVENT(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oamsa_cmevent, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN_OAMSA_CMEVENT(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oamsa_cmevent, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG_OAMSA_CMEVENT(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oamsa_cmevent, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define ENTER_OAMSA_CMEVENT() do { \
	tracepoint(com_ericsson_common_comsa_oamsa_cmevent, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)

#define LEAVE_OAMSA_CMEVENT() do { \
	tracepoint(com_ericsson_common_comsa_oamsa_cmevent, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
	if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
	    enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
	} \
} while(0)

#define DEBUG_OAMSA_CMEVENT(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oamsa_cmevent, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)

/* OAMSA_TRANSLATIONS TRACE DOMAIN */

#define ERR_OAMSA_TRANSLATIONS(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oamsa_translations, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN_OAMSA_TRANSLATIONS(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oamsa_translations, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG_OAMSA_TRANSLATIONS(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oamsa_translations, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define ENTER_OAMSA_TRANSLATIONS() do { \
	tracepoint(com_ericsson_common_comsa_oamsa_translations, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)

#define LEAVE_OAMSA_TRANSLATIONS() do { \
	tracepoint(com_ericsson_common_comsa_oamsa_translations, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
	if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
	    enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
	} \
} while(0)

#define DEBUG_OAMSA_TRANSLATIONS(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oamsa_translations, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)

/* OAMSA TRACE DOMAIN */

#define ERR_OAMSA(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oamsa, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN_OAMSA(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oamsa, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG_OAMSA(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oamsa, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define ENTER_OAMSA() do { \
	tracepoint(com_ericsson_common_comsa_oamsa, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)

#define LEAVE_OAMSA() do { \
	tracepoint(com_ericsson_common_comsa_oamsa, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
	if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
	    enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
	} \
} while(0)

#define DEBUG_OAMSA(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oamsa, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)

/* OIPROXY TRACE DOMAIN */

#define ERR_OIPROXY(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oiproxy, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN_OIPROXY(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oiproxy, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG_OIPROXY(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oiproxy, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define ENTER_OIPROXY() do { \
	tracepoint(com_ericsson_common_comsa_oiproxy, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)

#define LEAVE_OIPROXY() do { \
	tracepoint(com_ericsson_common_comsa_oiproxy, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
	if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
	    enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
	} \
} while(0)

#define DEBUG_OIPROXY(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_oiproxy, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)

/* COMSA TRACE DOMAIN */

#define ERR_COMSA(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN_COMSA(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG_COMSA(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define ENTER_COMSA() do { \
	tracepoint(com_ericsson_common_comsa, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)

#define LEAVE_COMSA() do { \
	tracepoint(com_ericsson_common_comsa, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
	if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
	    enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
	} \
} while(0)

#define DEBUG_COMSA(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)


/* PMTSA TRACE DOMAIN */

#define ERR_PMTSA_EVENT(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_pmtsa_event, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN_PMTSA_EVENT(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_pmtsa_event, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG_PMTSA_EVENT(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_pmtsa_event, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define ENTER_PMTSA_EVENT() do { \
	tracepoint(com_ericsson_common_comsa_pmtsa_event, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)

#define LEAVE_PMTSA_EVENT() do { \
	tracepoint(com_ericsson_common_comsa_pmtsa_event, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
	if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
	    enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
	} \
} while(0)

#define DEBUG_PMTSA_EVENT(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_pmtsa_event, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)

/* UNDEFINED TRACE DOMAIN, The old default logger functionality */

#define ERR(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_undefined, COMSA_ERROR, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_ERR, arg); \
} while(0)

#define WARN(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_undefined, COMSA_WARNING, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_WARNING, arg); \
} while(0)

#define LOG(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_undefined, COMSA_LOG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_log(LOG_NOTICE, arg); \
} while(0)

#define DEBUG(arg...) do { \
	char buffer[TRACE_BUFFER_SIZE]; \
	tracepoint(com_ericsson_common_comsa_undefined, COMSA_DEBUG, createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	coremw_debug_log(LOG_DEBUG, arg); \
} while(0)


#ifdef BUILD_WITH_TRACE_TESTS
#define TRACE_ALL_LEVELS_UNDEFINED() do { \
		ENTER();\
		ERR("COMSA TEST: ERR test string in %s", "undefined");\
		WARN("COMSA TEST: WARN test string in %s", "undefined");\
		LOG("COMSA TEST: LOG test string in %s", "undefined");\
		DEBUG("COMSA TEST: DEBUG string in %s", "undefined");\
		LEAVE(); \
} while(0)

#define TRACE_ALL_LEVELS_COMSA() do { \
		ENTER_COMSA();\
		ERR_COMSA("COMSA TEST: ERR test string in %s", "comsa");\
		WARN_COMSA("COMSA TEST: WARN test string in %s", "comsa");\
		LOG_COMSA("COMSA TEST: LOG test string in %s", "comsa");\
		DEBUG_COMSA("COMSA TEST: DEBUG string in %s", "comsa");\
		LEAVE_COMSA(); \
} while(0)

#define TRACE_ALL_LEVELS_MWSA() do { \
		ENTER_MWSA();\
		ERR_MWSA("COMSA TEST: ERR test string in %s", "mwsa");\
		WARN_MWSA("COMSA TEST: WARN test string in %s", "mwsa");\
		LOG_MWSA("COMSA TEST: LOG test string in %s", "mwsa");\
		DEBUG_MWSA("COMSA TEST: DEBUG string in %s", "mwsa");\
		LEAVE_MWSA(); \
} while(0)

#define TRACE_ALL_LEVELS_MWSA_AC() do { \
		ENTER_MWSA_AC();\
		ERR_MWSA_AC("COMSA TEST: ERR test string in %s", "mwsa_ac");\
		WARN_MWSA_AC("COMSA TEST: WARN test string in %s", "mwsa_ac");\
		LOG_MWSA_AC("COMSA TEST: LOG test string in %s", "mwsa_ac");\
		DEBUG_MWSA_AC("COMSA TEST: DEBUG string in %s", "mwsa_ac");\
		LEAVE_MWSA_AC(); \
} while(0)

#define TRACE_ALL_LEVELS_MWSA_GENERAL() do { \
		ENTER_MWSA_GENERAL();\
		ERR_MWSA_GENERAL("COMSA TEST: ERR test string in %s", "mwsa_general");\
		WARN_MWSA_GENERAL("COMSA TEST: WARN test string in %s", "mwsa_general");\
		LOG_MWSA_GENERAL("COMSA TEST: LOG test string in %s", "mwsa_general");\
		DEBUG_MWSA_GENERAL("COMSA TEST: DEBUG string in %s", "mwsa_general");\
		LEAVE_MWSA_GENERAL(); \
} while(0)

#define TRACE_ALL_LEVELS_MWSA_LOG() do { \
		ENTER_MWSA_LOG();\
		ERR_MWSA_LOG("COMSA TEST: ERR test string in %s", "mwsa_log");\
		WARN_MWSA_LOG("COMSA TEST: WARN test string in %s", "mwsa_log");\
		LOG_MWSA_LOG("COMSA TEST: LOG test string in %s", "mwsa_log");\
		DEBUG_MWSA_LOG("COMSA TEST: DEBUG string in %s", "mwsa_log");\
		LEAVE_MWSA_LOG(); \
} while(0)

#define TRACE_ALL_LEVELS_MWSA_REPLIST() do { \
		ENTER_MWSA_REPLIST();\
		ERR_MWSA_REPLIST("COMSA TEST: ERR test string in %s", "mwsa_replist");\
		WARN_MWSA_REPLIST("COMSA TEST: WARN test string in %s", "mwsa_replist");\
		LOG_MWSA_REPLIST("COMSA TEST: LOG test string in %s", "mwsa_replist");\
		DEBUG_MWSA_REPLIST("COMSA TEST: DEBUG string in %s", "mwsa_replist");\
		LEAVE_MWSA_REPLIST(); \
} while(0)

#define TRACE_ALL_LEVELS_MWSA_TRACE() do { \
		ENTER_MWSA_TRACE();\
		ERR_MWSA_TRACE("COMSA TEST: ERR test string in %s", "mwsa_trace");\
		WARN_MWSA_TRACE("COMSA TEST: WARN test string in %s", "mwsa_trace");\
		LOG_MWSA_TRACE("COMSA TEST: LOG test string in %s", "mwsa_trace");\
		DEBUG_MWSA_TRACE("COMSA TEST: DEBUG string in %s", "mwsa_trace");\
		LEAVE_MWSA_TRACE(); \
} while(0)

#define TRACE_ALL_LEVELS_IMM() do { \
		ENTER_IMM();\
		ERR_IMM("COMSA TEST: ERR test string in %s", "imm");\
		WARN_IMM("COMSA TEST: WARN test string in %s", "imm");\
		LOG_IMM("COMSA TEST: LOG test string in %s", "imm");\
		DEBUG_IMM("COMSA TEST: DEBUG string in %s", "imm");\
		LEAVE_IMM(); \
} while(0)

#define TRACE_ALL_LEVELS_IMM_OI() do { \
		ENTER_IMM_OI();\
		ERR_IMM_OI("COMSA TEST: ERR test string in %s", "imm_oi");\
		WARN_IMM_OI("COMSA TEST: WARN test string in %s", "imm_oi");\
		LOG_IMM_OI("COMSA TEST: LOG test string in %s", "imm_oi");\
		DEBUG_IMM_OI("COMSA TEST: DEBUG string in %s", "imm_oi");\
		LEAVE_IMM_OI(); \
} while(0)

#define TRACE_ALL_LEVELS_IMM_OM() do { \
		ENTER_IMM_OM();\
		ERR_IMM_OM("COMSA TEST: ERR test string in %s", "imm_om");\
		WARN_IMM_OM("COMSA TEST: WARN test string in %s", "imm_om");\
		LOG_IMM_OM("COMSA TEST: LOG test string in %s", "imm_om");\
		DEBUG_IMM_OM("COMSA TEST: DEBUG string in %s", "imm_om");\
		LEAVE_IMM_OM(); \
} while(0)

#define TRACE_ALL_LEVELS_OAMSA() do { \
		ENTER_OAMSA();\
		ERR_OAMSA("COMSA TEST: ERR test string in %s", "oamsa");\
		WARN_OAMSA("COMSA TEST: WARN test string in %s", "oamsa");\
		LOG_OAMSA("COMSA TEST: LOG test string in %s", "oamsa");\
		DEBUG_OAMSA("COMSA TEST: DEBUG string in %s", "oamsa");\
		LEAVE_OAMSA(); \
} while(0)

#define TRACE_ALL_LEVELS_OAMSA_ALARM() do { \
		ENTER_OAMSA_ALARM();\
		ERR_OAMSA_ALARM("COMSA TEST: ERR test string in %s", "oamsa_alarm");\
		WARN_OAMSA_ALARM("COMSA TEST: WARN test string in %s", "oamsa_alarm");\
		LOG_OAMSA_ALARM("COMSA TEST: LOG test string in %s", "oamsa_alarm");\
		DEBUG_OAMSA_ALARM("COMSA TEST: DEBUG string in %s", "oamsa_alarm");\
		LEAVE_OAMSA_ALARM(); \
} while(0)

#define TRACE_ALL_LEVELS_OAMSA_CMEVENT() do { \
		ENTER_OAMSA_CMEVENT();\
		ERR_OAMSA_CMEVENT("COMSA TEST: ERR test string in %s", "oamsa_cmevent");\
		WARN_OAMSA_CMEVENT("COMSA TEST: WARN test string in %s", "oamsa_cmevent");\
		LOG_OAMSA_CMEVENT("COMSA TEST: LOG test string in %s", "oamsa_cmevent");\
		DEBUG_OAMSA_CMEVENT("COMSA TEST: DEBUG string in %s", "oamsa_cmevent");\
		LEAVE_OAMSA_CMEVENT(); \
} while(0)

#define TRACE_ALL_LEVELS_OAMSA_TRANSLATIONS() do { \
		ENTER_OAMSA_TRANSLATIONS();\
		ERR_OAMSA_TRANSLATIONS("COMSA TEST: ERR test string in %s", "oamsa_translations");\
		WARN_OAMSA_TRANSLATIONS("COMSA TEST: WARN test string in %s", "oamsa_translations");\
		LOG_OAMSA_TRANSLATIONS("COMSA TEST: LOG test string in %s", "oamsa_translations");\
		DEBUG_OAMSA_TRANSLATIONS("COMSA TEST: DEBUG string in %s", "oamsa_translations");\
		LEAVE_OAMSA_TRANSLATIONS(); \
} while(0)

#define TRACE_ALL_LEVELS_OIPROXY() do { \
		ENTER_OIPROXY();\
		ERR_OIPROXY("COMSA TEST: ERR test string in %s", "oiproxy");\
		WARN_OIPROXY("COMSA TEST: WARN test string in %s", "oiproxy");\
		LOG_OIPROXY("COMSA TEST: LOG test string in %s", "oiproxy");\
		DEBUG_OIPROXY("COMSA TEST: DEBUG string in %s", "oiproxy");\
		LEAVE_OIPROXY(); \
} while(0)

#define TRACE_ALL_LEVELS_PMTSA_EVENT() do { \
		ENTER_PMTSA_EVENT();\
		ERR_PMTSA_EVENT("COMSA TEST: ERR test string in %s", "pmtsa_event");\
		WARN_PMTSA_EVENT("COMSA TEST: WARN test string in %s", "pmtsa_event");\
		LOG_PMTSA_EVENT("COMSA TEST: LOG test string in %s", "pmtsa_event");\
		DEBUG_PMTSA_EVENT("COMSA TEST: DEBUG string in %s", "pmtsa_event");\
		LEAVE_PMTSA_EVENT(); \
} while(0)

/* Add a call to this macro in ComSAAC.c startup to get all availible tracepoints */
#define PRINT_ALL_TRACE_DOMAIN_WITH_ALL_LEVELS() do { \
		TRACE_ALL_LEVELS_COMSA(); \
		TRACE_ALL_LEVELS_IMM(); \
		TRACE_ALL_LEVELS_IMM_OI(); \
		TRACE_ALL_LEVELS_IMM_OM(); \
		TRACE_ALL_LEVELS_MWSA(); \
		TRACE_ALL_LEVELS_MWSA_AC(); \
		TRACE_ALL_LEVELS_MWSA_GENERAL(); \
		TRACE_ALL_LEVELS_MWSA_LOG(); \
		TRACE_ALL_LEVELS_MWSA_REPLIST(); \
		TRACE_ALL_LEVELS_MWSA_TRACE(); \
		TRACE_ALL_LEVELS_OAMSA(); \
		TRACE_ALL_LEVELS_OAMSA_ALARM(); \
		TRACE_ALL_LEVELS_OAMSA_CMEVENT(); \
		TRACE_ALL_LEVELS_OAMSA_TRANSLATIONS(); \
		TRACE_ALL_LEVELS_OIPROXY(); \
		TRACE_ALL_LEVELS_UNDEFINED(); \
		TRACE_ALL_LEVELS_PMTSA_EVENT(); \
} while(0)
#endif /* BUILD_WITH_ALL_TRACE_PRINT_DURING_START */
#endif /* __INTERNAL_TRACE_H_ */
