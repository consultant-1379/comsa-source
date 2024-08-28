/********************************************************************************************
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
 *   File:   ComSA.h
 *
 *   Author: uablrek
 *
 *   Date:   2010-05-21
 *
 *   This file declares the need functions for COM_SA.
 *
 *   Reviewed: efaiami 2010-07-08
 *   Modify:   efaiami 2011-07-18  add comSAMgmtSpiThreadContInit() for ErrorString
 *   Modify:   efaiami 2011-07-18  Removed get_accessmgm_interface function
 *   Modify:   xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *   Modify:   ejnolsz 2014-04-11  Add new method getMillisecondsSinceEpochUnixTime() for marking COM SA start and stop procedure limits
 *   Modify:   xjonbuc 2014-06-23  Implement MR-20275 support for explicit ccb validate() & abort()
 *   Modify:   xadaleg 2014-08-15  Implement MR35347 increase DN length
 *   Modify:   xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *   Modify:   xthabui 2015-08-05  MR36067-Improved OI/SPI
 *****************************************************************************************/
#ifndef _COMSA_H_
#define _COMSA_H_
#include <MafMgmtSpiInterface_1.h>
#include <libxml2/libxml/tree.h>
#include "MafMgmtSpiThreadContext_2.h"
#include "MafMgmtSpiServiceIdentities_1.h"
#include "MafMgmtSpiInterfacePortal_3.h"
#include "MafMgmtSpiInterfacePortal_3_1.h"
#include <saAis.h>
#include <trace.h>
#include "UtilsInterface.h"
#include "saname_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PATH_DATA_LENGTH 256
#define MAX_CMD_LENGTH 512

extern SaVersionT ImmVersion;

/*
** The definition of the IMM version that supports explicit
** ccb validate() & abort() functionality.
** The same version supports Long DNs, OamSAOiProxy and
** and the forwarding of error strings feature.
*/
#define immReleaseCode 'A'
#define immMajorVersion 2
#define immMinorVersion 15

/*
** The Checkpoint service version
*/

#define immRListReleaseCode 'B'
#define immRListMajorVer 1
#define immRListMinorVer 0

/*
** The AMF API version
*/
#define immAcReleaseCode 'B'
#define immBaseMajorVer 1
#define immBaseMinorVer 1

/*
** The PM API version
*/
#define pmReleaseCode 'A'
#define pmMajorVersion 2
#define pmMinorVersion 1

/* We can have up to 128 error strings and we need a NULL terminator at the end of the array
 * the magic number 128 is defined here in OpenSAF, but we can not include this header file:
 *  ~/osaf/libs/common/immsv/include/immsv_api.h
 *  #define IMMSV_MAX_ATTRIBUTES 128
 */
#define ImmAdmOpNoReturnParams  128

extern MafMgmtSpiInterfacePortal_3_1T* portal_3_1;

/**
 * PSO API: Persistent Storage locations on target
 */
#define PSO_API_CONFIG   "/usr/share/pso/storage-paths/config"
#define PSO_API_CLEAR    "/usr/share/pso/storage-paths/clear"
#define PSO_API_SOFTWARE "/usr/share/pso/storage-paths/software"
#define PSO_API_NOBACKUP "/usr/share/pso/storage-paths/no-backup"
#define PSO_API_USER     "/usr/share/pso/storage-paths/user"

/**
 * command to get internal root
 */
#define COMEA_INT_FILE_SYSTEM_COMMAND "/opt/com/comea/bin/comea file-system internalRoot"

/**
 * default path for internal root
 */
#define DEFAULT_INT_FILEM_PATH "/var/filem/internal_root"

/**
 * local storag path for com-reencryptor component's flat file
 */
#ifndef UNIT_TEST
#define REENCRYPTOR_MODEL_DATA_FILE_PATH "/opt/com/run/reencryptor-model-data"
#else
#define REENCRYPTOR_MODEL_DATA_FILE_PATH "UT-reencryptor-model-data"
#endif

/**
 * This handle is used for calls to the SelectTimer functions within
 * ComSA.
 */
extern void* comSASThandle;
extern void* comSASThandle_ntf;

/**
 * Trace Service.
 */
MafReturnT comSATraceStart(xmlNode* cfg_file);
void comSATraceStop(void);

//LM SA Trace start/stop routines
MafReturnT lmSATraceStart();
void lmSATraceStop();

MafMgmtSpiInterface_1T* comSATraceInterface(void);
/* SDP 1694 - support MAF SPI */
MafMgmtSpiInterface_1T* maf_comSATraceInterface(void);
MafMgmtSpiInterface_1T* maf_comSATraceInterface_2(void);

void ClearOamContextTransactionMap(void);
/* SDP 1694 - support MAF SPI */

/**
 * MW OAM Component control (in ntf)
 */
MafReturnT maf_comSAOamComponentInitialize(
	xmlDocPtr config,
	MafMgmtSpiInterfacePortal_3T* portal_MAF);

void maf_comSAOamComponentFinalize(
	MafMgmtSpiInterfacePortal_3T* portal_MAF);

/**
* MW TEST Component control
*/
MafReturnT maf_comSATestComponentInitialize(
	MafMgmtSpiInterfacePortal_3T* _portal,
	xmlDocPtr config);

void maf_comSATestComponentFinalize(
	MafMgmtSpiInterfacePortal_3T* portal);

/**
 * MW Component control (in rlist)
 */
MafReturnT maf_comSAMwComponentInitialize(
	MafMgmtSpiInterfacePortal_3T* portal,
	xmlDocPtr config);
void maf_comSAMwComponentFinalize(
	MafMgmtSpiInterfacePortal_3T* portal);

MafReturnT maf_comSAMgmtSpiThreadContInit(
		MafMgmtSpiInterfacePortal_3T* _portal);

MafReturnT maf_oamSAOiProxyInitialization(
		MafMgmtSpiInterfacePortal_3T* _portal);

void maf_ClearOamContextTransactionMap(void);

/**
 * Error;
 */
void err_quit(char const* fmt, ...)
   __attribute__((noreturn, format(printf,1,2)));

/**
 * Com_SA Internal Trace
 */

//#define DEBUG(arg...) coremw_debug_log(LOG_DEBUG,arg)
//#define ERR(arg...) coremw_log(LOG_ERR, arg)
//#define WARN(arg...) coremw_log(LOG_WARNING, arg)
//#define LOG(arg...) coremw_log(LOG_NOTICE, arg)
#include "InternalTrace.h"


/*
 * Config help;
 */
#define xstrcmp(a,b) xmlStrcmp(a,(xmlChar const*)b)

#define isElementName(n,s) \
	(n->type == XML_ELEMENT_NODE && xstrcmp(n->name, s) == 0)

xmlNode* coremw_find_config_item(
	char const* component_name, char const* node_name);
char const* coremw_xmlnode_contents(xmlNode* cur);
char const* coremw_xmlnode_get_attribute(xmlNode* n, char const* attr_name);
unsigned long long getMillisecondsSinceEpochUnixTime();

/*
 * Return value convertion
 */

/*SDP1694 - support MAF SPI */
MafReturnT maf_coremw_rc(SaAisErrorT rc);

void auditlogger(char* tag, int priority, int add_pid,
        char* msgid, const char *format_msg);

#ifdef __cplusplus
}
#endif
#endif
