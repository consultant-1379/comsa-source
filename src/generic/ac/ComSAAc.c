/*******************************************************************************
 * Copyright (C) 2010 by Ericsson AB
 * S - 125 26  STOCKHOLM
 * SWEDEN, tel int + 46 10 719 0000
 *
 * The copyright to the computer program herein is the property of
 * Ericsson AB. The program may be used and/or copied only with the
 * written permission from Ericsson AB, or in accordance with the terms
 * and conditions stipulated in the agreement/contract under which the
 * program has been supplied.
 *
 * All rights reserved.
 *
 *
 *   Author: uablrek
 *
 *   File: ComSAAc.c
 *
 * 2011-02-23	egorped 	Do not terminate if LogServiceOpen fails
 * 							nor if trace services cannot be started
 *
 *   AvailabilityControlled for CoreMW-COM_SA.
 *   Modified: xjonbuc 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *   Modified: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 *******************************************************************************
 *
 * Reviewed: uaberin 2010-06-21
 *
 * Modify: efaiami 2011-02-21 trace and log function
 *
 * Modify: efaiami 2011-11-06 update comLCMinit for PSO
 *
 * Reviewed: eaparob 2011-09-19
 * Reviewed: ejnolsz 2011-09-20
 *
 * Modify: eaparob 2012-05-24 update comLCMinit for SDP1120 - temporary change to get the MAF portal in comLCMinit to be able to use MAF service for SDP1120
 * Modify: efaiami & eaparob 2012-07-23 update amf_csi_set_callback() for open and close AlarmAndAlertLogService.
 * Reviewed: efaiami 2012-07-31 amf_csi_set_callback()
 *
 * Modify: xjonbuc 2012-09-06 create mafLCMinit and mafLCMterminate for SDP1694 - support MAF SPI
 * Modify: xdonngu 2013-12-12 run cppcheck 1.62 and fix errors and warnings
 * Modify: uabjoy  2014-03-18 Adopt to Trace CC
 * Modify: ejnolsz 2014-04-11 add and use new method getMillisecondsSinceEpochUnixTime() for marking COM SA start and stop procedure limits
 * Modify: xthabui 2015-08-05 MR36067-Improved OI/SPI
 */

#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>
#include <libxml2/libxml/parser.h>
#include <sys/wait.h>
#include <stdlib.h>

// CoreMW Headers
#include <saAmf.h>
// COM Headers
#include <MafMgmtSpiLibraryComponentManager_1.h>
#include <MafMgmtSpiInterfacePortal_3.h>
#include <MafMwSpiServiceIdentities_1.h>
#include <MafMwSpiAvailabilityController_1.h>

// COMSA Headers
#include "SelectTimer.h"
#include "ComSALogService.h"
#include <trace.h>
#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#include "ComSA.h"
#include "OamSAOIProxy.h"
#include <dlfcn.h>

//defines for logger
#define LDE_LOGGER_LIBRARY "/usr/lib64/liblde-logger.so"
#define LDE_LOGGER_OPEN_FUNCTION "rfc5424_open"
#define LDE_LOGGER_CLOSE_FUNCTION "rfc5424_close"
#define LDE_LOGGER_LOG_FUNCTION "rfc5424_syslog"

//As syslog.h file does not contain predefined macro for security_audit
// declared a new variable here.
#define	LOG_SECURITY_AUDIT	(13<<3)

static void * lib = NULL;

/* LDE Logger Functions */
void (*audit_logging)(char* tag, int priority, int add_pid,
                      char* msgid, const char *format_msg, ...);

void (*rfc5424_open)(int facility);
void (*rfc5424_close)();

void auditlogger(char* tag, int priority, int add_pid,
        char* msgid, const char *format_msg)
{
	if(audit_logging){
		audit_logging(tag,priority,add_pid,msgid,"%s",format_msg);
	}
}

void load_logger_library()
{
	if(getenv("SECURITY_STREAM")) {
		// Step-1 : Load LDE Logger
		lib = dlopen(LDE_LOGGER_LIBRARY,RTLD_LAZY);
		if(lib) {
			rfc5424_open = dlsym(lib, LDE_LOGGER_OPEN_FUNCTION);
			audit_logging = dlsym(lib,LDE_LOGGER_LOG_FUNCTION);
			rfc5424_close = dlsym(lib, LDE_LOGGER_CLOSE_FUNCTION);

			if(!audit_logging) {
				WARN_COMSA("rfc5424_syslog symbol loading error : %s",dlerror());
				dlclose(lib);
				lib = NULL;
				rfc5424_open = NULL;
				rfc5424_close = NULL;

			}

		} else {
			WARN_COMSA("liblde-logger.so loading failed with error : %s",dlerror());
		}

	}
}

#ifndef UNIT_TEST
static char pathToStorageDirectory[MAX_PATH_DATA_LENGTH] = "\0";
#else
char pathToStorageDirectory[MAX_PATH_DATA_LENGTH] = "\0";
#endif

static char sshdFileName[] = "/opt/com/run/AMF_handles_SSHD";
static char sshdByComsa[] = "/opt/com/run/ComSA_handles_SSHD";
static char mimToolFileName[] = "pending_comsa_restart";
static char comsaProductDir[] = COMSA_FOR_COREMW_DIR;

#ifdef CODE_COVERAGE
// #include <gcc/gcov-io.h>  /* The prototype below should be in this header file */
void __gcov_flush(void);     /* define here until we find the header file         */
#endif
// integer to control if COM SA should handle sshd control
static int comsa_sshd_control = 1;
//static int comsa_sshd_control = 0; // The default should be 1, temporarily set to 0 for debugging only

//sshd update and reload command to execute
const char* updateScript = " /usr/bin/update_sshd -d";
const char* reloadScript = " /usr/bin/reload_sshd";

// MDF registration command to execute
const char modelConsumerReadycmd[] = "cmw-modelconsumer-ready COM_R1 /opt/com/util/comsa_mdf_consumer";

#define SAF_API_CALL(f) do {\
	unsigned int try = 0;\
	for (rc = f; rc == SA_AIS_ERR_TRY_AGAIN&& try < AMF_MAX_TRIES; rc = f, try++){\
		sleep(AMF_TRY_INTERVAL);\
	}} while (0)
#define SAF_API_INIT_CALL(f) do {\
	unsigned int try = 0;\
	SaVersionT ver = api_version;\
	for (rc = f; rc == SA_AIS_ERR_TRY_AGAIN&& try < AMF_MAX_TRIES; rc = f, try++){\
		sleep(AMF_TRY_INTERVAL);\
		ver = api_version;\
	}} while (0)

/* Forward declarations; */
static int alertalarm=0;

unsigned long long getMillisecondsSinceEpochUnixTime();
MafReturnT InitAmfSshdControl();
void handshakeToComsaMimTool();

/*
 * Utility functions to convert case.
 * NOTE: FREE THE RETURN char* VALUE AFTER USE!
 */

/* SDP 1694 - support MAF SPI */
static MafReturnT maf_coremw_start(MafStateChangeReasonT reason);
static MafReturnT maf_coremw_stop(MafStateChangeReasonT reason);
static MafReturnT maf_coremw_acInitialize(const MafMwSpiAcT* acCallbacks);
static MafReturnT maf_coremw_healthCheckReport(
	MafMwSpiAvailabilityInvocationT invocation, MafReturnT healthReport,
	MafMwSpiRecommendedRecoveryT recommendedRecovery);
static MafReturnT maf_coremw_haModeAssumed(
	MafMwSpiAvailabilityInvocationT invocation, MafReturnT error);
static MafReturnT maf_coremw_prepareTerminationResponse(
	MafMwSpiAvailabilityInvocationT invocation, MafReturnT error);

static const MafMwSpiAcT* maf_acCallbacks = NULL;
static void* maf_amf_thread(void* arg);

static void saAmfResponse_wrapper(
	SaAmfHandleT amfHandle,
	SaInvocationT invocation,
	SaAisErrorT error);

/* check if com-reencryptor component's flat file exists.
 * If yes, remove.
 */
void removeSecretAttrFile();

MafMgmtSpiInterfacePortal_3T* portal_MAF;

/* This portal is used for fetching the Component Interface Array from registered Components */
MafMgmtSpiInterfacePortal_3_1T* portal_3_1 = NULL;

/* MDF registration functions */
extern void removeFileMConsumer();
extern void killChildIfAny();
extern void setUpInstanceModelConsumers();

/* Data; */
#define COM_INITIATED_INVOCATION 0
#define AMF_MAX_TRIES 10
#define AMF_TRY_INTERVAL 2
#define COMPONENT_NAME "MW_BASE"
static char const* const component_name = COMPONENT_NAME;
static int event_pipe[2];
static pthread_t amf_pthread;
static int amf_pthread_isrunning = 0;
int ntf_pthread_isrunning = 0;
pthread_t ntf_pthread;
void* comSASThandle_ntf = NULL;
void* comSASThandle = NULL;
static xmlParserCtxtPtr ctxt = NULL;
static xmlDocPtr doc = NULL;
static SaVersionT const api_version = {immAcReleaseCode, immBaseMajorVer, immBaseMinorVer};

extern int ntf_event_pipe[2];

/* (the COM structures must live while the component is) */
static MafMwSpiAvailabilityController_1T maf_ac = {
	.base = MafMwSpiAvailabilityController_1Id,
	.acInitialize = maf_coremw_acInitialize,
	.healthCheckReport = maf_coremw_healthCheckReport,
	.haModeAssumed = maf_coremw_haModeAssumed,
	.prepareTerminationResponse = maf_coremw_prepareTerminationResponse
};

static MafMgmtSpiInterface_1T* maf_ifarray[4]; /* {ac, log, trace v1/v2, NULL} */
static MafMgmtSpiInterface_1T* maf_deparray[] =  { NULL };
static MafMgmtSpiInterface_1T* maf_optarray[] =  { NULL };
static MafMgmtSpiComponent_2T maf_coremw_sa = {
	.base = {COMPONENT_NAME, "MafMgmtSpiComponent","2"},
	.interfaceArray = maf_ifarray,
	.dependencyArray = maf_deparray,
	.start = maf_coremw_start,
	.stop = maf_coremw_stop,
	.optionalDependencyArray = maf_optarray
};


/*
 * mafLCMinit --
 * mafLCMterminate --
 *  Public functions called by MAF on load/unload(?) of the dynamic lib.
 */
MafReturnT
mafLCMinit(
	MafMgmtSpiInterfacePortalAccessorT* accessor,
	const char* config)
{
	MafReturnT rc = MafOk;
	ENTER_COMSA();
	LOG_COMSA("mafLCMinit(): Enter");

	char* tmp = convertToLowerCase((const char*)CC_NAME);
	asprintf(&_CC_NAME, "%s", tmp);
	if (tmp)
	{
		free(tmp);
		tmp = NULL;
	}
	tmp = convertToUpperCase((const char*)CC_NAME);
	asprintf(&_CC_NAME_UPPERCASE, "%s", tmp);
	if (tmp)
	{
		free(tmp);
		tmp = NULL;
	}
	if(_CC_NAME_SA == NULL)
	{
		asprintf(&_CC_NAME_SA, "%s%s", _CC_NAME_UPPERCASE, "_SA");
	}

	LOG_COMSA("mafLCMinit(): %s init procedure begins: %llu", _CC_NAME_SA, getMillisecondsSinceEpochUnixTime());
	LOG_COMSA("mafLCMinit() Loaded (compiled %s %s) ...", __DATE__, __TIME__);

	load_logger_library();
	if(rfc5424_open){
		rfc5424_open(LOG_SECURITY_AUDIT);
		LOG_COMSA("logger API successfully opened");
	}
	/* We better get ready for calls from COM ASAP */
	if (pipe(event_pipe) != 0) {
		ERR_COMSA("Failed to create pipe");
		return MafFailure;
	}else {
		/* Set the read-end of the pipe in non-blocking mode */
		int rc = fcntl(event_pipe[0], F_GETFL);
		if (rc < 0) abort();
		if (fcntl(event_pipe[0], F_SETFL, rc | O_NONBLOCK)) abort();
	}

	asprintf(&tmp, "%s%s", _CC_NAME, "sa_pso start");
	if (processOpen(tmp) != 0) {
		WARN_COMSA("%s called failed", tmp);
	}
	free(tmp);
	tmp = NULL;

#ifdef BUILD_WITH_TRACE_TESTS
	PRINT_ALL_TRACE_DOMAIN_WITH_ALL_LEVELS();
#endif
	portal_MAF = (MafMgmtSpiInterfacePortal_3T*)accessor->getPortal("3");
	if (portal_MAF == NULL) {
		ERR_COMSA("No MAF portal");
		return MafFailure;
	}

	/* Parse the config data */
	if (config != NULL) {
		ctxt = xmlNewParserCtxt();
		if (ctxt == NULL)
			err_quit("Failed to create XML-parser context");
		doc = xmlCtxtReadMemory(
			ctxt, config, strlen(config), "config", NULL, 0);
		if (doc == NULL)
			ERR_COMSA("Failed to parse configuration");
	}

	DEBUG_COMSA("Create timer handle.");
	/* SelectTimer init; */
	comSASThandle = timerCreateHandle_r();
	poll_maxfd(comSASThandle, 16);
	comSASThandle_ntf = timerCreateHandle_r();
	poll_maxfd(comSASThandle_ntf, 16);

	/* Initialize the MW SA Base component; */
	maf_ac.base.componentName = component_name;
	maf_ifarray[0] = (MafMgmtSpiInterface_1T*)&maf_ac;
	maf_ifarray[1] = (MafMgmtSpiInterface_1T*)maf_ExportLogServiceInterface();
	maf_ifarray[1]->componentName = component_name;

	//NOTE: Only LM CC uses Trace SPI V2 implementation.
	//      So, all other components will register with V1 implementation.
	if(strcmp("lm", _CC_NAME))
	{
		maf_ifarray[2] = maf_comSATraceInterface();
	}
	else
	{
		maf_ifarray[2] = maf_comSATraceInterface_2();
	}
	maf_ifarray[2]->componentName = component_name;
	maf_ifarray[3] = NULL;

	DEBUG_COMSA("Start the %s log Service.", _CC_NAME);
	if ((rc = maf_ComLogServiceOpen()) != MafOk) {
		syslog(LOG_USER*8 + LOG_WARNING, "%s %s %d", _CC_NAME_SA, " -- LogService failed to start, error = ",rc);
	} else {
		auditlogger("com",LOG_INFO,1,"security_audit","Com LogService successfully opened");
	}

	/* Com_SA internal Trace */
	#ifdef _TRACE_FLAG
		/* Fix for HS32948: Call tzset() before using localtime_r() while writting log trace */
		tzset();
		struct log_state_t log=
		{
			LOG_LEVEL_DEBUG,
			TRACE_TAG_LOG | TRACE_TAG_ENTER | TRACE_TAG_LEAVE,
			LOG_MODE_FILE | LOG_MODE_FILELINE | LOG_MODE_TIMESTAMP
		};

		log_control( &log, 0 );
		createComSaTraceDirectory();

		char* log_file = NULL;
		asprintf(&log_file, "%s%s%s%s%s", "/var/opt/", _CC_NAME, "sa/", _CC_NAME,"sa.trc");
		log_to_file(log_file);
		free(log_file);

		log_init(_CC_NAME_SA);

		LOG_PRINTF(LOG_LEVEL_DEBUG,"%s Trace........\n", _CC_NAME_SA);
	#endif

	if(strcmp("lm", _CC_NAME)) {
		rc = (MafReturnT) comSATraceStart(
			coremw_find_config_item(component_name, "traceconfig"));
	} else {
		rc= (MafReturnT) lmSATraceStart();
	}

	if (rc != MafOk)
	{
		syslog(LOG_USER*8 + LOG_WARNING, "%s %s %d", _CC_NAME_SA, " -- TraceService failed to start, error = ",rc);
	}

	LOG_COMSA("Calling MAF registerComponent ...");
	if ((rc = portal_MAF->registerComponent(&maf_coremw_sa)) != MafOk)
	{
	  ERR_COMSA("MAF registerComponent failed, rc=%d", rc);
	  return rc;
	}
	/*Provides the replicated list interface */
	if ((rc = maf_comSAMwComponentInitialize(portal_MAF, doc)) != MafOk)  {
	  ERR_COMSA("maf_comSAMwComponentInitialize failed, rc=%d", rc);
	  return rc;
	}

	 //  Provides 3 interfaces - TransactionalResource, ManagedObjects  and RegisteredObjectImplementer which must be COM SPI
	 // Also provides Notification service which must be MAF SPI
	if ((rc = maf_comSAOamComponentInitialize(doc, portal_MAF)) != MafOk) {
		ERR_COMSA("maf_comSAOamComponentInitialize failed, rc=%d", rc);
		return rc;
	}


	// Need to provide portal to ImmBridge because TransactionalResource i/f uses portal
	// which is provided from comSAOamComponentInitialize() above.
	// Can provide both COM and MAF portals in case either is used.
	/* Saves the portal to ImmBridge.cc for interfaces to use */
	if ((rc = maf_comSAMgmtSpiThreadContInit(portal_MAF)) != MafOk)	{
		DEBUG_COMSA("Failed to init maf_comSAMgmtSpiThreadContInit");
		return rc;
	}

	// MANDATORY. Must set MAF Portal in the OamSAOIProxy part.
	// Portal is used in OIProxy during maf_amf_csi_set_callback() to write to IMM
	// Portal is used by OIProxy for Transactions, so must be the same type as TransactionResource
	// Interface which is set by comSAOamComponentInitialize() above.
	if ((rc =(MafReturnT) maf_oamSAOiProxyInitialization(portal_MAF)) != MafOk)	{
		DEBUG_COMSA("Failed to init maf_oamSAOiProxyInitialization");
		return rc;
	}

	// Link defined instance model types for FM and Authorization in coreMw if the MDF (Model Delivery Function)
	// is present
	setUpInstanceModelConsumers();

	#ifdef FTEST
	DEBUG_COMSA("Calling maf_comSATestComponentInitialize");
	if ((rc = (MafReturnT)maf_comSATestComponentInitialize(portal_MAF, doc)) != MafOk)
	{
		return rc;
	}
	#endif

	ClearOamContextTransactionMap();

	// Initialize the COM SA AMF SSHD control (global variable and a file indicating enable/disable)
	if(strcmp("lm", _CC_NAME)) //This is applicable for COM only.
	{
		rc = InitAmfSshdControl();
		if (MafOk != rc)
		{
			return rc;
		}
	}

	// MR36067: This variable is used for get the Component Interface Array
	portal_3_1 = (MafMgmtSpiInterfacePortal_3_1T*)accessor->getPortal("3_1");
	if (portal_3_1 == NULL) {
		ERR_COMSA("No MAF portal for version 3_1");
		return MafFailure;
	}

	LOG_COMSA("mafLCMinit return OK");
	LOG_COMSA("mafLCMinit(): %s SA init procedure completed: %llu", _CC_NAME_SA, getMillisecondsSinceEpochUnixTime());
	LEAVE_COMSA();
	return MafOk;
}


void mafLCMterminate()
{
	ENTER_COMSA();
	LOG_COMSA("mafLCMterminate called");
	LOG_COMSA("mafLCMterminate(): %s SA terminate procedure begins: %llu", _CC_NAME_SA, getMillisecondsSinceEpochUnixTime());
	/* Stop other components */
#ifdef CODE_COVERAGE
	/* This is for Code Coverage testing only */
	LOG_COMSA("mafLCMterminate: calling __gcov_flush()...");
	__gcov_flush();
	LOG_COMSA("mafLCMterminate: after calling __gcov_flush()...");
#endif

	DEBUG_COMSA("mafLCMterminate() alertalarm = %d", alertalarm);
	//This block is applicable for COM only.
	if(strcmp("lm", _CC_NAME)) {
		if (alertalarm!=0)
		{
			alertalarm=0;
			DEBUG_COMSA("mafLCMterminate() alertalarm = %d", alertalarm);
			if((MafReturnT)AlarmAndAlertLogServiceClose() != MafOk)
			{
				WARN_COMSA("mafLCMterminate() failed to close AlarmAndAlertLogServiceClose()");
				//myRetVal = MafFailure;
			}
			if((MafReturnT)cmdLogServiceClose() != MafOk)
			{
				WARN_COMSA("mafLCMterminate() failed to close CmdLogServiceClose()");
			}
			if((MafReturnT)secLogServiceClose() != MafOk)
			{
				WARN_COMSA("mafLCMterminate() failed to close secLogServiceClose()");
			}
		}
		comSATraceStop();
	} else {
		lmSATraceStop();
	}
	maf_comSAOamComponentFinalize(portal_MAF);
	maf_comSAMwComponentFinalize(portal_MAF);

#ifdef FTEST
	DEBUG_COMSA("Calling comSATestComponentFinalize");
	maf_comSATestComponentFinalize(portal_MAF);
#endif
	stop_LogEventProducer();

	/* Stop my own interfaces */
	(void)ComLogServiceClose();

	DEBUG_COMSA("Calling unregisterComponent coremw_sa");
	portal_MAF->unregisterComponent(&maf_coremw_sa);

	if (doc != NULL) xmlFreeDoc(doc);
	if (ctxt != NULL) xmlFreeParserCtxt(ctxt);
	if (_CC_NAME != NULL) {
		free(_CC_NAME);
		_CC_NAME = NULL;
	}
	if (_CC_NAME_UPPERCASE != NULL) {
		free(_CC_NAME_UPPERCASE);
		_CC_NAME_UPPERCASE = NULL;
	}


	LOG_COMSA("mafLCMterminate(): %s SA terminate procedure completed: %llu", _CC_NAME_SA, getMillisecondsSinceEpochUnixTime());

	if (_CC_NAME_SA != NULL){
		free(_CC_NAME_SA);
		_CC_NAME_SA = NULL;
	}

	if (rfc5424_close){
	    rfc5424_close();
	    LOG_COMSA("logger API successfully closed");
	}

	if(lib) {
	    dlclose(lib);
	    lib=NULL;
	    audit_logging=NULL;
	    rfc5424_open=NULL;
	    rfc5424_close=NULL;

	}
	LEAVE_COMSA();
}




/* ----------------------------------------------------------------------
 * Event handling;
 *   When a call from COM is received an event must be sent to the
 *   COM_SA thread who will carry out the request. This is implemented
 *   with an sychronized event-queue and a pipe to wake up the COM_SA
 *   thread.
 */

/*
	 Event handling principle;

				 COM_SA thread                 COM thread
					 |                             |
  SAF up-call ------>|                             |
					 |                             |
				  Create event                     |
				  Object                           |
					 |                             |
				  Associate the                    |
				  Event object with                |
				  an "invocation"                  |
					 |                             |
					 |        Call to COM          |
					 |---------------------------->|
					 |                             |
					 |<- - - - - - - - - - - - - - |
					 |            return           |
					 |                             |
					 |                             |
					 |       Event handling        |
					 |             |               |
					 |             |    Call-back from COM
					 |             |<--------------|
					 |             |               |
					 |        Get the event        |
					 |        by the "invocation"  |
					 |             |               |
					 |        Add the event        |
					 |        to the queue         |
					 |             |               |
					 |        Send something       |
					 |        on the event-pipe    |
					 |             |               |
					 |             |               |
					 |             |- - - - - - - >|
					 |             |   return      |
					 |                             |
					 |                             |
					 |                             |
	Pipe-input ----->| get-queue   |               |
					 |------------>|               |
					 |             |               |
					 |< - - - - - -|               |
					 |             |               |
				Flush the pipe                     |
					 |                             |
				Handle events                      |
					 |                             |
					 |                             |

 */
enum CoremwEventType {
	EVENT_None,
	EVENT_Terminate,
	EVENT_healthCheckReport,
	EVENT_haModeAssumed,
	EVENT_prepareTerminationResponse
};
struct CoremwEvent {
	struct CoremwEvent* next;
	enum CoremwEventType event_type;
	SaInvocationT saf_invokation;
	unsigned int flags;
	MafReturnT error;
	MafMwSpiRecommendedRecoveryT recommendedRecovery;
};

static pthread_mutex_t eventq_lock = PTHREAD_MUTEX_INITIALIZER;


/* ----------------------------------------------------------------------
 * SDP1694: MAF Call-back functions
 */
struct maf_CoremwEvent {
	struct maf_CoremwEvent* next;
	enum CoremwEventType event_type;
	SaInvocationT saf_invokation;
	unsigned int flags;
	MafReturnT error;
	MafMwSpiRecommendedRecoveryT recommendedRecovery;
};


static struct maf_CoremwEvent* maf_eventq_head = NULL;
static struct maf_CoremwEvent* maf_eventq_tail = NULL;


static struct maf_CoremwEvent* maf_new_event(
	enum CoremwEventType event_type,
	SaInvocationT saf_invokation,
	unsigned int flags)
{
	ENTER_MWSA_GENERAL();
	struct maf_CoremwEvent* event;
	event = (struct maf_CoremwEvent*) malloc(sizeof(struct maf_CoremwEvent));
	if (event == NULL) abort(); /* Out of mem */
	event->event_type = event_type;
	event->saf_invokation = saf_invokation;
	event->flags = flags;
	DEBUG_MWSA_GENERAL("maf_new_event (%p) event_type=%u", event, event->event_type);
	LEAVE_MWSA_GENERAL();
	return event;
}

static void maf_event_add(struct maf_CoremwEvent* event) {
	ENTER_MWSA_GENERAL();
	if (pthread_mutex_lock(&eventq_lock) != 0) abort();
	event->next = NULL;
	if (maf_eventq_tail == NULL) {
		DEBUG_MWSA_GENERAL("Add first event %u", event->event_type);
		assert(maf_eventq_head  == NULL);
		maf_eventq_head  = maf_eventq_tail = event;
		if (write(event_pipe[1], "A", 1) != 1) abort();
	} else {
		DEBUG_MWSA_GENERAL("Add subsequent event %u", event->event_type);
		assert(maf_eventq_head != NULL);
		maf_eventq_tail->next = event;
		maf_eventq_tail = event;
	}
	if (pthread_mutex_unlock(&eventq_lock) != 0) abort();
	LEAVE_MWSA_GENERAL();
}

static struct maf_CoremwEvent* maf_event_getq(void) {
	ENTER_MWSA_GENERAL();
	struct maf_CoremwEvent* eventq;
	char buffer[16];
	int rc;
	if (pthread_mutex_lock(&eventq_lock) != 0) abort();
	eventq = maf_eventq_head;
	maf_eventq_head = maf_eventq_tail = NULL;
	/* Flush the pipe (should contain max 1 character) */
	DEBUG_MWSA_GENERAL("Reading from pipe...");
	rc = read(event_pipe[0], buffer, sizeof(buffer));
	DEBUG_MWSA_GENERAL("Read from pipe returned, rc=%d", rc);
	if (rc < 0) {
		if (errno != EWOULDBLOCK) err_quit(
			"Failed to read from pipe [%s]", strerror(errno));
		DEBUG_MWSA_GENERAL("EWOULDBLOCK on pipe");
	} else if (rc > 1) {
		WARN_MWSA_GENERAL("event_pipe, read=%d", rc);
	}
	if (pthread_mutex_unlock(&eventq_lock) != 0) abort();
	LEAVE_MWSA_GENERAL();
	return eventq;
}

static MafReturnT maf_coremw_start(MafStateChangeReasonT reason)
{
	ENTER_MWSA_GENERAL();
	LOG_MWSA_GENERAL("maf_coremw_start called");

	LEAVE_MWSA_GENERAL();
	return MafOk;
}

static MafReturnT maf_coremw_stop(MafStateChangeReasonT reason)
{
	ENTER_MWSA_GENERAL();
	DEBUG_MWSA_GENERAL("maf_coremw_stop called");
	LOG_MWSA_GENERAL ("maf_coremw_stop(): MW SA component stop procedure begins: %llu", getMillisecondsSinceEpochUnixTime());

	// kill the children if any
	killChildIfAny();

	if (ntf_pthread_isrunning) {
		// Write stop ntf event
		write(ntf_event_pipe[1], "A", 1);
		// Joint the ntf polling thread
		LOG_MWSA_GENERAL("Stopping the ntf_pthread...");
		(void)pthread_cancel(ntf_pthread);
		(void)pthread_join(ntf_pthread, NULL);
		LOG_MWSA_GENERAL("The ntf_pthread is stopped");
		ntf_pthread_isrunning = 0;
	}
	if (amf_pthread_isrunning) {
		struct maf_CoremwEvent* event = maf_new_event(EVENT_Terminate, 0, 0);
		maf_event_add(event);
		/* Here we can potentially block the COM thread, so do
		 * some logging. */
		LOG_MWSA_GENERAL("Stopping the amf_pthread...");
		(void)pthread_join(amf_pthread, NULL);
		LOG_MWSA_GENERAL("The amf_pthread is stopped");
		amf_pthread_isrunning = 0;
	}
	maf_acCallbacks = NULL;
	LOG_MWSA_GENERAL ("maf_coremw_stop(): MW SA component stop procedure completed: %llu", getMillisecondsSinceEpochUnixTime());
	LEAVE_MWSA_GENERAL();
	return MafOk;
}

static MafReturnT maf_coremw_acInitialize(const MafMwSpiAcT* i_acCallbacks)
{
	ENTER_MWSA_GENERAL();
	pthread_attr_t attr;
	DEBUG_MWSA_GENERAL("maf_coremw_acInitialize called");
	maf_acCallbacks = i_acCallbacks;
	if (pthread_attr_init(&attr) != 0) {
		ERR_MWSA_GENERAL("Failed attr_init maf_amf_thread");
		return MafFailure;
	}

	if (pthread_create(&amf_pthread, &attr, maf_amf_thread, NULL) != 0) {
		ERR_MWSA_GENERAL("Failed to start maf_amf_thread");
		return MafFailure;
	}
	(void)pthread_attr_destroy(&attr);

	/* Do not "improve" by moving the setting of
	 * amf_pthread_isrunning to the thread itself! Then you will
	 * get yourself a race-condition. */
	amf_pthread_isrunning = 1;
	LEAVE_MWSA_GENERAL();
	return MafOk;
}

static MafReturnT maf_coremw_healthCheckReport(
	MafMwSpiAvailabilityInvocationT invocation, MafReturnT healthReport,
	MafMwSpiRecommendedRecoveryT recommendedRecovery)
{
	ENTER_MWSA_GENERAL();
	struct maf_CoremwEvent* eventp;
	DEBUG_MWSA_GENERAL("maf_coremw_healthCheckReport called");

	if (invocation == COM_INITIATED_INVOCATION) {

		/* According to the COM documentation a
		 * healthCheckReport may be called out-of-the-blue to
		 * report some error. In this case the "invocation"
		 * parameter must have a special value. There is no
		 * associated up-call and therefore no associated
		 * event.
		 */

		eventp = maf_new_event(EVENT_healthCheckReport, 0, 1);

	} else {
		eventp = (struct maf_CoremwEvent*)invocation;
	}

	if (eventp->event_type != EVENT_healthCheckReport) {
		err_quit("API call-call-back mismatch (%u,%u)",
			 EVENT_healthCheckReport, eventp->event_type);
	}
	eventp->error = healthReport;
	eventp->recommendedRecovery = recommendedRecovery;
	maf_event_add(eventp);
	DEBUG_MWSA_GENERAL("maf_coremw_healthCheckReport end");
	LEAVE_MWSA_GENERAL();
	return MafOk;
}
static MafReturnT maf_coremw_haModeAssumed(
	MafMwSpiAvailabilityInvocationT invocation, MafReturnT error)
{
	ENTER_MWSA_GENERAL();
	struct maf_CoremwEvent* eventp;
	DEBUG_MWSA_GENERAL("maf_coremw_haModeAssumed called");

	eventp = (struct maf_CoremwEvent*)invocation;
	if (eventp->event_type != EVENT_haModeAssumed) {
		err_quit("API call-call-back mismatch (%u,%u)",
			 EVENT_haModeAssumed, eventp->event_type);
	}
	eventp->error = error;
	maf_event_add(eventp);
	DEBUG_MWSA_GENERAL("maf_coremw_haModeAssumed end");
	LEAVE_MWSA_GENERAL();
	return MafOk;
}

static MafReturnT maf_coremw_prepareTerminationResponse(
	MafMwSpiAvailabilityInvocationT invocation, MafReturnT error)
{
	ENTER_MWSA_GENERAL();
	struct maf_CoremwEvent* eventp;
	DEBUG_MWSA_GENERAL("maf_coremw_prepareTerminationResponse called");

	eventp = (struct maf_CoremwEvent*)invocation;
	if (eventp->event_type != EVENT_prepareTerminationResponse) {
		err_quit("API call-call-back mismatch (%u,%u)",
			 EVENT_prepareTerminationResponse, eventp->event_type);
	}
	eventp->error = error;
	maf_event_add(eventp);
	LEAVE_MWSA_GENERAL();
	return MafOk;
}


/* ----------------------------------------------------------------------
 * AMF - Interface
 */

/* AMF Handle */
static SaAmfHandleT amf_handle = 0;

/* HealthCheck Key on which healthcheck is started */
static SaAmfHealthcheckKeyT gl_healthcheck_key_comsa = {"COM_SA", 6};
static SaAmfHealthcheckKeyT gl_healthcheck_key_lmsa = {"LM_SA", 5};


/* COM has a problem with certain transitions. */
static int stop_thread = 0;
int ntf_stop_thread = 0;
static SaNameT gl_comp_name;

void* ntf_thread(void* arg)
{
	ENTER_MWSA_GENERAL();
	LOG_MWSA_GENERAL("ntf_thread running ...");
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	while (!ntf_stop_thread) {
		if (poll_execute(comSASThandle_ntf) != 0)
			err_quit("Drop out of the poll-loop for ntf");
	}
	LEAVE_MWSA_GENERAL();
	return NULL;
}

/*
 * Called by the AMF on a CSI set.
 */


static void dispatch_amf(struct pollfd* pfd, void *ref)
{
	ENTER_MWSA_GENERAL();
	SaAisErrorT rc;
	DEBUG_MWSA_GENERAL("dispatch_amf called...");
	rc = saAmfDispatch(amf_handle, SA_DISPATCH_ALL);
	if (SA_AIS_OK != rc)
		err_quit("saAmfDispatch FAILED %u", rc);
	LEAVE_MWSA_GENERAL();
}


/* SDP 1694 - support MAF SPI */
static MafMwSpiHaModeT maf_com_ha_state = MafMwSpiHaModeUnassigned;
static MafMwSpiHaModeT maf_new_com_ha_state;

static void maf_amf_csi_set_callback(
	SaInvocationT inv, const SaNameT *comp_name,
	SaAmfHAStateT ha_state, SaAmfCSIDescriptorT csi_desc)
{
	ENTER_MWSA_GENERAL();
	MafMwSpiHaReasonT reason = MafMwSpiHaReasonSwitchover;
	MafMwSpiAvailabilityInvocationT invocation;
	DEBUG_MWSA_GENERAL("maf_amf_csi_set_callback called (hastate=%u)", ha_state);

	switch (ha_state) {
	case SA_AMF_HA_ACTIVE: {
		DEBUG_MWSA_GENERAL("maf_amf_csi_set_callback for ACTIVE");
		if(strcmp("lm", _CC_NAME)) //This block is applicable for COM only.
		{
			DEBUG_MWSA_GENERAL("Executing Model consumer ready: %s", modelConsumerReadycmd);
			if(processOpen(modelConsumerReadycmd)!=0)
			{
				ERR_MWSA_GENERAL("Model consumer ready command failed during switchover");
			}

			if (comsa_sshd_control)
			{
				DEBUG_MWSA_GENERAL("maf_amf_csi_set_callback: SSHD controlled by COM SA (but doing nothing here)");
			}
			else
			{
				DEBUG_MWSA_GENERAL("maf_amf_csi_set_callback: SSHD controlled by AMF");
			}
		}
		SaAmfCSITransitionDescriptorT transition =
			csi_desc.csiStateDescriptor.activeDescriptor.transitionDescriptor;
		maf_new_com_ha_state = MafMwSpiHaModeActive;
		if (transition == SA_AMF_CSI_NEW_ASSIGN)
			reason = MafMwSpiHaReasonInit;
		else if (transition == SA_AMF_CSI_NOT_QUIESCED)
			reason = MafMwSpiHaReasonFail;
		// If the other COM process has crashed this will work because then IMM automatically calls saImmOiFinalize()

		 // SDP1694 -support MAF SPI
		// Portal is used by OIProxy for Transactions, so must be the same type as TransactionResource
		// Interface which is set by comSAOamComponentInitialize() in mafLCMinit().
		if(((MafReturnT)ObjImp_init_imm(true)) == MafFailure)
		{
			ERR_MWSA_GENERAL("ObjImp_init_imm(): Object Implementer initialization failed, preparing for the termination of COM");
			maf_acCallbacks->prepareTermination((MafMwSpiAvailabilityInvocationT)
					maf_new_event(EVENT_prepareTerminationResponse, inv, 0));
		}
		//This block is applicable for COM only. For LM always the logs are logged into saf logs
		if(strcmp("lm", _CC_NAME))
		{
			if (alertalarm!=1)
			{
				alertalarm=1;
				DEBUG_MWSA_GENERAL("maf_amf_csi_set_callback(SA_AMF_HA_ACTIVE) alertalarm = %d", alertalarm);
				if((MafReturnT) AlarmAndAlertLogServiceOpen() != MafOk)
				{
					WARN_MWSA_GENERAL("maf_amf_csi_set_callback failed to open AlarmAndAlertLogServiceOpen()");
				}
				if((MafReturnT) cmdLogServiceOpen() != MafOk)
				{
					WARN_MWSA_GENERAL("maf_amf_csi_set_callback failed to open CmdLogServiceOpen()");
				}
				if((MafReturnT) secLogServiceOpen() != MafOk)
				{
					WARN_MWSA_GENERAL("maf_amf_csi_set_callback failed to open secLogServiceOpen()");
				}
			}
		}
		break;
	}
	case SA_AMF_HA_STANDBY:
		DEBUG_MWSA_GENERAL("maf_amf_csi_set_callback for STANDBY");
		if(0 == strcmp("com", _CC_NAME)) //This block is applicable for COM only.
		{
			if (comsa_sshd_control)
			{
				char* sudo = getenv("SUDO_CMD");
				unsigned int sudoLen = 0;
				char* updateSshdScript = NULL;
				char* reloadSshdScript = NULL;
				if(NULL != sudo)
				{
					sudoLen = strlen(sudo);
					updateSshdScript = (char *)malloc(sudoLen + strlen(updateScript) + 1);
					strcpy(updateSshdScript,sudo);
					strcat(updateSshdScript,updateScript);
				} else {
					updateSshdScript = (char *)malloc(strlen(updateScript) + 1);
					strcpy(updateSshdScript,updateScript);
				}


				DEBUG_MWSA_GENERAL("maf_amf_csi_set_callback: SSHD controlled by COM SA ");
				if(processOpen(updateSshdScript) != 0)
				{
					ERR_MWSA_GENERAL("amf_csi_set_callback failed processOpen for Standby: Could not update sshd configuration file!!!");
				}
				free(updateSshdScript);

				if(NULL != sudo)
				{
					reloadSshdScript = (char *)malloc(sudoLen + strlen(reloadScript) + 1);
					strcpy(reloadSshdScript,sudo);
					strcat(reloadSshdScript,reloadScript);
				} else {
					reloadSshdScript = (char *)malloc(strlen(reloadScript) + 1);
					strcpy(reloadSshdScript,reloadScript);
				}

				if(processOpen(reloadSshdScript) != 0)
				{
					ERR_MWSA_GENERAL("amf_csi_set_callback failed processOpen for Standby: Could not reload sshd process!!!");
				}
				free(reloadSshdScript);
			}
			else
			{
				DEBUG_MWSA_GENERAL("maf_amf_csi_set_callback: SSHD controlled by AMF");
			}
			removeSecretAttrFile();
		}
		maf_new_com_ha_state = MafMwSpiHaModeStandby;
		reason = MafMwSpiHaReasonInit;

		break;
	case SA_AMF_HA_QUIESCED:
		DEBUG_MWSA_GENERAL("maf_amf_csi_set_callback for QUIESCED");
		if (comsa_sshd_control)
		{
			DEBUG_MWSA_GENERAL("maf_amf_csi_set_callback: SSHD controlled by COM SA (but doing nothing here)");
		}
		else
		{
			DEBUG_MWSA_GENERAL("maf_amf_csi_set_callback: SSHD controlled by AMF");
		}
		// Addition to enable the registration of the Model-Type IMM-I-FileM
		removeFileMConsumer();

		if(0 == strcmp("com", _CC_NAME)) { //This block is applicable for COM only.
			removeSecretAttrFile();
		}

		maf_new_com_ha_state = MafMwSpiHaModeUnassigned;
		break;
	case SA_AMF_HA_QUIESCING:

		if(0 == strcmp("com", _CC_NAME)) { //This block is applicable for COM only.
			removeSecretAttrFile();
		}

		maf_new_com_ha_state = MafMwSpiHaModeUnassigned;
		break;
	default:
		assert(0);
	}

	if (maf_acCallbacks == NULL || maf_com_ha_state == maf_new_com_ha_state) {

		/* It's illegal to go to the same state in COM, so
		 * just answer opensaf. */

		SaAisErrorT rc;
		LOG_MWSA_GENERAL("Transition to same hastate (%u)", maf_com_ha_state);
		saAmfResponse_wrapper(amf_handle, inv, SA_AIS_OK);
		if (ha_state == SA_AMF_HA_QUIESCING) {
			rc = saAmfCSIQuiescingComplete(
				amf_handle, inv, SA_AIS_OK);
			if (SA_AIS_OK != rc) err_quit(
				"saAmfCSIQuiescingComplete FAILED - %u", rc);
		}
		LEAVE_MWSA_GENERAL();
		return;
	}

	if (ha_state == SA_AMF_HA_QUIESCING) {
		/* For QUIESCING we send the amfResponse immediately
		 * and do a saAmfCSIQuiescingComplete() on call-back
		 * from COM. */
		DEBUG_MWSA_GENERAL("QUIESCING");
		saAmfResponse_wrapper(amf_handle, inv, SA_AIS_OK);
		invocation = (MafMwSpiAvailabilityInvocationT)
			maf_new_event(EVENT_haModeAssumed, inv, 1);

	} else {
		invocation = (MafMwSpiAvailabilityInvocationT)
			maf_new_event(EVENT_haModeAssumed, inv, 0);
	}

	DEBUG_MWSA_GENERAL("setHaMode %u (%u)", maf_new_com_ha_state, maf_com_ha_state);
	maf_acCallbacks->setHaMode(invocation, maf_new_com_ha_state, reason);
	LEAVE_MWSA_GENERAL();
}

static void maf_coremw_haModeAssumed_event(struct maf_CoremwEvent* event)
{
	ENTER_MWSA_GENERAL();
	SaAisErrorT response;

	DEBUG_MWSA_GENERAL("coremw_haModeAssumed_event called");
	if (event->error == MafOk) {
		response = SA_AIS_OK;
		maf_com_ha_state = maf_new_com_ha_state;
		if(maf_new_com_ha_state == MafMwSpiHaModeActive)
		{
			pthread_attr_t ntf_attr;
			if (pthread_attr_init(&ntf_attr) != 0) {
				ERR_MWSA_GENERAL("Failed ntf_attr_init ntf_thread");
				return;
			}
			if (pthread_create(&ntf_pthread, &ntf_attr, ntf_thread, NULL) != 0) {
				ERR_MWSA_GENERAL("Failed to start ntf_thread");
				(void)pthread_attr_destroy(&ntf_attr);
				return;
			}
			ntf_pthread_isrunning = 1;
			LOG_MWSA_GENERAL("ntf_pthread_isrunning = %d", ntf_pthread_isrunning);
		}
	} else {
		WARN_MWSA_GENERAL("haModeAssumed_event, error=%d",(int)event->error);
		response = SA_AIS_ERR_FAILED_OPERATION;
	}

	if (event->flags == 1) {
		/* For QUIESCING we have sent the amfResponse already.
		 * So do a saAmfCSIQuiescingComplete(). */
		DEBUG_MWSA_GENERAL("Call saAmfCSIQuiescingComplete (%u)", response);
		SaAisErrorT rc = saAmfCSIQuiescingComplete(
			amf_handle, event->saf_invokation, response);
		if (SA_AIS_OK != rc) err_quit(
			"saAmfCSIQuiescingComplete FAILED - %u", rc);

	} else {

		if((maf_new_com_ha_state == MafMwSpiHaModeActive || maf_new_com_ha_state == MafMwSpiHaModeStandby)
				&& (strcmp(_CC_NAME, "lm")))
		{
			handshakeToComsaMimTool();
		}
		saAmfResponse_wrapper(
			amf_handle, event->saf_invokation, response);
	}
	LEAVE_MWSA_GENERAL();
}


static void maf_amf_csi_remove_callback(
	SaInvocationT inv, const SaNameT *comp_name,
	const SaNameT  *csi_name, SaAmfCSIFlagsT csi_flags)
{
	ENTER_MWSA_GENERAL();
	DEBUG_MWSA_GENERAL("amf_csi_remove_callback called");
	if (maf_com_ha_state == MafMwSpiHaModeUnassigned) {
		saAmfResponse_wrapper(amf_handle, inv, SA_AIS_OK);
	} else {

		/* An active or standby load is brutally removed. We
		 * handle this exectly as a SA_AMF_HA_QUIESCED load *
		 * assignment. */
		LOG_MWSA_GENERAL("Removing active or standby load");
		SaAmfCSIDescriptorT csi_desc = {0}; /* (will not be used) */
		maf_amf_csi_set_callback(
			inv, comp_name,	SA_AMF_HA_QUIESCED, csi_desc);
	}
	LEAVE_MWSA_GENERAL();
}

static void maf_amf_healthcheck_callback(
	SaInvocationT inv, const SaNameT *comp_name,
	SaAmfHealthcheckKeyT *health_check_key)
{
	ENTER_MWSA_GENERAL();
	DEBUG_MWSA_GENERAL("maf_amf_healthcheck_callback called");
	if (maf_acCallbacks == NULL) {
		saAmfResponse_wrapper(amf_handle, inv, SA_AIS_OK);
		LEAVE_MWSA_GENERAL();
		return;
	}

	maf_acCallbacks->healthCheck((MafMwSpiAvailabilityInvocationT)
		maf_new_event(EVENT_healthCheckReport, inv, 0));
	LEAVE_MWSA_GENERAL();
}

static void maf_coremw_healthCheckReport_event(struct maf_CoremwEvent* event)
{
	ENTER_MWSA_GENERAL();
	SaAisErrorT response = (event->error == MafOk) ?
		SA_AIS_OK : SA_AIS_ERR_FAILED_OPERATION;
	DEBUG_MWSA_GENERAL("coremw_healthCheckReport_event called");
	if (event->flags == 0) {
		saAmfResponse_wrapper(
			amf_handle, event->saf_invokation, response);
	} else {
		LOG_MWSA_GENERAL("A spontaneous healthCheckReport from COM");
		SaAmfRecommendedRecoveryT recover =
			(event->recommendedRecovery == MafMwSpiRecommendedRecoveryFailover) ?
			SA_AMF_COMPONENT_FAILOVER : SA_AMF_COMPONENT_RESTART;
		SaAisErrorT rc = saAmfComponentErrorReport(
			amf_handle,
			&gl_comp_name,
			0,
			recover,
			SA_NTF_IDENTIFIER_UNUSED);
		if (rc != SA_AIS_OK) err_quit(
			"saAmfComponentErrorReport failed %u", rc);
	}
	DEBUG_MWSA_GENERAL("coremw_healthCheckReport_event end");
	LEAVE_MWSA_GENERAL();
}

static void maf_amf_comp_terminate_callback(
	SaInvocationT inv, const SaNameT *comp_name)
{
	ENTER_MWSA_GENERAL();
	DEBUG_MWSA_GENERAL("amf_comp_terminate_callback called");

	if (maf_acCallbacks == NULL) {
		saAmfResponse_wrapper(amf_handle, inv, SA_AIS_OK);
		LEAVE_MWSA_GENERAL();
		err_quit("Forced termination");
	}

	maf_acCallbacks->prepareTermination((MafMwSpiAvailabilityInvocationT)
		maf_new_event(EVENT_prepareTerminationResponse, inv, 0));
	LEAVE_MWSA_GENERAL();
}

static void maf_coremw_prepareTerminationResponse_event(struct maf_CoremwEvent* event)
{
	ENTER_MWSA_GENERAL();
	SaAisErrorT response = (event->error == MafOk) ?
		SA_AIS_OK : SA_AIS_ERR_FAILED_OPERATION;
	saAmfResponse_wrapper(amf_handle, event->saf_invokation, response);
	assert(maf_acCallbacks != NULL);
	maf_acCallbacks->terminateProcess();
	LEAVE_MWSA_GENERAL();
}


static void maf_handle_events(struct pollfd* pfd, void *ref)
{
	ENTER_MWSA_GENERAL();
	struct maf_CoremwEvent* event = maf_event_getq();
	while (event != NULL) {
		struct maf_CoremwEvent* event_tobedeleted;
		switch (event->event_type) {
		case EVENT_Terminate:
			stop_thread = 1;
			break;
		case EVENT_haModeAssumed:
			maf_coremw_haModeAssumed_event(event);
			break;
		case EVENT_healthCheckReport:
			maf_coremw_healthCheckReport_event(event);
			break;
		case EVENT_prepareTerminationResponse:
			maf_coremw_prepareTerminationResponse_event(event);
			break;
		default:
			assert(0);
		}
		event_tobedeleted = event;
		event = event->next;
		free(event_tobedeleted);
	}
	LEAVE_MWSA_GENERAL();
}


static void* maf_amf_thread(void* arg)
{
	ENTER_MWSA_GENERAL();
	SaAmfCallbacksT amf_callbacks;
	SaAisErrorT rc,rc1;
	SaSelectionObjectT amf_sel_obj;
	struct pollfd fds[2];

	LOG_MWSA_GENERAL("maf_amf_thread running ...");

	memset(&amf_callbacks, 0, sizeof(SaAmfCallbacksT));
	amf_callbacks.saAmfCSISetCallback = maf_amf_csi_set_callback;
	amf_callbacks.saAmfCSIRemoveCallback = maf_amf_csi_remove_callback;
	amf_callbacks.saAmfHealthcheckCallback = maf_amf_healthcheck_callback;
	amf_callbacks.saAmfComponentTerminateCallback =
		maf_amf_comp_terminate_callback;

	SAF_API_INIT_CALL(saAmfInitialize(&amf_handle, &amf_callbacks, &ver));
	if (SA_AIS_OK != rc) {
		rc1 = rc;
		SAF_API_INIT_CALL(saAmfFinalize(amf_handle));
		err_quit("saAmfInitialize FAILED %u", rc1);
	}

	SAF_API_CALL(saAmfSelectionObjectGet(amf_handle, &amf_sel_obj));
	if (SA_AIS_OK != rc) {
		rc1 = rc;
		SAF_API_INIT_CALL(saAmfFinalize(amf_handle));
		err_quit("saAmfSelectionObjectGet FAILED %u", rc1);
	}

	SAF_API_CALL(saAmfComponentNameGet(amf_handle, &gl_comp_name));
	if (SA_AIS_OK != rc) {
		rc1 = rc;
		SAF_API_INIT_CALL(saAmfFinalize(amf_handle));
		err_quit("saAmfComponentNameGet FAILED %u", rc1);
	}

	SAF_API_CALL(saAmfComponentRegister(amf_handle, &gl_comp_name, 0));
	if (SA_AIS_OK != rc) {
		rc1 = rc;
		SAF_API_INIT_CALL(saAmfFinalize(amf_handle));
		err_quit("saAmfComponentRegister FAILED %u", rc1);
	}

	if (strcmp(_CC_NAME, "lm"))
	{
		SAF_API_CALL(saAmfHealthcheckStart(
				amf_handle, &gl_comp_name, &gl_healthcheck_key_comsa,
				SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_RESTART));
	}
	else
	{
		SAF_API_CALL(saAmfHealthcheckStart(
				amf_handle, &gl_comp_name, &gl_healthcheck_key_lmsa,
				SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_RESTART));
	}

	if ( SA_AIS_OK != rc ) {
		rc1 = rc;
		SAF_API_INIT_CALL(saAmfFinalize(amf_handle));
		err_quit("saAmfHealthcheckStart FAILED - %u", rc1);
	}

	fds[0].fd = amf_sel_obj;
	fds[0].events = POLLIN;
	fds[1].fd = event_pipe[0];
	fds[1].events = POLLIN;
	poll_setcallback(comSASThandle, dispatch_amf, fds[0], NULL);
	poll_setcallback(comSASThandle, maf_handle_events, fds[1], NULL);

	while (!stop_thread) {
		if (poll_execute(comSASThandle) != 0) {
			SAF_API_INIT_CALL(saAmfFinalize(amf_handle));
			err_quit("Drop out of the poll-loop");
		}
	}

	if (strcmp(_CC_NAME, "lm"))
	{
		//FIXME: check if this is appropriate for even COM!! else remove for COM too altogether.
		SAF_API_INIT_CALL(saAmfFinalize(amf_handle));
		if (SA_AIS_OK != rc)
			err_quit("saAmfFinalize FAILED %u", rc);
	}

	LEAVE_MWSA_GENERAL();
	return NULL;
}




/* ----------------------------------------------------------------------
 * Help functions;
 */

static void coremw_vlog(int priority, char const* fmt, va_list ap)
{
	char buffer[TRACE_BUFFER_SIZE];
	if(_CC_NAME_SA == NULL)
	{
		char* tmp = convertToUpperCase((const char*)CC_NAME);
		asprintf(&_CC_NAME_SA, "%s%s", tmp, "_SA");
		if (tmp)
		{
			free(tmp);
		}
	}

	int len = strlen((const char*)_CC_NAME_SA);
	strcpy(buffer, (const char*)_CC_NAME_SA);
	buffer[len] = ' ';
	len++;
	vsnprintf(buffer+len, sizeof(buffer)-len, fmt, ap);
	syslog(LOG_USER*8 + priority, "%s", buffer);
}

void err_quit(char const* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	coremw_vlog(LOG_ERR, fmt, ap);
	va_end(ap);
#if 0
	if (acCallbacks != NULL) {
		acCallbacks->terminateProcess(); /* (returns!) */
		sleep(3600);
	}
#endif
	/* Do not use exit() here since it will run destructors in COM! */
	//abort();
	_exit(1);
}

static void
saAmfResponse_wrapper(
	SaAmfHandleT amfHandle,
	SaInvocationT invocation,
	SaAisErrorT error)
{
	ENTER_MWSA_GENERAL();
	SaAisErrorT rc;
	SAF_API_CALL(saAmfResponse(amfHandle, invocation, error));
	if (rc != SA_AIS_OK)
		err_quit("saAmfResponse failed %u", rc);
	LEAVE_MWSA_GENERAL();
}

char const* coremw_xmlnode_get_attribute(xmlNode* n, char const* attr_name)
{
	ENTER_MWSA_GENERAL();
	xmlAttr *a;
	for (a = n->properties; a != NULL; a = a->next) {
		if (xstrcmp(a->name, attr_name) == 0) {
			return (char const*)a->children->content;
		}
	}
	LEAVE_MWSA_GENERAL();
	return NULL;
}

char const* coremw_xmlnode_contents(xmlNode* cur) {
	ENTER_MWSA_GENERAL();
	if (cur == NULL) return NULL;
	if (cur->type == XML_ELEMENT_NODE) {
		cur = cur->children;
		if (cur == NULL) return NULL;
	}
	if (cur->type == XML_TEXT_NODE) {
		LEAVE_MWSA_GENERAL();
		return (char const*) XML_GET_CONTENT(cur);
	}
	LEAVE_MWSA_GENERAL();
	return NULL;
 }

xmlNode* coremw_find_config_item(
	char const* component_name, char const* node_name)
{
	ENTER_MWSA_GENERAL();
	xmlNode* root;
	xmlNode* comp;
	xmlNode* n;
	if (doc == NULL) return NULL;
	root = xmlDocGetRootElement(doc);
	if (root == NULL) return NULL;
	for (comp = root->children; comp != NULL; comp = comp->next) {
		if (isElementName(comp, "component")) {
			DEBUG_MWSA_GENERAL("Found a component");
			for (n = comp->children; n != NULL; n = n->next) {
				if (isElementName(n, "name")) {
					char const* name =
						coremw_xmlnode_contents(n);
					DEBUG_MWSA_GENERAL("Found component name [%s]",
							name==NULL?"(null)":name);
					if (name != NULL
						&& strcmp(component_name, name)==0)
						break;
				}
			}
			if (n != NULL) break;
		}
	}
	if (comp == NULL) {
		WARN_MWSA_GENERAL("No configuration for component [%s]",
						component_name);
		LEAVE_MWSA_GENERAL();
		return NULL;
	}
	for (n = comp->children; n != NULL; n = n->next) {
		if (isElementName(n, node_name)) {
			DEBUG_MWSA_GENERAL("Config item [%s] found in [%s]",
								node_name, component_name);
			LEAVE_MWSA_GENERAL();
			return n;
		}
	}
	WARN_MWSA_GENERAL("Config item [%s] not found in component [%s]",
						node_name, component_name);
	LEAVE_MWSA_GENERAL();
	return NULL;
}


/* SDP1694 - support MAF SPI */
MafReturnT maf_coremw_rc(SaAisErrorT rc)
{
	ENTER_MWSA_GENERAL();
	LEAVE_MWSA_GENERAL();
	switch (rc) {
	case SA_AIS_OK:
		return MafOk;
	case SA_AIS_ERR_LIBRARY:
		return MafFailure;
	case SA_AIS_ERR_VERSION:
		return MafFailure;
	case SA_AIS_ERR_INIT:
		return MafFailure;
	case SA_AIS_ERR_TIMEOUT:
		return MafTimeOut;
	case SA_AIS_ERR_TRY_AGAIN:
		return MafTryAgain;
	case SA_AIS_ERR_INVALID_PARAM:
		return MafFailure;
	case SA_AIS_ERR_NO_MEMORY:
		return MafNoResources;
	case SA_AIS_ERR_BAD_HANDLE:
		return MafFailure;
	case SA_AIS_ERR_BUSY:
		return MafFailure;
	case SA_AIS_ERR_ACCESS:
		return MafFailure;
	case SA_AIS_ERR_NOT_EXIST:
		return MafNotExist;
	case SA_AIS_ERR_NAME_TOO_LONG:
		return MafFailure;
	case SA_AIS_ERR_EXIST:
		return MafAlreadyExist;
	case SA_AIS_ERR_NO_SPACE:
		return MafNoResources;
	case SA_AIS_ERR_INTERRUPT:
		return MafFailure;
	case SA_AIS_ERR_NAME_NOT_FOUND:
		return MafNotExist;
	case SA_AIS_ERR_NO_RESOURCES:
		return MafFailure;
	case SA_AIS_ERR_NOT_SUPPORTED:
		return MafFailure;
	case SA_AIS_ERR_BAD_OPERATION:
		return MafFailure;
	case SA_AIS_ERR_FAILED_OPERATION:
		return MafFailure;
	case SA_AIS_ERR_MESSAGE_ERROR:
		return MafFailure;
	case SA_AIS_ERR_QUEUE_FULL:
		return MafNoResources;
	case SA_AIS_ERR_QUEUE_NOT_AVAILABLE:
		return MafFailure;
	case SA_AIS_ERR_BAD_FLAGS:
		return MafFailure;
	case SA_AIS_ERR_TOO_BIG:
		return MafFailure;
	case SA_AIS_ERR_NO_SECTIONS:
		return MafFailure;
	case SA_AIS_ERR_NO_OP:
		return MafFailure;
	case SA_AIS_ERR_REPAIR_PENDING:
		return MafFailure;
	case SA_AIS_ERR_NO_BINDINGS:
		return MafFailure;
	case SA_AIS_ERR_UNAVAILABLE:
		return MafFailure;
	case SA_AIS_ERR_CAMPAIGN_ERROR_DETECTED:
		return MafFailure;
	case SA_AIS_ERR_CAMPAIGN_PROC_FAILED:
		return MafFailure;
	case SA_AIS_ERR_CAMPAIGN_CANCELED:
		return MafFailure;
	case SA_AIS_ERR_CAMPAIGN_FAILED:
		return MafFailure;
	case SA_AIS_ERR_CAMPAIGN_SUSPENDED:
		return MafFailure;
	case SA_AIS_ERR_CAMPAIGN_SUSPENDING:
		return MafFailure;
	case SA_AIS_ERR_ACCESS_DENIED:
		return MafFailure;
	case SA_AIS_ERR_NOT_READY:
		return MafFailure;
	case SA_AIS_ERR_DEPLOYMENT:
		return MafFailure;
	}
	return MafFailure;
}

unsigned long long getMillisecondsSinceEpochUnixTime(){
	ENTER_COMSA();
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long long millisecondsSinceEpoch = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
	LEAVE_COMSA();
	return millisecondsSinceEpoch;
}


MafReturnT InitAmfSshdControl()
{
	DEBUG_COMSA("InitAmfSshdControl(): SSHD control flag file: %s", sshdFileName);
	char cmd2[MAX_PATH_DATA_LENGTH];

	int scriptResult = processOpen("is-com-sshd-flag-enabled.sh");
	if (0 == scriptResult)
	{
		DEBUG_COMSA("InitAmfSshdControl(): 'sshdManagement' in 'libcom_sshd_manager.cfg' is TRUE");
		comsa_sshd_control = 0;    // AMF will control SSHD
		sprintf (cmd2, "rm -f %s", sshdByComsa);
		int scriptResult1 = processOpen(cmd2);
		if (0 == scriptResult1)
		{
			DEBUG_COMSA("InitAmfSshdControl(): command '%s' was successful", cmd2);
		}
		else
		{
			ERR_COMSA("InitAmfSshdControl(): ERROR: command '%s' failed", cmd2);
			return MafFailure;
		}

		sprintf (cmd2, "touch %s", sshdFileName);
		scriptResult1 = processOpen(cmd2);
		if (0 == scriptResult1)
		{
			DEBUG_COMSA("InitAmfSshdControl(): command '%s' was successful", cmd2);
		}
		else
		{
			ERR_COMSA("InitAmfSshdControl(): ERROR: command '%s' failed", cmd2);
			return MafFailure;
		}
	}
	else
	{
		DEBUG_COMSA("InitAmfSshdControl(): 'sshdManagement' in 'libcom_sshd_manager.cfg' is FALSE or there was an error executing the 'is-com-sshd-flag-enabled.sh' script");
		comsa_sshd_control = 1;    // COM SA will control SSHD
		sprintf (cmd2, "rm -f %s", sshdFileName);
		int scriptResult1 = processOpen(cmd2);
		if (0 == scriptResult1)
		{
			DEBUG_COMSA("InitAmfSshdControl(): command '%s' was successful", cmd2);
		}
		else
		{
			ERR_COMSA("InitAmfSshdControl(): ERROR: command '%s' failed", cmd2);
			return MafFailure;
		}

		/* This second 'global environment boolean variable' (a file present or not)
		 * may seem redundand but is needed for the transition period when switching from
		 * COM SA controlled SSHD to AMF controlled SSHD while COM SA is restarting.
		 * During this period the AMF is still calling the wrapper script with 'status' and if we pass this to the
		 * com_sshd.sh it will return 'not running' to AMF. This will cause AMF to decide that COM SA failed and
		 * will try to restart it. This results in cyclic cluster reboot.
		 */
		sprintf (cmd2, "touch %s", sshdByComsa);
		scriptResult1 = processOpen(cmd2);
		if (0 == scriptResult1)
		{
			DEBUG_COMSA("InitAmfSshdControl(): command '%s' was successful", cmd2);
		}
		else
		{
			ERR_COMSA("InitAmfSshdControl(): ERROR: command '%s' failed", cmd2);
			return MafFailure;
		}
	}
	return MafOk;
}


/* This function supports the handshake with the 'comsa-mim-tool com_restart' script command in order to hold the script
 * not to return from the command until the COM SA component is fully started and provides service.
 * It is called from the AMF callback function
 */
void handshakeToComsaMimTool()
{
	getClearStorage(pathToStorageDirectory);

	DEBUG_COMSA("handshakeToComsaMimTool(): control flag file: %s/%s/%s", pathToStorageDirectory, comsaProductDir, mimToolFileName);
	char cmd[MAX_PATH_DATA_LENGTH];

	sprintf (cmd, "rm -rf %s/%s/%s", pathToStorageDirectory, comsaProductDir, mimToolFileName);

	// wait for up to 20 sec
	unsigned int nTries = 20;
	int scriptResult = processOpen(cmd);

	while(1 == scriptResult && nTries > 0)
	{
		usleep(1000);
		scriptResult = processOpen(cmd);
		nTries--;
	}

	if (0 == scriptResult)
	{
		DEBUG_COMSA("(): command '%s' was successful", cmd);
	}
	else
	{
		ERR_COMSA("(): ERROR: command '%s' failed", cmd);
	}
}

void removeSecretAttrFile()
{
	if (0 == remove(REENCRYPTOR_MODEL_DATA_FILE_PATH)) {

	}
}
