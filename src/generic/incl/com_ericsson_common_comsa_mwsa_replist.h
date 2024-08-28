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
 *   File:   com_ericsson_common_comsa_mwsa_replist.h
 *
 *   Author: eerwest
 *
 *   Date:   2014-03-31
 *
 *   Definition of the Ericsson CBA component tracepoints that are used in COM SA.
 *
 *   Reviewed:
 *   Modify:
 *
 *****************************************************************************************/


#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER com_ericsson_common_comsa_mwsa_replist

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./com_ericsson_common_comsa_mwsa_replist.h"

#if !defined(COM_ERICSSON_COMMON_COMSA_MWSA_REPLIST_H_) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define COM_ERICSSON_COMMON_COMSA_MWSA_REPLIST_H_

#include "lttng/tracepoint.h"


TRACEPOINT_EVENT(com_ericsson_common_comsa_mwsa_replist, COMSA_ERROR,
		TP_ARGS(const char*, error, const char*, source, const char*, func, int, line),
		TP_FIELDS(ctf_string(Error, error) ctf_string(Source, source) ctf_string(Function, func) ctf_integer(int, Line, line))
		)
TRACEPOINT_LOGLEVEL(com_ericsson_common_comsa_mwsa_replist, COMSA_ERROR, TRACE_ERR)


TRACEPOINT_EVENT(com_ericsson_common_comsa_mwsa_replist, COMSA_WARNING,
		TP_ARGS(const char*, warning, const char*, source, const char*, func, int, line),
		TP_FIELDS(ctf_string(Warning, warning) ctf_string(Source, source) ctf_string(Function, func) ctf_integer(int, Line, line))
		)
TRACEPOINT_LOGLEVEL(com_ericsson_common_comsa_mwsa_replist, COMSA_WARNING, TRACE_WARNING)


TRACEPOINT_EVENT(com_ericsson_common_comsa_mwsa_replist, COMSA_LOG,
		TP_ARGS(const char*, log, const char*, source, const char*, func, int, line),
		TP_FIELDS(ctf_string(Log, log) ctf_string(Source, source) ctf_string(Function, func) ctf_integer(int, Line, line))
		)
TRACEPOINT_LOGLEVEL(com_ericsson_common_comsa_mwsa_replist, COMSA_LOG, TRACE_INFO)


TRACEPOINT_EVENT(com_ericsson_common_comsa_mwsa_replist, COMSA_SA_INIT,
		TP_ARGS(const char*, init, const char*, source, const char*, func, int, line),
		TP_FIELDS(ctf_string(Init, init) ctf_string(Source, source) ctf_string(Function, func) ctf_integer(int, Line, line))
		)
TRACEPOINT_LOGLEVEL(com_ericsson_common_comsa_mwsa_replist, COMSA_SA_INIT, TRACE_DEBUG_PROCESS)


TRACEPOINT_EVENT(com_ericsson_common_comsa_mwsa_replist, COMSA_SA_START,
		TP_ARGS(const char*, start, const char*, source, const char*, func, int, line),
		TP_FIELDS(ctf_string(Start, start) ctf_string(Source, source) ctf_string(Function, func) ctf_integer(int, Line, line))
		)
TRACEPOINT_LOGLEVEL(com_ericsson_common_comsa_mwsa_replist, COMSA_SA_START, TRACE_DEBUG_PROCESS)


TRACEPOINT_EVENT(com_ericsson_common_comsa_mwsa_replist, COMSA_SA_STOP,
		TP_ARGS(const char*, stop, const char*, source, const char*, func, int, line),
		TP_FIELDS(ctf_string(Stop, stop) ctf_string(Source, source) ctf_string(Function, func) ctf_integer(int, Line, line))
		)
TRACEPOINT_LOGLEVEL(com_ericsson_common_comsa_mwsa_replist, COMSA_SA_STOP, TRACE_DEBUG_PROCESS)


TRACEPOINT_EVENT(com_ericsson_common_comsa_mwsa_replist, COMSA_ENTER,
		TP_ARGS(const char*, enter, const char*, source, const char*, func, int, line),
		TP_FIELDS(ctf_string(Enter, enter) ctf_string(Source, source) ctf_string(Function, func) ctf_integer(int, Line, line))
		)
TRACEPOINT_LOGLEVEL(com_ericsson_common_comsa_mwsa_replist, COMSA_ENTER, TRACE_DEBUG_FUNCTION)


TRACEPOINT_EVENT(com_ericsson_common_comsa_mwsa_replist, COMSA_LEAVE,
		TP_ARGS(const char*, leave, const char*, source, const char*, func, int, line),
		TP_FIELDS(ctf_string(Leave, leave) ctf_string(Source, source) ctf_string(Function, func) ctf_integer(int, Line, line))
		)
TRACEPOINT_LOGLEVEL(com_ericsson_common_comsa_mwsa_replist, COMSA_LEAVE, TRACE_DEBUG_FUNCTION)


TRACEPOINT_EVENT(com_ericsson_common_comsa_mwsa_replist, COMSA_DEBUG,
		TP_ARGS(const char*, debug, const char*, source, const char*, func, int, line),
		TP_FIELDS(ctf_string(Debug, debug) ctf_string(Source, source) ctf_string(Function, func) ctf_integer(int, Line, line))
		)
TRACEPOINT_LOGLEVEL(com_ericsson_common_comsa_mwsa_replist, COMSA_DEBUG, TRACE_DEBUG)

#endif /* COM_ERICSSON_COMMON_COMSA_MWSA_REPLIST_H_ */

#include "lttng/tracepoint-event.h"
