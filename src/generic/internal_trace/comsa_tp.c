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
 *   File:   comsa_tp.c
 *
 *   Author: eerwest
 *
 *   Date:   2014-03-31
 *
 *   Instrumentation of the defined Ericsson CBA component tracepoints that are used in COM SA.
 *
 *   Reviewed:
 *   Modify:
 *
 *****************************************************************************************/


#define TRACEPOINT_CREATE_PROBES
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
#include "com_ericsson_common_comsa_pmtsa.h"
#include "com_ericsson_common_comsa_pmtsa_event.h"
#include "com_ericsson_common_comsa_undefined.h"

