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
*   Author: uaberin
*
*   Date:   2010-06-17
*
*   This file implement forwarding of notifications of type alarm and
*   security alarm from a SAF notification service. The notifications are
*   converted to a suitable format for COM.
*
*
* Reviewed: uablrek 2010-06-13
*
* Modify: efaiami 2011-03-09 for log and trace function
*
* Modify: eaparob 2012-05-24  added new functionality to ComNtfServiceOpen and ComNtfServiceClose - the first version of implementation (from emilzor)
* Modify: eaparob 2012-05-24  updated ComNtfServiceOpen and ComNtfServiceClose to use MAF SPI
* Modify: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
* Modify: eaparob 2013-01-28  Implemented subscription for CM notifications and added producer event for CM events
* Modify: uabjoy  2013-09-13  Correcting trouble report HR74015
* Modify: xanhdao 2013-10-23  MR24146  Support for Floation point numbers
* Modify: xadaleg 2014-01-03  MR-29443 Align to ECIM FM 4.0
* Modify: uabjoy  2014-03-24  Adopt to Trace CC.
* Modify: xdonngu 2014-05-22  HS62388: Fixed segment fault: null argument in string functions.
* Modify: xadaleg 2014-08-02  MR35347 - increase DN length
* Modify: xvintra 2014-12-24  HT30029 - Add mutex lock in readOldAlarms function
* Modify: xvintra 2014-12-29  TR HT18076 - Merge additional Text for PM Alarms
* Modify: xvintra 2014-12-24  HT30029 - add condition to clean up nFilter_v4
* Modify: xvintra 2015-04-01  MR38690 - Support for UUID
* Modify: xadaleg 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
*
*******************************************************************************/

#ifdef UNIT_TEST
#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#endif

#include "ComSANtf.h"

static bool is_ntf_stop_component = false;
static unsigned int nRetries;
static pthread_t read_pthread;
extern pthread_t ntf_pthread;
extern int ntf_stop_thread;
extern int ntf_pthread_isrunning;
extern int ntf_event_pipe[2];

#define OAM_SA_NTF_RETRY(F)do {\
	nRetries = 0;\
	rc = F;\
	while(rc == SA_AIS_ERR_TRY_AGAIN && nRetries < OAM_SA_NTF_MAX_RETRIES){\
		usleep(OAM_SA_NTF_RETRY_SLEEP);\
		rc = F;\
		nRetries++;\
	}} while (0)

static SaNtfHandleT ntfHandle;
bool addFilter_setup = false;
SaNtfSubscriptionIdT COMSA_NTF_SUBSCRIPTION_ID = 44;

static pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t notify_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t cm_notification_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t alarm_queue_lock = PTHREAD_MUTEX_INITIALIZER;

static const char* PM_THRESHOLD_FMALARMTYPE_DN = "fmAlarmTypeId=PMThresholdCrossedOrReached,fmAlarmModelId=PM,fmId=1";

/* Release code, major version, minor version */
static const SaVersionT ntfApiVersion = { immReleaseCode, immBaseMajorVer, immBaseMinorVer };

// struct telling if the Cache needs to be informed about the callback
typedef struct {
	SaImmOiCcbIdT ccbId;
	bool ccbLast;
}cacheInformation;

// each UUID contains 36 characters(UUID has the format "123e4567-e89b-12d3-a456-426655440000" for example)
#define UUID_LENGTH 36
#define UUID_ADDTEXT_HEADER ";uuid:"
#define ERICSSON_VENDOR_ID 193

// Hard code reserved SaNtfElementIdT. Should define it later
const SaNtfElementIdT UUID = 99;
extern UuidCfg uuid_cfg_flag;

// Flags to inform saNtfNotificationCallback4 about old alarm processing.
static bool readOldAlarm = false;
static bool lastNotificationMatched = false;
static bool readOldAlarmDone = false;// inform saNtfNotification call back about reader theread complete.

static unsigned int sReaderMaxRetries = 0;
// Struct for Queuing alarms from CMW while processing old alarms.
typedef struct alarmObject {
	MafOamSpiNotificationFmStruct_4T *alarm;
	struct alarmObject *next;
} AlarmObjectT;

// Pointers to AlarmQueue for storing and accessing stored alarms
static AlarmObjectT *alarmQueue = NULL;
static AlarmObjectT *alarmQueueFront = NULL;

static uint64_t callbackLastCmwNotificationID = 0; // Reading the callbacknotificationid when the readOldAlarm is inactive
static uint64_t readerLastNotificationID = 0; // Data to sync the reader thread FM notification

// Functions to process the queue and send the notification to MAF
uint64_t getNotificationId(const SaNtfNotificationsT *notification);
static void sendQueuedAlarms();
void sendSaNtfNotification(MafOamSpiNotificationFmStruct_4T *comNot);

char* getUUIDFromAdditionalInfo(const SaNtfNotificationHeaderT *nh, const SaNtfNotificationHandleT notificationHandle);

static SaAisErrorT readOldAlarms(SaUint32T numDiscarded, const SaNtfIdentifierT *discardedIds, const bool isDiscarded);
static void cmNotificationCallback(const SaNtfNotificationsT *notification);
static SaAisErrorT subscribeForNotifications(SaNtfSubscriptionIdT subscriptionId);
static SaAisErrorT subscribeFor_CM_Notifications(void);
static SaAisErrorT unsubscribeFor_CM_Notifications(void);
void freeConvertCMResources(char *immRdn);

#ifndef UNIT_TEST
static void dispatch_ntf(struct pollfd* pfd, void *ref)
#else
void dispatch_ntf(struct pollfd* pfd, void *ref)
#endif
{
	ENTER_OAMSA_CMEVENT();
	SaAisErrorT rc;
	DEBUG_OAMSA_CMEVENT("dispatch_ntf called...");
	rc = saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
	if (SA_AIS_ERR_BAD_HANDLE == rc)
	{
		WARN_OAMSA_CMEVENT("Warning: saNtfDispatch - bad handle");
	}
	else if (SA_AIS_OK != rc)
	{
		err_quit("saNtfDispatch FAILED %u", rc);
	}
	LEAVE_OAMSA_CMEVENT();
}

/* forward declarations */
static MafReturnT doneWithValue(const char *eventType, void * value);

static MafOamSpiEventProducerHandleT producer_handle;
static MafOamSpiEventRouter_1T *event_router = NULL;

#ifndef UNIT_TEST
static MafReturnT comsaNotify(MafOamSpiEventConsumerHandleT handle, const char * eventType, MafNameValuePairT **filter, void * value);
#else
MafReturnT comsaNotify(MafOamSpiEventConsumerHandleT handle, const char * eventType, MafNameValuePairT **filter, void * value);
#endif

static MafOamSpiEventConsumer_1T consumer_if = {
	.notify = &comsaNotify
};

static MafOamSpiEventConsumerHandleT consumer_handle;
static MafOamSpiEventRouter_1T *event_router_MAF = NULL;
static MafNameValuePairT *nullNameValuePair[1] = { NULL };

/* FM v4 */

// addFilter_V4 is called by the event router whenever the producer needs to be updated about what events are subscribed for
static MafReturnT addFilter_V4(
	MafOamSpiEventConsumerHandleT consumerHandle, // [in] identifier of the consumer
	const char * eventType, // [in] the type of event.
	MafNameValuePairT ** filter); // [in] the filter specifies in more detail what it is the consumer will be notified for.

// removeFilter_V4 is called by the event service whenever the producer needs to be updated about events that are no longer subscribed for
static MafReturnT removeFilter_V4(
	MafOamSpiEventConsumerHandleT consumerHandle, // [in] identifier of the consumer
	const char * eventType, // [in] the type of event.
	MafNameValuePairT ** filter); // [in] the filter instance that was previously added.

// clearAll can be called by the Event Router to restart from a known state. When called the producer will discard all filters.
#ifndef UNIT_TEST
static MafReturnT clearAll_V4( void );
#else
MafReturnT clearAll_V4( void );
#endif

// doneWithValue, Event router calls this function to notify the Producer that the
// the value is not needed anymore. The producer will know how to delete the value.
static MafReturnT doneWithValue_V4(const char *eventType, void * value);

// Clean the FM 4 value list
#ifndef UNIT_TEST
static void cleanVlist4( void );
#else
void cleanVlist4( void );
#endif
// Free the additional information in the notification
#ifndef UNIT_TEST
static void freeAdditionalInfo(MafOamSpiNotificationFmStruct_4T *comNot);
#else
void freeAdditionalInfo(MafOamSpiNotificationFmStruct_4T *comNot);
#endif
// The opensaf notification callback V4
#ifndef UNIT_TEST
static void saNtfNotificationCallback4(SaNtfSubscriptionIdT subscriptionId, const SaNtfNotificationsT *notification);
#else
void saNtfNotificationCallback4(SaNtfSubscriptionIdT subscriptionId, const SaNtfNotificationsT *notification);
#endif
// Read the additionalInfo pointer value and returns the result as a char
static char* getAdditionalInfoName(SaNtfNotificationHandleT notificationHandle, SaNtfAdditionalInfoT *nextAddInfo);

#ifndef UNIT_TEST
static MafFilterT nFilter_V4;
#else
MafFilterT nFilter_V4;
#endif

// The Event Producer interface
MafOamSpiEventProducer_1T producer_if_V4 = {
	.addFilter = &addFilter_V4,
	.removeFilter =  &removeFilter_V4,
	.clearAll = &clearAll_V4,
	.doneWithValue = &doneWithValue_V4
};

// When a producer sends a notification for an event type, this function is called on all registered subscribers that want this type of notification,
// and that matches their filters.
#ifndef UNIT_TEST
static MafReturnT comsaNotify4(MafOamSpiEventConsumerHandleT handle, const char * eventType, MafNameValuePairT **filter, void * value);
#else
MafReturnT comsaNotify4(MafOamSpiEventConsumerHandleT handle, const char * eventType, MafNameValuePairT **filter, void * value);
#endif

static valueList4T *nVlist4 = NULL;
static valueList4T *nVlLast4 = NULL;


/* Implementation */

/**
 * Clean the FM 4 value list
 *
 * @return No return value.
 */
void cleanVlist4(void)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("cleanVlist4 ENTER");
	if (pthread_mutex_lock(&list_lock) != 0)
	{
		ERR_OAMSA_CMEVENT("cleanVlist4 Failed locking mutex, aborting(!) ...");
		LEAVE_OAMSA_CMEVENT();
		abort();
	}
	valueList4T *tmp = nVlist4;
	while (tmp) {
		valueList4T *tmp2;
		MafOamSpiNotificationFmStruct_4T *cn = tmp->comNot;
		/* free and unlink from list */
		if (NULL != cn->dn) {
			free(cn->dn);
			cn->dn = NULL;
		}
		if (NULL != cn->additionalText) {
			free(cn->additionalText);
			cn->additionalText = NULL;
		}
		freeAdditionalInfo(cn);
		free(cn);
		tmp2 = tmp->next;
		if (MafOamSpiNotificationFm_3 == _mafSpiNtfFmVersion)
		{
			if (tmp->comNot_2) {
				free(tmp->comNot_2);
			}
		}
		free(tmp);
		tmp=tmp2;
	}
	nVlist4 = NULL;
	nVlLast4 = NULL;
	if (pthread_mutex_unlock(&list_lock) != 0)
	{
		ERR_OAMSA_CMEVENT("cleanVlist4 Failed unlocking mutex, aborting(!) ...");
		LEAVE_OAMSA_CMEVENT();
		abort();
	}
	DEBUG_OAMSA_CMEVENT("cleanVlist4 LEAVE");
	LEAVE_OAMSA_CMEVENT();
}

/**
 * Add FM 4 notification to the value list
 *
 * @param[in]                   comNot  Pointer to COM notification struct.
 * @return                      pointer to valueList4T struct item added.
 */
valueList4T* addNotValue4(MafOamSpiNotificationFmStruct_4T *comNot)
{
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("addNotValue4 ENTER");
	if (pthread_mutex_lock(&list_lock) != 0)
	{
		ERR_PMTSA_EVENT("addNotValue4 Failed locking mutex, aborting(!) ...");
		LEAVE_PMTSA_EVENT();
		abort();
	}
	valueList4T *ne = malloc(sizeof(valueList4T));
	if (!ne)
	{
		ERR_PMTSA_EVENT("addNotValue4 Failed allocating memory for 'valueList4T', aborting (!)...");
		LEAVE_PMTSA_EVENT();
		abort();
	}
	ne->next = NULL;
	ne->comNot = comNot;
	if (MafOamSpiNotificationFm_3 == _mafSpiNtfFmVersion)
	{
		ne->comNot_2                 = malloc(sizeof(MafOamSpiNotificationFmStruct_2T));
		ne->comNot_2->dn             = ne->comNot->dn;
		ne->comNot_2->majorType      = ne->comNot->majorType;
		ne->comNot_2->minorType      = ne->comNot->minorType;
		ne->comNot_2->eventTime      = ne->comNot->eventTime;
		ne->comNot_2->additionalText = ne->comNot->additionalText;
		ne->comNot_2->severity       = ne->comNot->severity;
	}
	if (!nVlist4) {
		nVlist4 = ne; nVlLast4 = nVlist4;
	} else {
		nVlLast4->next = ne;
		nVlLast4 = ne;
	}
	if (pthread_mutex_unlock(&list_lock) != 0)
	{
		ERR_PMTSA_EVENT("addNotValue4 Failed unlocking mutex, aborting(!) ...");
		LEAVE_PMTSA_EVENT();
		abort();
	}
	DEBUG_PMTSA_EVENT("addNotValue4 LEAVE");
	LEAVE_PMTSA_EVENT();
	return ne;
}

#ifndef UNIT_TEST
static void DebugDumpAttributeContainer(MafMoAttributeValueContainer_3T *ctp )
#else
void DebugDumpAttributeContainer(MafMoAttributeValueContainer_3T *ctp )
#endif
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("Dumping attribute container");
	if (NULL == ctp) {
		DEBUG_OAMSA_CMEVENT("No attribute");
		LEAVE_OAMSA_CMEVENT();
		return;
	}
	DEBUG_OAMSA_CMEVENT("Number of values %d",ctp->nrOfValues);
	switch (ctp->type)
	{
	case MafOamSpiMoAttributeType_3_INT8:
		DEBUG_OAMSA_CMEVENT("MafOamSpiMoAttributeType_3_INT8");
		break;
	case MafOamSpiMoAttributeType_3_INT16:
		DEBUG_OAMSA_CMEVENT("MafOamSpiMoAttributeType_3_INT16");
		break;
	case MafOamSpiMoAttributeType_3_INT32:
		DEBUG_OAMSA_CMEVENT("MafOamSpiMoAttributeType_3_INT32");
		break;
	case MafOamSpiMoAttributeType_3_INT64:
		DEBUG_OAMSA_CMEVENT("MafOamSpiMoAttributeType_3_INT64");
		break;
	case MafOamSpiMoAttributeType_3_UINT8:
		DEBUG_OAMSA_CMEVENT("MafOamSpiMoAttributeType_3_UINT8");
		break;
	case MafOamSpiMoAttributeType_3_UINT16:
		DEBUG_OAMSA_CMEVENT("MafOamSpiMoAttributeType_3_UINT16");
		break;
	case MafOamSpiMoAttributeType_3_UINT32:
		DEBUG_OAMSA_CMEVENT("MafOamSpiMoAttributeType_3_UINT32");
		break;
	case MafOamSpiMoAttributeType_3_UINT64:
		DEBUG_OAMSA_CMEVENT("MafOamSpiMoAttributeType_3_UINT64");
		break;
	case MafOamSpiMoAttributeType_3_STRING:
		DEBUG_OAMSA_CMEVENT("MafOamSpiMoAttributeType_3_STRING");
		break;
	case MafOamSpiMoAttributeType_3_BOOL:
		DEBUG_OAMSA_CMEVENT("MafOamSpiMoAttributeType_3_BOOL");
		break;
	case MafOamSpiMoAttributeType_3_REFERENCE:
		DEBUG_OAMSA_CMEVENT("MafOamSpiMoAttributeType_3_REFERENCE");
		break;
	case MafOamSpiMoAttributeType_3_ENUM:
		DEBUG_OAMSA_CMEVENT("MafOamSpiMoAttributeType_3_ENUM");
		break;
	case MafOamSpiMoAttributeType_3_STRUCT:
		DEBUG_OAMSA_CMEVENT("MafOamSpiMoAttributeType_3_STRUCT");
		break;
	case MafOamSpiMoAttributeType_3_DECIMAL64:
		DEBUG_OAMSA_CMEVENT("MafOamSpiMoAttributeType_3_DECIMAL64");
		break;
	default:
		ERR_OAMSA_CMEVENT("ERROR - unknown MafOamSpiMoAttributeType_3: %d", ctp->type);
		LEAVE_OAMSA_CMEVENT();
		return;
	}
	unsigned int i;
	for ( i = 0; i < ctp->nrOfValues; i++)
	{
		switch (ctp->type)
		{
		case MafOamSpiMoAttributeType_3_INT8:
			DEBUG_OAMSA_CMEVENT("Value nr %u  = %d", i+1, ctp->values[i].value.i8);
			break;
		case MafOamSpiMoAttributeType_3_INT16:
			DEBUG_OAMSA_CMEVENT("Value nr %u  = %d", i+1,ctp->values[i].value.i16);
			break;
		case MafOamSpiMoAttributeType_3_INT32:
			DEBUG_OAMSA_CMEVENT("Value nr %u  = %d", i+1, ctp->values[i].value.i32);
			break;
		case MafOamSpiMoAttributeType_3_INT64:
			DEBUG_OAMSA_CMEVENT("Value nr %u  = %ld", i+1, ctp->values[i].value.i64);
			break;
		case MafOamSpiMoAttributeType_3_UINT8:
			DEBUG_OAMSA_CMEVENT("Value nr %u  = %u", i+1, ctp->values[i].value.u8);
			break;
		case MafOamSpiMoAttributeType_3_UINT16:
			DEBUG_OAMSA_CMEVENT("Value nr %u  = %u", i+1, ctp->values[i].value.u16);
			break;
		case MafOamSpiMoAttributeType_3_UINT32:
			DEBUG_OAMSA_CMEVENT("Value nr %u  = %u", i+1, ctp->values[i].value.u32);
			break;
		case MafOamSpiMoAttributeType_3_UINT64:
			DEBUG_OAMSA_CMEVENT("Value nr %u  = %lu", i+1, ctp->values[i].value.u64);
			break;
		case MafOamSpiMoAttributeType_3_STRING:
			if (ctp->values[i].value.theString != NULL)
			{
				DEBUG_OAMSA_CMEVENT("Value nr %u  = %s", i+1, ctp->values[i].value.theString);
			}
			else
			{
				DEBUG_OAMSA_CMEVENT("Error String pointer is NULL!!");
			}
			break;
		case MafOamSpiMoAttributeType_3_BOOL:
			DEBUG_OAMSA_CMEVENT("Value nr %u  = %d", i+1, ctp->values[i].value.theBool);
			break;
		case MafOamSpiMoAttributeType_3_STRUCT:
		case MafOamSpiMoAttributeType_3_REFERENCE:
			if (ctp->values[i].value.moRef != NULL)
			{
				DEBUG_OAMSA_CMEVENT("Value nr %u  = %s", i+1, ctp->values[i].value.moRef);
			}
			else
			{
				DEBUG_OAMSA_CMEVENT("Error mo reference is NULL");
			}
			break;

		case MafOamSpiMoAttributeType_3_ENUM:
			DEBUG_OAMSA_CMEVENT("Value nr %u  = %u", i + 1, (unsigned int) (ctp->values[i].value.theEnum));
			break;
		case MafOamSpiMoAttributeType_3_DECIMAL64:
			DEBUG_OAMSA_CMEVENT("MafOamSpiMoAttributeType_3_DECIMAL64: %d", ctp->type);
			DEBUG_OAMSA_CMEVENT("Value nr %u  = %f", i + 1, (double) (ctp->values[i].value.decimal64));
			break;
		default:
			ERR_OAMSA_CMEVENT("ERROR - unknown MafOamSpiMoAttributeType_3: %d", ctp->type);
			LEAVE_OAMSA_CMEVENT();
			return;
		}
	}
	LEAVE_OAMSA_CMEVENT();
}

/**
 * Search for the needle and return everything after it until next comma in resultBuffer,
 * or 0, and an empty buffer.
 *
 * @param[in]       hayStack        big string to search in
 * @param[in]       needle          key-value to search for
 * @param[in,out]   resultBuffer    buffer where data should be stored
 * @param[in]       maxlen          longest output string
 *
 * @retval          1   If needle was found
 * @retval          0   If needle wasn't found
 * @returns     int     The result of the parse
 */
int parseDnValues(const char* hayStack, const char* needle, char* resultBuffer, const int maxlen)
{
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("parseDnValues ENTER hayStack (%s) needle (%s) maxlen %d", hayStack, needle, maxlen);
	assert(hayStack != NULL && needle != NULL && resultBuffer != NULL);
	char* jStart = strstr(hayStack, needle);
	if (jStart == NULL)
	{
		DEBUG_PMTSA_EVENT("parseDnValues jStart == NULL");
		LEAVE_PMTSA_EVENT();
		return 0;
	}
	jStart += strlen(needle);
	char* jStop = strstr(jStart, ",");
	if (jStop == NULL)
	{
		// We're at the bitter end
		jStop = jStart + strlen(jStart);
	}
	// Caller should pass an empty buffer
	int i = 0;
	while (jStart != jStop && i < maxlen)
	{
		resultBuffer[i++] = *jStart;
		++jStart;
	}
	resultBuffer[i] = '\0';
	DEBUG_PMTSA_EVENT("parseDnValues LEAVE resultBuffer %s", resultBuffer);
	LEAVE_PMTSA_EVENT();
	return 1;
}

/**
 * With the given pmJobId and ecimMeasType, this method will look up the ID of
 * the MeasurementReader in IMM.  If it's failing somewhere, it will return NULL,
 * otherwise the Id-string of the MeasurementReader.
 *
 * @param[in]   immOmAccessorHandle     Handle towards IMM
 * @param[in]   ecimMeasTypeDn          DN of measurement type
 * @param[in]   theJobName              Job name that we're matching towards
 *
 * @return      A pointer to a MeasurementReaderId or NULL. Caller should return
 *              memory when finished.
 */
char* findThatEcimMeasReader(SaImmHandleT immOmHandle,
							const SaNameT* ecimMeasTypeDn,
							const char* theJobName)
{
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("findThatEcimMeasReader ENTER ecimMeasTypeDn [%s] theJobName [%s]",  saNameGet(ecimMeasTypeDn), theJobName);
	SaNameT theEcimPmJobId;
	memset(&theEcimPmJobId, '\0', sizeof(theEcimPmJobId));
	{
		/*
		* Build a valid DN for PM-job, find all MeasurementReaders connected.
		* The DN should be pmJobId='jobName',CmwPmpmId=1
		*/
		assert((theJobName != NULL) && (strlen(theJobName) > 0));
		char value[saNameMaxLen()];
		snprintf(value, saNameMaxLen(), "pmJobId=%s,CmwPmpmId=1", theJobName);
		saNameSet(value, &theEcimPmJobId);
		DEBUG_PMTSA_EVENT("DN of PmJob object: %s", saNameGet(&theEcimPmJobId));
	}
	SaImmSearchParametersT_2 immSearchParams;

	immSearchParams.searchOneAttr.attrName = NULL;
	immSearchParams.searchOneAttr.attrValue = NULL;

	SaImmSearchHandleT immSearchHdl;
	SaAisErrorT rc = SA_AIS_OK;
	DEBUG_PMTSA_EVENT("Initializing search with <%s> as key-value", saNameGet(&theEcimPmJobId));
	int retries = 3;
	do {
		rc = saImmOmSearchInitialize_2(immOmHandle,
									&theEcimPmJobId,  /* pmJobId=XYZ */
									SA_IMM_SUBTREE, /* Should give all MeasReader-objects */
									(SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR),
									&immSearchParams,
									NULL,
									&immSearchHdl); /* Handle is only output */
		if (rc != SA_AIS_OK) {
			usleep(OAM_SA_RETRY_SLEEP); /* Sleep 1ms */
			--retries;
		}
	} while ((rc == SA_AIS_ERR_TRY_AGAIN) && (retries > 0));
	if (rc == SA_AIS_OK)
	{
		/*
		* OK, lets start trying to find the MeasReader that matches the MeasType
		*/
		SaImmAttrValuesT_2 **attributes = NULL;
		SaNameT objectName;
		rc = saImmOmSearchNext_2(immSearchHdl, &objectName, &attributes);
		while(rc == SA_AIS_OK || rc == SA_AIS_ERR_TRY_AGAIN)
		{
			retries = 2;
			while ((rc == SA_AIS_ERR_TRY_AGAIN) && (retries > 0))
			{
				usleep(OAM_SA_RETRY_SLEEP);
				rc = saImmOmSearchNext_2(immSearchHdl, &objectName, &attributes);
				--retries;
			}
			if (rc != SA_AIS_OK)
			{
				ERR_PMTSA_EVENT("Got unexpecteded return-code %d from saImmOmSearchNext_2, bailing out", rc);
				(void)saImmOmSearchFinalize(immSearchHdl);
				LEAVE_PMTSA_EVENT();
				return NULL;
			}
			/*
			* The only objects that should show up now will be of PmJob, MeasurementReader or MeasurementSpecification
			* type.  The MeasurementReader should be part of the DN of the MeasurementSpecification DN, thus if we
			* find the DN of the _MeasurementType_ we're looking for in the MeasurementSpecification-object, we're
			* finished!
			* So, check if the attribute-name is 'measurementTypeRef', if so, compare the value of it and if we've
			* found the right one, it will match the DN of the MeasurementType.
			*/
			SaImmAttrValuesT_2 *oneAttr = NULL;
			int attribNumber = 0;
			while ((oneAttr = attributes[attribNumber]) != NULL)
			{
				/* Check and see if we're at measurementTypeRef */
				DEBUG_PMTSA_EVENT("Looking at attrName %s", oneAttr->attrName);
				if (0 == strncmp(oneAttr->attrName, "measurementTypeRef", strlen("measurementTypeRef")))
				{
					/* Bingo! If the value match the DN of the MeasurementType, we're done! */
					DEBUG_PMTSA_EVENT("This might be it: %s", saNameGet(&objectName));
					if (oneAttr->attrValueType == SA_IMM_ATTR_SANAMET && oneAttr->attrValues[0] != NULL)
					{
						DEBUG_PMTSA_EVENT("Comparing <%s> with <%s>", saNameGet((SaNameT*)oneAttr->attrValues[0]), saNameGet(ecimMeasTypeDn));
						if (0 == strncmp(saNameGet((SaNameT*)(oneAttr->attrValues[0])), saNameGet(ecimMeasTypeDn), saNameLen(ecimMeasTypeDn)))
						{
							/*
							* Very much BINGO, we've found the object with the right connection.
							* The MeasurementReader-name is found in the DN of the current object,so
							* get it and return that name.
							*/
							char parsedMeasReadName[saNameMaxLen()];
							memset(parsedMeasReadName, '\0', sizeof(char)*saNameMaxLen());
							if (parseDnValues(saNameGet(&objectName), "measurementReaderId=", parsedMeasReadName, saNameMaxLen()))
							{
								(void)saImmOmSearchFinalize(immSearchHdl);
								DEBUG_PMTSA_EVENT("Returning measurementReaderId=<%s>", parsedMeasReadName);
								LEAVE_PMTSA_EVENT();
								return strdup(parsedMeasReadName);
							}
						}
					}
				}
				++attribNumber;
			}
			rc = saImmOmSearchNext_2(immSearchHdl, &objectName, &attributes);
		}
	}
	else
	{
		if (rc == SA_AIS_ERR_TRY_AGAIN)
		{
			ERR_PMTSA_EVENT("Tried three times to initialize search in IMM but failed, got TRY_AGAIN.");
		}
		else
		{
			ERR_PMTSA_EVENT("Failed initializing search in IMM, return-code=%d", rc);
			if (rc == SA_AIS_ERR_INVALID_PARAM || rc == SA_AIS_ERR_NOT_EXIST)
			{
				DEBUG_PMTSA_EVENT("Initializing PM-root object search with <%s> as root-object failed", saNameGet(&theEcimPmJobId));
			}
		}
	}
	LEAVE_PMTSA_EVENT();
	return NULL;
}

/* MafOamSpiNoticiationFmStruct_4T additions */

/**
 * Copy the additional information from the SaNtfNotificationHeaderT to MafOamSpiNotificationFmStruct_4T
 *
 * @param[in]               nh                  Pointer to NTF header struct
 * @param[in]               notificationHandle  Pointer to the internal notification structure
 * @param[out]              comNot              Pointer to COM notification struct
 * @return No return value.
 */
void copyAdditionalInfo(const SaNtfNotificationHeaderT *nh, const SaNtfNotificationHandleT notificationHandle, MafOamSpiNotificationFmStruct_4T *comNot)
{
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("copyAdditionalInfo ENTER");

	comNot->additionalInfo.size = 0;
	// Copy the additionalInfo
	int iAddInfo = 0;
	DEBUG_PMTSA_EVENT("copyAdditionalInfo nh->numAdditionalInfo=%d", nh->numAdditionalInfo);
	if (nh->numAdditionalInfo > 0)
	{
		char *strValue = NULL;
		int iNumUUIDInfo = 0;
		// Count the number of additionalInfo which is uuid type that needs to map to additionalText
		if (uuid_cfg_flag == UuidMapValueToAddText)
		{
			for (iAddInfo = 0; iAddInfo < nh->numAdditionalInfo; iAddInfo++) {
				if (nh->additionalInfo[iAddInfo].infoId == UUID && nh->notificationClassId->vendorId == ERICSSON_VENDOR_ID && nh->additionalInfo[iAddInfo].infoType == SA_NTF_VALUE_STRING)
				{
					iNumUUIDInfo++;
				}
			}
		}
		// if mapping uuid to additional text, don't send additional Info to Com
		comNot->additionalInfo.size = nh->numAdditionalInfo - iNumUUIDInfo;
		DEBUG_PMTSA_EVENT("copyAdditionalInfo comNot->additionalInfo.size = %d", comNot->additionalInfo.size);
		if (comNot->additionalInfo.size > 0)
		{
			comNot->additionalInfo.additionalInfoArr = malloc(sizeof(MafOamSpiNotificationFmAdditionalInfoT)*(comNot->additionalInfo.size));
			int iComAddInfo = 0;
			for (iAddInfo = 0; iAddInfo < nh->numAdditionalInfo; iAddInfo++)
			{
				DEBUG_PMTSA_EVENT("copyAdditionalInfo iAddInfo=%d, infoId=%d, infoType=%d", iAddInfo, nh->additionalInfo[iAddInfo].infoId, nh->additionalInfo[iAddInfo].infoType);
				bool isUUIDAlarm =  (nh->additionalInfo[iAddInfo].infoId == UUID && nh->notificationClassId->vendorId == ERICSSON_VENDOR_ID && nh->additionalInfo[iAddInfo].infoType == SA_NTF_VALUE_STRING);
				// if it is not the case mapping UUID to additionalText. Need to process additionalText outside copyAdditionalInfo function
				if (!(uuid_cfg_flag == UuidMapValueToAddText && isUUIDAlarm))
				{
					if (uuid_cfg_flag == UuidMapToAddInfoName && isUUIDAlarm)
					{
						comNot->additionalInfo.additionalInfoArr[iComAddInfo].name = strdup("uuid");
					}
					else
					{
						comNot->additionalInfo.additionalInfoArr[iComAddInfo].name = strdup("");
					}

					strValue = getAdditionalInfoName(notificationHandle, &nh->additionalInfo[iAddInfo]);
					if (strValue != NULL)
					{
						comNot->additionalInfo.additionalInfoArr[iComAddInfo].value = strdup(strValue);
						if (nh->additionalInfo[iAddInfo].infoType == SA_NTF_VALUE_LDAP_NAME)
						{
							free (strValue);
							strValue = NULL;
						}
					}
					else
					{
						comNot->additionalInfo.additionalInfoArr[iComAddInfo].value = strdup("");
					}
					DEBUG_PMTSA_EVENT("copyAdditionalInfo name (%s) value (%s)",
								comNot->additionalInfo.additionalInfoArr[iComAddInfo].name,
								comNot->additionalInfo.additionalInfoArr[iComAddInfo].value);
					iComAddInfo++;
				}
			}
		}
		else
		{
			comNot->additionalInfo.additionalInfoArr = NULL;
			DEBUG_PMTSA_EVENT("copyAdditionalInfo comNot->additionalInfo.size = %d ",comNot->additionalInfo.size);
		}
	}
	DEBUG_PMTSA_EVENT("copyAdditionalInfo LEAVE");
	LEAVE_PMTSA_EVENT();
}

/**
 * Free the additional information from the MafOamSpiNotificationFmStruct_4T structure
 *
 * @param[in]                   comNot  Pointer to COM notification struct
 * @return No return value.
 */
void freeAdditionalInfo(MafOamSpiNotificationFmStruct_4T *comNot)
{
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("freeAdditionalInfo ENTER");
	DEBUG_PMTSA_EVENT("freeAdditionalInfo comNot->additionalInfo.size=%d", comNot->additionalInfo.size);
	int iAddInfo = 0;
	MafOamSpiNotificationFmAdditionalInfoT *addInfo = comNot->additionalInfo.additionalInfoArr;
	for (iAddInfo = 0; iAddInfo < comNot->additionalInfo.size; iAddInfo++)
	{
		free(addInfo[iAddInfo].name);
		free(addInfo[iAddInfo].value);
		addInfo[iAddInfo].name = NULL;
		addInfo[iAddInfo].value = NULL;
	}
	free(addInfo);
	comNot->additionalInfo.additionalInfoArr = NULL;
	comNot->additionalInfo.size = 0;
	DEBUG_PMTSA_EVENT("freeAdditionalInfo LEAVE");
	LEAVE_PMTSA_EVENT();
}

/**
 * Re-Initializes IMM handlers
 *
 * @param[in] immOmHandle      		IMM handler to be re-initialized
 * @param[in] immOmAccessorHandle	Accessor handler to be re-initialized
 *
 * @return  The result of IMM re-initialization
 * @retval  SA_AIS_OK      if re-initialization went fine
 */
SaAisErrorT reInitializeBadHandler(SaImmHandleT* immOmHandle, SaImmAccessorHandleT* immOmAccessorHandle)
{
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("reInitializeBadHandler ENTER");

	/* Try to clean up before leaving */
	(void)saImmOmAccessorFinalize(*immOmAccessorHandle);
	(void)saImmOmFinalize(*immOmHandle);

	SaVersionT immVersion = { immReleaseCode, immMajorVersion, immMinorVersion };

	/* Try to initialize IMM handler */
	SaAisErrorT retCode = saImmOmInitialize(immOmHandle, NULL, &immVersion);

	if(SA_AIS_OK == retCode)
	{
		/* Try to initialize Accessor handler */
		retCode = saImmOmAccessorInitialize(*immOmHandle, immOmAccessorHandle);
	}
	LEAVE_PMTSA_EVENT();
	return retCode;
}
/*
 * Handle error codes bad handle and try again of get MO attributes
 */
MafReturnT getMOAttributesRetry(SaImmHandleT* omHandle, SaImmAccessorHandleT* accessorHandle, const SaNameT *objectName, const SaImmAttrNameT *attributeNames, SaImmAttrValuesT_2 ***attributes )
{
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("getMOAttributesRetry: saImmOmAccessorGet_2 Mo = %s", saNameGet(objectName));
	SaAisErrorT retCode = saImmOmAccessorGet_2(*accessorHandle, objectName, attributeNames, attributes);

	if(SA_AIS_ERR_BAD_HANDLE == retCode)
	{
		LOG_PMTSA_EVENT("getMOAttributesRetry::saImmOmAccessorGet_2 returns SA_AIS_ERR_BAD_HANDLE, hence re-initializing handlers");
		if(SA_AIS_OK == reInitializeBadHandler(omHandle, accessorHandle))
		{
			LOG_PMTSA_EVENT("getMOAttributesRetry:reInitializeBadHandler returned SUCCESS, calling saImmOmAccessorGet_2 again with new handlers");
			retCode = saImmOmAccessorGet_2(*accessorHandle, objectName, attributeNames, attributes);
			LOG_PMTSA_EVENT("saImmOmAccessorGet_2 after re-initialize returned retCode=%d",retCode);
		}
		else
		{
			LOG_PMTSA_EVENT("getMOAttributesRetry:reInitializeBadHandler FAILED and returned retCode=%d",retCode);
		}
	}

	if (retCode != SA_AIS_OK)
	{
		int loggedRetry = 0;
		while (((retCode == SA_AIS_ERR_TRY_AGAIN) || (retCode == SA_AIS_ERR_BUSY)) && (loggedRetry < OAM_SA_NTF_MAX_RETRIES))
		{
			if (!loggedRetry)
			{
				LOG_PMTSA_EVENT("getMOAttributesRetry: Failed reading value of the alarmType, retry in 1ms.");
			}
			++loggedRetry;
			usleep(1000); // Sleep 1 ms
			retCode = saImmOmAccessorGet_2(*accessorHandle, objectName, attributeNames, attributes);
		}
		if (retCode != SA_AIS_OK)
		{
				DEBUG_PMTSA_EVENT("getMOAttributesRetry Couldn't read attribute of MO=%s, bailing out. Return-code=%d", saNameGet(objectName), retCode);
				LEAVE_PMTSA_EVENT();
				return MafFailure;
		}
	}
	LEAVE_PMTSA_EVENT();
	return MafOk;
}

/* Read thresholdDirection from Measurement Reader MO.
 * If not set read it from Measurement Type MO
 */
MafReturnT getThresholdDirection(SaImmHandleT* omHandle, SaImmAccessorHandleT* omAccessorHandle, const char* thePmJobName, const char* theMeasReaderId, SaNameT* measTypeIMMDn, SaInt32T* thresholdDirection){
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("getThresholdDirection ENTER");
	*thresholdDirection = 3; //default value indicates not set

	//get Threshold Direction from Measurement Reader
	size_t ecimMeasReaderDnSize = strlen("measurementReaderId=") + strlen(theMeasReaderId) +
			strlen(",pmJobId=") + strlen(thePmJobName) +
			strlen(",CmwPmpmId=1");
	char* ecimMeasurementReaderId = malloc(ecimMeasReaderDnSize+1);

	memset(ecimMeasurementReaderId, '\0', ecimMeasReaderDnSize +1);
	char* ptr = ecimMeasurementReaderId;
	memcpy(ptr, "measurementReaderId=", strlen("measurementReaderId="));
	ptr += strlen("measurementReaderId=");
	memcpy(ptr, theMeasReaderId, strlen(theMeasReaderId));
	ptr += strlen(theMeasReaderId);
	memcpy(ptr, ",pmJobId=", strlen(",pmJobId="));
	ptr += strlen(",pmJobId=");
	memcpy(ptr, thePmJobName, strlen(thePmJobName));
	ptr += strlen(thePmJobName);
	memcpy(ptr, ",CmwPmpmId=1", strlen(",CmwPmpmId=1"));
	SaNameT ecimMeasReaderDN;
	memset(&ecimMeasReaderDN, '\0', sizeof(ecimMeasReaderDN));
	saNameSet(ecimMeasurementReaderId, &ecimMeasReaderDN);
	SaImmAttrNameT saMeasReaderThresDirAttr[2];
	saMeasReaderThresDirAttr[0] = "thresholdDirection";
	saMeasReaderThresDirAttr[1] = '\0';
	SaImmAttrValuesT_2** theAttribValue;
	DEBUG_PMTSA_EVENT("getThresholdDirection:  Reading MeasurementReader-object, trying to fetch thresholdDirection attribute");
	DEBUG_PMTSA_EVENT("                          MeasurementReader-DN = <%s>", saNameGet(&ecimMeasReaderDN));

	MafReturnT ret = getMOAttributesRetry(omHandle, omAccessorHandle,&ecimMeasReaderDN, saMeasReaderThresDirAttr, &theAttribValue);
	if(ret != MafOk){
		ERR_PMTSA_EVENT("getThresholdDirection: Couldn't read attributes of MO =%s, bailing out. Return-code=%d", saNameGet(&ecimMeasReaderDN), ret);
		if (ecimMeasurementReaderId != NULL) free(ecimMeasurementReaderId);
		LEAVE_PMTSA_EVENT();
		return MafFailure;
	}

	if(theAttribValue[0] != NULL){
		if (strncmp("thresholdDirection", theAttribValue[0]->attrName, strlen("thresholdDirection")) == 0)
		{
			if(theAttribValue[0]->attrValuesNumber > 0){
				*thresholdDirection = *(SaInt32T*)(theAttribValue[0]->attrValues[0]);
			}
		}
	}else{
		ERR_PMTSA_EVENT("getThresholdDirection: reading measurementReader MO failed, the  attribute value is null");
	}
	if (NULL != ecimMeasurementReaderId) {
		free(ecimMeasurementReaderId);
		ecimMeasurementReaderId = NULL;
	}
	DEBUG_PMTSA_EVENT("getThresholdDirection measurementReader thresholdDirection=%d", *thresholdDirection);
	if(*thresholdDirection == 3){ //means thresholdDirection  in measurementReader is not set
		//get thresholdDirection from measurement type DN
		SaImmAttrNameT saMeasTypeThresDirAttr[2];
		saMeasTypeThresDirAttr[0] = "thresholdDirection";
		saMeasTypeThresDirAttr[1] = '\0';
		SaImmAttrValuesT_2** attrValue;
		DEBUG_PMTSA_EVENT("getThresholdDirection, saImmOmAccessorGet_2 ...  Reading MeasurementType-object, trying to fetch thresholdDirection attribute");
		DEBUG_PMTSA_EVENT("                          MeasurementType DN = <%s>", saNameGet(measTypeIMMDn));
		MafReturnT ret = getMOAttributesRetry(omHandle, omAccessorHandle,measTypeIMMDn, saMeasTypeThresDirAttr, &attrValue);
		if(ret != MafOk){
			ERR_PMTSA_EVENT("getThresholdDirection: Couldn't read attributes of MO =%s, bailing out. Return-code=%d", saNameGet(&ecimMeasReaderDN), ret);
			LEAVE_PMTSA_EVENT();
			return MafFailure;
		}

		if(attrValue[0] != NULL){
			if (strncmp("thresholdDirection", attrValue[0]->attrName, strlen("thresholdDirection")) == 0)
			{
				if(attrValue[0]->attrValuesNumber > 0){
					*thresholdDirection = *(SaInt32T*)(attrValue[0]->attrValues[0]);
				}
			}
		}else{
			ERR_PMTSA_EVENT("getThresholdDirection: reading measurementType MO failed, the  attribute value is null");
		}
		DEBUG_PMTSA_EVENT("getThresholdDirection measurementType thresholdDirection=%d", *thresholdDirection);
	}
	DEBUG_PMTSA_EVENT("getThresholdDirection thresholdDirection=%d", *thresholdDirection);
	LEAVE_PMTSA_EVENT();
	return MafOk;
}


/**
 * Converts a Performance Management Threshold notification to the
 * type of alarm that the application has decided it wanted to be
 * sent to COM.
 *
 * Pick out the job-name from the dn retreived from notifyingObject,
 * then create an alarm by adding the job-name to stuff found in
 * additional text.
 *
 * @param[in]                   nh      Pointer to NTF notification data header
 * @param[in,out]               comNot  Pointer to COM notification struct
 * @return                      The result of the operation
 * @retval  MafOk               Things went fine
 * @retval  MafFailure          Calls to SAF-services failed.
 * @retval  MafNoResources      We're out of memory
 * @retval  MafInvalidArgument  Couldn't parse the DN of notifying object
 */
#ifndef UNIT_TEST
static MafReturnT convertPmThresholdHeader4(const SaNtfNotificationsT *notification, MafOamSpiNotificationFmStruct_4T *comNot)
#else
MafReturnT convertPmThresholdHeader4(const SaNtfNotificationsT *notification, MafOamSpiNotificationFmStruct_4T *comNot)
#endif
{
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("convertPmThresholdHeader4 ENTER");

	const SaNtfNotificationHeaderT *nh = NULL;
	SaNtfNotificationHandleT notificationHandle = 0 ;
	SaNtfThresholdInformationT *thresholdInformation = NULL;
	if(notification->notificationType == SA_NTF_TYPE_SECURITY_ALARM){
		nh = &notification->notification.securityAlarmNotification.notificationHeader;
		notificationHandle = notification->notification.securityAlarmNotification.notificationHandle;
		//threshold information is not available for security alarm notifications,treated as null
	}else{ // SA_NTF_TYPE_ALARM or SA_NTF_ALARM_QOS
		nh = &notification->notification.alarmNotification.notificationHeader;
		notificationHandle = notification->notification.alarmNotification.notificationHandle;
		thresholdInformation = notification->notification.alarmNotification.thresholdInformation;
	}
	SaNameT *notifyingObject = nh->notifyingObject;
	/*
	 * See if we can pick out the actual job-name, we need it
	 * to create a "unique" alarm.
	 */
	char thePmJobName[saNameMaxLenNtf()];
	memset(thePmJobName, '\0', saNameMaxLenNtf());
	if (parseDnValues(saNameGet(notifyingObject), "\\,safPm=1,safJob=", thePmJobName, saNameMaxLenNtf()) == 0)
	{
		ERR_PMTSA_EVENT("convertPmThresholdHeader4 Failed parsing out the job-name (safJob=) from <%s>", saNameGet(notifyingObject));
		LEAVE_PMTSA_EVENT();
		return MafInvalidArgument;
	}
	/*
	 * Ok, in the additionalText attribute, we have the full DN of the
	 * measurement type.  Read the measurement type from IMM, and fetch
	 * the alarm that should be sent (and the alarm attribute in the measurement
	 * type is of course yet another DN that also must be read).
	 */
	SaImmHandleT immOmHandle;
	SaVersionT immVersion = { immReleaseCode, immMajorVersion, immMinorVersion };
	DEBUG_PMTSA_EVENT("convertPmThresholdHeader4.saImmOmInitialize ...");
	SaAisErrorT retCode = saImmOmInitialize(&immOmHandle, NULL, &immVersion);
	if (retCode != SA_AIS_OK)
	{
		int loggedRetry = 0;
		while (((retCode == SA_AIS_ERR_TRY_AGAIN) || (retCode == SA_AIS_ERR_BUSY)) && (loggedRetry < OAM_SA_NTF_MAX_RETRIES))
		{
			if (!loggedRetry)
			{
				LOG_PMTSA_EVENT("convertPmThresholdHeader4 Failed initializing ImmOm-handle, retry in 1ms.");
			}
			++loggedRetry;
			usleep(OAM_SA_RETRY_SLEEP); // Sleep 1 ms
			retCode = saImmOmInitialize(&immOmHandle, NULL, &immVersion);
		}
		if (retCode != SA_AIS_OK)
		{
			ERR_PMTSA_EVENT("convertPmThresholdHeader4 Failed initializing IMM-OM handle, return-code=%d", retCode);
			LEAVE_PMTSA_EVENT();
			return MafFailure;
		}
	}
	/*
	 * Duhh, we need an accessor handle too :-O
	 */
	SaImmAccessorHandleT immOmAccessorHandle;
	DEBUG_PMTSA_EVENT("convertPmThresholdHeader4.saImmOmAccessorInitialize ...");
	retCode = saImmOmAccessorInitialize(immOmHandle, &immOmAccessorHandle);
	if (retCode != SA_AIS_OK)
	{
		int loggedRetry = 0;
		while ((retCode == SA_AIS_ERR_TRY_AGAIN) && (loggedRetry < OAM_SA_NTF_MAX_RETRIES))
		{
			if (!loggedRetry)
			{
				LOG_PMTSA_EVENT("convertPmThresholdHeader4 Failed initializing ImmOmAccessor-handle, retry in 1ms.");
			}
			++loggedRetry;
			usleep(1000); // Sleep 1 ms
			retCode = saImmOmAccessorInitialize(immOmHandle, &immOmAccessorHandle);
		}
		if (retCode != SA_AIS_OK)
		{
			/* Try to clean up before leaving */
			(void)saImmOmFinalize(immOmHandle);
			ERR_PMTSA_EVENT("convertPmThresholdHeader4 Failed initializing IMM-OM accessor handle, return-code=%d", retCode);
			LEAVE_PMTSA_EVENT();
			return MafFailure;
		}
	}
	/*
	 * Now read the measurementType object and find out the alarm.
	 * The "key" to the measurementType is in additional text
	 */
	char* ecimMeasurementTypeId = strdup((char*)nh->additionalText);
	if (ecimMeasurementTypeId == NULL || strlen(ecimMeasurementTypeId) == 0)
	{
		/* Try to clean up before leaving */
		(void)saImmOmAccessorFinalize(immOmAccessorHandle);
		(void)saImmOmFinalize(immOmHandle);
		if (ecimMeasurementTypeId != NULL) free(ecimMeasurementTypeId);
		ERR_PMTSA_EVENT("convertPmThresholdHeader4 No information about ecimMeasurementTypeId in notification from PM, can't continue!");
		LEAVE_PMTSA_EVENT();
		return MafInvalidArgument;
	}
	SaNameT ecimMeasTypeDN;
	memset(&ecimMeasTypeDN, '\0', sizeof(ecimMeasTypeDN));
	saNameSet(ecimMeasurementTypeId, &ecimMeasTypeDN);
	SaImmAttrNameT saMeasTypeAlarmAttr[2];
	saMeasTypeAlarmAttr[0] = "fmAlarmType";
	saMeasTypeAlarmAttr[1] = '\0';
	SaImmAttrValuesT_2** theAttribValue;
	DEBUG_PMTSA_EVENT("convertPmThresholdHeader4:  Reading MeasurementType-object, trying to fetch fmAlarmType attribute");
	MafReturnT ret = getMOAttributesRetry(&immOmHandle, &immOmAccessorHandle,&ecimMeasTypeDN, saMeasTypeAlarmAttr, &theAttribValue);
	if(ret != MafOk){
		(void)saImmOmAccessorFinalize(immOmAccessorHandle);
		(void)saImmOmFinalize(immOmHandle);
		ERR_PMTSA_EVENT("convertPmThresholdHeader4 Couldn't read attributes of measTypeObjDN=%s, bailing out. Return-code=%d", ecimMeasurementTypeId, ret);
		if (ecimMeasurementTypeId != NULL) free(ecimMeasurementTypeId);
		LEAVE_PMTSA_EVENT();
		return MafFailure;
	}
	/* Not needed anymore */
	if (ecimMeasurementTypeId != NULL)
	{
		free(ecimMeasurementTypeId);
		ecimMeasurementTypeId = NULL;
	}
	/*
	 * So, once here, we have the values, right ...
	 */
	assert(theAttribValue[0] != NULL);

	/*
	 * theAttribValue should now contain the DN of the alarm. Read the fmAlarmType to find
	 * out what alarm to send.
	 */
	SaNameT saFmAlarmTypeDN;
	bool isPmGenricThresholdAlarm = false;
	memset(&saFmAlarmTypeDN, '\0', sizeof(saFmAlarmTypeDN));

	SaImmAttrValuesT_2* oneAttrValue = theAttribValue[0];
	if (oneAttrValue->attrValueType != SA_IMM_ATTR_SANAMET)
	{
		/* Try to clean up before leaving */
		(void)saImmOmAccessorFinalize(immOmAccessorHandle);
		(void)saImmOmFinalize(immOmHandle);
		ERR_PMTSA_EVENT("convertPmThresholdHeader4 Unexpected attribute-type from read, should have been %d but was %d", SA_IMM_ATTR_SANAMET, oneAttrValue->attrValueType);
		LEAVE_PMTSA_EVENT();
		return MafFailure;
	}
	if(oneAttrValue->attrValuesNumber > 0){
		DEBUG_PMTSA_EVENT("convertPmThresholdHeader4.Fetched DN of fmAlarmType, len = %d, dn = %s", saNameLen((SaNameT*)(oneAttrValue->attrValues[0])), saNameGet((SaNameT*)oneAttrValue->attrValues[0]));
		saNameSet(saNameGet((SaNameT*)(oneAttrValue->attrValues[0])), &saFmAlarmTypeDN);
		if((strcmp(saNameGet(&saFmAlarmTypeDN),"0") == 0) || (strcmp(saNameGet(&saFmAlarmTypeDN),"\0") == 0))
		{
			saNameSet(PM_THRESHOLD_FMALARMTYPE_DN, &saFmAlarmTypeDN);
			isPmGenricThresholdAlarm = true;
			DEBUG_PMTSA_EVENT("Set default alarm %s when fmAlarmType is 0 or null", PM_THRESHOLD_FMALARMTYPE_DN);
		}else if((strcmp(saNameGet(&saFmAlarmTypeDN), PM_THRESHOLD_FMALARMTYPE_DN) == 0)){
			isPmGenricThresholdAlarm = true;
			DEBUG_PMTSA_EVENT("fmAlarmType is already set with %s", PM_THRESHOLD_FMALARMTYPE_DN);
		}
	}else{
		DEBUG_PMTSA_EVENT("Set default alarm %s when fmAlarmType is null", PM_THRESHOLD_FMALARMTYPE_DN);
		saNameSet(PM_THRESHOLD_FMALARMTYPE_DN, &saFmAlarmTypeDN);
		isPmGenricThresholdAlarm = true;
	}

	SaImmAttrNameT saTheAlarms[4];
	saTheAlarms[0] = "majorType";
	saTheAlarms[1] = "minorType";
	saTheAlarms[2] = "additionalText";
	saTheAlarms[3] = '\0';
	SaImmAttrValuesT_2** theAlarmCodes;
	DEBUG_PMTSA_EVENT("convertPmThresholdHeader4: Reading the alarm values (majorType & minorType & additionalText)");
	ret = getMOAttributesRetry(&immOmHandle, &immOmAccessorHandle,&saFmAlarmTypeDN, saTheAlarms, &theAlarmCodes);
	if(ret != MafOk){
		(void)saImmOmAccessorFinalize(immOmAccessorHandle);
		(void)saImmOmFinalize(immOmHandle);
		ERR_PMTSA_EVENT("convertPmThresholdHeader4 Couldn't read attributes of MO =%s, bailing out. Return-code=%d", saNameGet(&saFmAlarmTypeDN), ret);
		LEAVE_PMTSA_EVENT();
		return MafFailure;
	}

	/*
	 * So, once here, we have the values, right ...
	 */
	assert(theAlarmCodes[0] != NULL && theAlarmCodes[1] != NULL && theAlarmCodes[2] != NULL);

	/*
	 * Only in C :-)
	 * Cast void* to SaUint32T* and then fetch what we're pointing to :-D
	 */
	SaUint32T majAlarm = 0;
	SaUint32T minAlarm = 0;
	SaStringT addText = NULL;
	/*
	 * Only in IMM :-(
	 * Just because you ask for majorType in 0 and minorType in 1 doesn't
	 * mean that you'll get them in that order ...
	 */
	int iIndex = 0;

	for (; iIndex < 3; iIndex++ )
	{
		if (strncmp("majorType", theAlarmCodes[iIndex]->attrName, strlen("majorType")) == 0)
		{
			if (theAlarmCodes[iIndex]->attrValueType != SA_IMM_ATTR_SAUINT32T)
			{
				iIndex = -1; // Error code
				ERR_PMTSA_EVENT("convertPmThresholdHeader4.Unexpected result from reading alarm-attribute <majorType>, expected %d but got %d", SA_IMM_ATTR_SAUINT32T, theAlarmCodes[iIndex]->attrValueType);
				break;
			}
			majAlarm = *(SaUint32T*)(theAlarmCodes[iIndex]->attrValues[0]);
		}
		else if (strncmp("minorType", theAlarmCodes[iIndex]->attrName, strlen("minorType")) == 0)
		{
			if (theAlarmCodes[iIndex]->attrValueType != SA_IMM_ATTR_SAUINT32T)
			{
				iIndex = -1; // Error code
				ERR_PMTSA_EVENT("convertPmThresholdHeader4.Unexpected result from reading alarm-attribute <minorType>, expected %d but got %d", SA_IMM_ATTR_SAUINT32T, theAlarmCodes[iIndex]->attrValueType);
				break;
			}
			minAlarm = *(SaUint32T*)(theAlarmCodes[iIndex]->attrValues[0]);
		}
		else
		{
			if (theAlarmCodes[iIndex]->attrValueType != SA_IMM_ATTR_SASTRINGT)
			{
				ERR_PMTSA_EVENT("convertPmThresholdHeader4.Unexpected result from reading alarm-attribute <additionalText>, expected %d but got %d", SA_IMM_ATTR_SASTRINGT, theAlarmCodes[iIndex]->attrValueType);
				iIndex = -1; // Error code
				break;
			}
			if (theAlarmCodes[iIndex]->attrValues != NULL && theAlarmCodes[iIndex]->attrValuesNumber > 0)
			{
				addText = *(SaStringT*)(theAlarmCodes[iIndex]->attrValues[0]);
			}
		}
	}

	if (iIndex == -1)
	{
		/* Try to clean up before leaving */
		(void)saImmOmAccessorFinalize(immOmAccessorHandle);
		(void)saImmOmFinalize(immOmHandle);
		LEAVE_PMTSA_EVENT();
		return MafFailure;
	}

	DEBUG_PMTSA_EVENT("convertPmThresholdHeader4.PM-notification: Extracted major alarm=%u, minor alarm=%u, additionalText=%s", majAlarm, minAlarm, (addText==NULL ? "" : addText));
	/*
	 * Threshold monitoring functionality in Opensaf PM will send Alarm
	 * Notification with the following attributes set:
	 *
	 * notificationObject  :  measObjLDN (will be and has to be restricted to length 255 in producer API)
	 * notifyingObject     :  dn of SaPmfThreshold
	 * additionalText      :  dn of MeasurementType (OsafPmf gets this dn from Imm / SaPmfMT.saThresholdAdditionalText)
	 * vendorId            : 18568   (SA_NTF_VENDOR_ID_SAF)
	 * majorId             : 20 (current max osaf majorId + 5)
	 * minorId             : 1 (this is first pm notification)
	 *
	 * The DN of the COM-notification should how-ever be like this:
	 * "ManagedElement=1,SystemFunctions=1,Pm=1,PmJob={pmJobId},MeasurementReader={measurementReaderId}:measObjLdn".
	 * We have the pmJobId (parsed in the beginning of this function) and the MeasurementType.
	 * This gives the following:
	 * Find the 'MeasurementReader' that points to a 'MeasurementSpecification' that points to the
	 * given 'MeasurementType' :-)
	 */
	char* theMeasReaderId = findThatEcimMeasReader(immOmHandle, &ecimMeasTypeDN, thePmJobName);
	if (theMeasReaderId != NULL)
	{
		/* Length of alarm identity + extra nulled bytes */
		int lenOfNewDN = strlen("ManagedElement=1,SystemFunctions=1,Pm=1,PmJob=,MeasurementReader=:") +
				strlen(thePmJobName) +
				strlen(theMeasReaderId) +
				(saNameLen((SaNameT*)nh->notificationObject)+1);
		comNot->dn = (char*)malloc((lenOfNewDN*sizeof(char))+1);
		if (comNot->dn == NULL)
		{
			ERR_PMTSA_EVENT("convertPmThresholdHeader4.Couldn't allocate memory for COM notification DN!");
			/* Try to clean up before leaving */
			(void)saImmOmAccessorFinalize(immOmAccessorHandle);
			(void)saImmOmFinalize(immOmHandle);
			if (theMeasReaderId != NULL) free(theMeasReaderId);
			LEAVE_PMTSA_EVENT();
			return MafNoResources;
		}
		memset(comNot->dn, '\0', lenOfNewDN);
		char* destPtr = comNot->dn;

		memcpy(destPtr, "ManagedElement=1,SystemFunctions=1,Pm=1,PmJob=", strlen("ManagedElement=1,SystemFunctions=1,Pm=1,PmJob="));
		destPtr += strlen("ManagedElement=1,SystemFunctions=1,Pm=1,PmJob=");
		memcpy(destPtr, thePmJobName, strlen(thePmJobName));
		destPtr += strlen(thePmJobName);

		memcpy(destPtr, ",MeasurementReader=", strlen(",MeasurementReader="));
		destPtr += strlen(",MeasurementReader=");
		memcpy(destPtr, theMeasReaderId, strlen(theMeasReaderId));
		destPtr += strlen(theMeasReaderId);
		memcpy(destPtr, ":", strlen(":"));
		destPtr += strlen(":");
		memcpy(destPtr, saNameGet((SaNameT*)nh->notificationObject), saNameLen((SaNameT*)nh->notificationObject));

		if ( isPmGenricThresholdAlarm ){
			// get Additional Text fields in case of PM generic threshold alarms
			SaNameT ntfAddText;
			memset(&ntfAddText, '\0', sizeof(ntfAddText));
			saNameSet((char*)nh->additionalText, &ntfAddText);

			//get threshold Direction
			SaInt32T thresholdDirection = 3; //default value indicates empty
			MafReturnT ret = getThresholdDirection(&immOmHandle, &immOmAccessorHandle, thePmJobName, theMeasReaderId , &ntfAddText, &thresholdDirection);
			saNameDelete(&ntfAddText,false);
			if(ret != MafOk){
				(void)saImmOmAccessorFinalize(immOmAccessorHandle);
				(void)saImmOmFinalize(immOmHandle);
				if (theMeasReaderId != NULL) free(theMeasReaderId);
				LEAVE_PMTSA_EVENT();
				return ret;
			}

			// get 3gpp MeasurementType DN
			char* measTypeDn = convertTo3Gpp((SaNameT*)&ntfAddText);

			//get measObjLdn
			size_t measObjLdnSize = saNameLen((SaNameT*)nh->notificationObject);
			char* measObjLdn = malloc(measObjLdnSize+1);
			memset(measObjLdn, '\0', measObjLdnSize+1);
			memcpy(measObjLdn, saNameGet((SaNameT*)nh->notificationObject), measObjLdnSize);

			//Prepare additional Text
			addText = formatAdditionalText(thresholdInformation, measTypeDn, thresholdDirection, measObjLdn);
			/* Not needed anymore */
			if(NULL != measTypeDn){
				free(measTypeDn);
				measTypeDn = NULL;
			}

			if (NULL != measObjLdn) {
				free(measObjLdn);
				measObjLdn = NULL;
			}
		}

		if (addText != NULL && strlen(addText) > 0)
		{
			if ( !isPmGenricThresholdAlarm ){
				comNot->additionalText = strdup(addText);
			}else{
				comNot->additionalText = addText; //memory already allocated
			}
		}else
		{
			comNot->additionalText = strdup("");
		}

		if(NULL != theMeasReaderId){
			free(theMeasReaderId);
			theMeasReaderId = NULL;
		}

		comNot->eventTime = *nh->eventTime;
		comNot->majorType = majAlarm;
		comNot->minorType = minAlarm;
		copyAdditionalInfo(nh, notificationHandle, comNot);
		DEBUG_PMTSA_EVENT("convertPmThresholdHeader4.Sending PM-notification on dn=<%s>, majorType=%u, minorType=%u", comNot->dn, comNot->majorType, comNot->minorType);
	}
	else
	{
		ERR_PMTSA_EVENT("convertPmThresholdHeader4.Could not find measurement reader object.");
		(void)saImmOmAccessorFinalize(immOmAccessorHandle);
		(void)saImmOmFinalize(immOmHandle);
		LEAVE_PMTSA_EVENT();
		return MafFailure;
	}
	/* Clean up before leaving */
	(void)saImmOmAccessorFinalize(immOmAccessorHandle);
	(void)saImmOmFinalize(immOmHandle);
	DEBUG_PMTSA_EVENT("convertPmThresholdHeader4 LEAVE");
	LEAVE_PMTSA_EVENT();
	return MafOk;
}

/**
 * Translates the IMM RDN to a 3GPP DN
 *
 * @param[in] no      pointer to SaNameT containing the IMM RDN
 * @param[out] dn     pointer to the created dn of 3GPP format
 *
 * @return  The result of the conversion of an error code
 * @retval  true      if conversion went fine
 * @retval  false     if the conversion failed
 */
bool convertTo3GPPDN(SaNameT *no, char **dn)
{
	ENTER_OAMSA_CMEVENT();
	if (saNameLen(no) >= 4 && !strncmp("3GPP", saNameGet(no), 4))
	{
		*dn = malloc (saNameMaxLenNtf());
		if (!*dn)
		{
			LEAVE_OAMSA_CMEVENT();
			return false;
		}
		strncpy(*dn, &saNameGet(no)[4], saNameMaxLenNtf() - 5);
		(*dn)[saNameLen(no)-4]='\0';
	} else
	{
		*dn = convertTo3Gpp(no);
		if(!*dn)
		{
			LEAVE_OAMSA_CMEVENT();
			return false;
		}
	}
	LEAVE_OAMSA_CMEVENT();
	return true;
}

/**
 * Read the additionalInfo pointer value and returns the result as a char
 *
 * @param[in] notificationHandle The NTF notification handle for this 'session'
 * @param[in] nextAddInfo     pointer to SaNtfAdditionalInfoT array containing the name of the attributes
 *
 * @return  The result of the read operation
 * @retval  NOT NULL	if read went fine
 * @retval  NULL		if the read failed
 */
char* getAdditionalInfoName(SaNtfNotificationHandleT notificationHandle,
			SaNtfAdditionalInfoT *nextAddInfo)
{
	ENTER_OAMSA_CMEVENT();
	// Get the pointer
	if (nextAddInfo->infoType == SA_NTF_VALUE_STRING)
	{
			void *dataPtr;
			SaUint16T dataSize;
#ifdef UNIT_TEST
			SaAisErrorT retVal = saNtfPtrValGetTest(notificationHandle,        // [IN] Handle to identify the transaction
					&(nextAddInfo->infoValue),						// [IN] offset and size of data buffer
					&dataPtr, 										// [OUT] Pointer to the buffer with the data
					&dataSize, false);								// [OUT] Size of this particular data (Including null at the end of the string)
#else
			SaAisErrorT retVal = saNtfPtrValGet(notificationHandle, // [IN] Handle to identify the transaction
					&(nextAddInfo->infoValue),						// [IN] offset and size of data buffer
					&dataPtr, 										// [OUT] Pointer to the buffer with the data
					&dataSize); 									// [OUT] Size of this particular data (Including null at the end of the string)
#endif
			if (retVal == SA_AIS_OK)
			{
				DEBUG_OAMSA_CMEVENT("getAdditionalInfoName : Read Additional Information infoId:%i infoType:SA_NTF_VALUE_STRING InfValue:%s",nextAddInfo->infoId , (char*)dataPtr);
				LEAVE_OAMSA_CMEVENT();
				return (char *)dataPtr;
			}
	}
	else if (nextAddInfo->infoType == SA_NTF_VALUE_LDAP_NAME)
	{
			void *dataPtr;
			SaUint16T dataSize;
#ifdef UNIT_TEST
			SaAisErrorT retVal = saNtfPtrValGetTest(notificationHandle,		// [IN] Handle to identify the transaction
					&(nextAddInfo->infoValue),							// [IN] offset and size of data buffer
					&dataPtr,											// [OUT] Pointer to the buffer with the data
					&dataSize, false);									// [OUT] Size of this particular data (Including null at the end of the string)
#else
			SaAisErrorT retVal = saNtfPtrValGet(notificationHandle, // [IN] Handle to identify the transaction
					&(nextAddInfo->infoValue),						// [IN] offset and size of data buffer
					&dataPtr,										// [OUT] Pointer to the buffer with the data
					&dataSize);										// [OUT] Size of this particular data (Including null at the end of the string)
#endif
			if (retVal == SA_AIS_OK)
			{
				DEBUG_OAMSA_CMEVENT("getAdditionalInfoName : Read Additional Information infoId:%i infoType:SA_NTF_VALUE_LDAP_NAME",nextAddInfo->infoId);
				// OK, in this case (ONLY!) we have to allocate memory on the heap and deallocate it when we convert the data.
				char *the3gppName;
				the3gppName = convertTo3Gpp((SaNameT *)dataPtr);
				return the3gppName;
			}
	}
	LEAVE_OAMSA_CMEVENT();
	return NULL;
}

/**
 * Read the attributeValue pointer value and returns the result as a void pointer
 *
 * @param[in] infoId 				Id of Attribute to convert
 * @param[in] changedAttributes     pointer to SaNtfAttributeChangeT containing the value of the attribute
 * @param[in] numAttributes     	Number of attributes
 *
 * @return  Number Of values found
 */
int getNumberOfAttributeValues(
						SaNtfElementIdT infoId, 						// Id of Attribute to convert
						SaNtfAttributeChangeT *changedAttributes,		// Pointer to value array to search for the value, for modification
						SaNtfAttributeT *objectAttributes,				// Pointer to value array to search for the value, for creation
						SaUint16T numAttributes)						// Total Number of attributes
{
	ENTER_OAMSA_CMEVENT();
	int nextValuePosition = 0;
	int numberOfValues = 0;
	SaNtfAttributeT *objectAttributesPointer;
	if (changedAttributes != NULL && objectAttributes == NULL)
	{
		while (nextValuePosition < numAttributes)
		{
			if (changedAttributes->attributeId == infoId)
			{
				++numberOfValues;
			}
			++nextValuePosition;
			++changedAttributes;
		}
	}
	else
	{
		objectAttributesPointer = objectAttributes;
		while (nextValuePosition < numAttributes)
		{
			if (objectAttributesPointer->attributeId == infoId)
			{
				++numberOfValues;
			}
			++nextValuePosition;
			++objectAttributesPointer;
		}
	}
	LEAVE_OAMSA_CMEVENT();
	return numberOfValues;
}

// Internal helper
void* getOneAttributeValue(
		SaNtfElementIdT infoId, 						// Id of Attribute to convert
		SaNtfNotificationHandleT notificationHandle,	// Handle of transaction
		SaNtfAttributeChangeT *changedAttributesPointer,		// Pointer to value array to search for the value, for modification
		SaNtfAttributeT *objectAttributesPointer,				// Pointer to value array to search for the value, for creation
		SaUint16T numAttributes,						// Number of attributes
		bool isobjectAttributesType)					// Type of Value to return
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("getOneAttributeValue ENTER");
	void *dataPtrInner = NULL;
	SaUint16T dataSizeInner;
	SaAisErrorT retValInner;
	SaNtfValueTypeT attributeTypeToSwitch;
	SaNtfValueT *newAttributeValueToSwitch = NULL;
	SaNtfElementIdT attributeIdToSwitch;
	if (isobjectAttributesType)
	{
		attributeTypeToSwitch = objectAttributesPointer->attributeType;
		newAttributeValueToSwitch = &(objectAttributesPointer->attributeValue);
		attributeIdToSwitch = objectAttributesPointer->attributeId;
	}
	else
	{
		attributeTypeToSwitch = changedAttributesPointer->attributeType;
		newAttributeValueToSwitch = &(changedAttributesPointer->newAttributeValue);
		attributeIdToSwitch = changedAttributesPointer->attributeId;
	}
	switch (attributeTypeToSwitch) {
	case SA_NTF_VALUE_UINT8:     /* 1 byte long - unsigned int */
	case SA_NTF_VALUE_INT8:      /* 1 byte long - signed int */
	case SA_NTF_VALUE_UINT16:    /* 2 bytes long - unsigned int */
	case SA_NTF_VALUE_INT16:     /* 2 bytes long - signed int */
	case SA_NTF_VALUE_UINT32:    /* 4 bytes long - unsigned int */
	case SA_NTF_VALUE_INT32:     /* 4 bytes long - signed int */
	case SA_NTF_VALUE_UINT64:    /* 8 bytes long - unsigned int */
	case SA_NTF_VALUE_INT64:     /* 8 bytes long - signed int */
	case SA_NTF_VALUE_DOUBLE:
	case SA_NTF_VALUE_FLOAT:
		// Only return a pointer directly into the structure
		DEBUG_OAMSA_CMEVENT("getAttributeValue : Read Attribute value infoId:%i infoType:%i ", attributeIdToSwitch, attributeTypeToSwitch);
		LEAVE_OAMSA_CMEVENT();
		return (void*)(newAttributeValueToSwitch);
		break;
	case SA_NTF_VALUE_STRING:    /* '\0' terminated char array (UTF-8 */
#ifdef UNIT_TEST
		retValInner = saNtfPtrValGetTest(notificationHandle,// [IN] Handle to identify the transaction
				(newAttributeValueToSwitch),					// [IN] offset and size of data buffer
				&dataPtrInner, 										// [OUT] Pointer to the buffer with the data
				&dataSizeInner, true); 									// [OUT] Size of this particular data (Including null at the end of the string)
#else
		retValInner = saNtfPtrValGet(notificationHandle,// [IN] Handle to identify the transaction
				(newAttributeValueToSwitch),					// [IN] offset and size of data buffer
				&dataPtrInner, 										// [OUT] Pointer to the buffer with the data
				&dataSizeInner); 									// [OUT] Size of this particular data (Including null at the end of the string)
#endif
		if (retValInner == SA_AIS_OK)
		{
			DEBUG_OAMSA_CMEVENT("getAttributeValue : Read Attribute value infoId:%i infoType:SA_NTF_VALUE_STRING InfValue:%s", attributeIdToSwitch, (char*)dataPtrInner);
			LEAVE_OAMSA_CMEVENT();
			return dataPtrInner;
		}

		break;
	case SA_NTF_VALUE_LDAP_NAME: /* SaNameT type */

#ifdef UNIT_TEST
		retValInner = saNtfPtrValGetTest(notificationHandle,// [IN] Handle to identify the transaction
				(newAttributeValueToSwitch),					// [IN] offset and size of data buffer
				&dataPtrInner, 										// [OUT] Pointer to the buffer with the data
				&dataSizeInner, true); 									// [OUT] Size of this particular data (Including null at the end of the string)
#else
		retValInner = saNtfPtrValGet(notificationHandle,// [IN] Handle to identify the transaction
				(newAttributeValueToSwitch),					// [IN] offset and size of data buffer
				&dataPtrInner, 										// [OUT] Pointer to the buffer with the data
				&dataSizeInner); 									// [OUT] Size of this particular data (Including null at the end of the string)
#endif
		if (retValInner == SA_AIS_OK)
		{
			DEBUG_OAMSA_CMEVENT("getAttributeValue : Read Attribute value infoId:%i infoType:SA_NTF_VALUE_LDAP_NAME", infoId);
			// OK, in this case (ONLY!) we have to allocate memory on the heap and deallocate it when we convert the data.
			SaNameT *atp = calloc(1, sizeof(SaNameT));
			saNameSetLen(dataPtrInner,dataSizeInner, atp); //allocate memory only for the single attribute in multivalue
			LEAVE_OAMSA_CMEVENT();
			return atp;
		}
		break;
	case SA_NTF_VALUE_IPADDRESS:
	case SA_NTF_VALUE_BINARY:
	case SA_NTF_VALUE_ARRAY:

		ERR_OAMSA_CMEVENT("getAttributeValue : IPADDRESS, BINARY and ARRAY not supported");
		break;
	default:
		ERR_OAMSA_CMEVENT("getAttributeValue : default");
		break;
	}
	DEBUG_OAMSA_CMEVENT("getOneAttributeValue LEAVE");
	LEAVE_OAMSA_CMEVENT();
	return NULL;
}

/**
 * Read the attributeValue pointer value and returns the result as a void pointer-pointer
 * for all values matching this id
 *
 * @param[out] values				Pointer to a pointer array to fill with values
 * @param[in] infoId 				Id of Attribute to convert
 * @param[in] notificationHandle 	The NTF notification handle for this 'session'
 * @param[in] changedAttributes		pointer to SaNtfAttributeChangeT containing the value of the attribute
 * @param[in] objectAttributes		pointer to SaNtfAttributeT containing the value of the attribute for create case
 * @param[in] numAttributes			Number of attributes
 * @param[in] ntfType				Attribute type
 *
 * @return  The result of the read operation
 * @retval  NOT NULL	if read went fine
 * @retval  NULL		if the read failed
 */
void getMultiAttributeValue(
		void **values,									// Pointer to an array of pointers
		SaNtfElementIdT infoId, 						// Id of Attribute to convert
		SaNtfNotificationHandleT notificationHandle,	// Handle of transaction
		SaNtfAttributeChangeT *changedAttributes,		// Pointer to value array to search for the value, for modification
		SaNtfAttributeT *objectAttributes,				// Pointer to value array to search for the value, for creation
		SaUint16T numAttributes,						// Number of attributes
		SaNtfValueTypeT *ntfType)						// Attribute type
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("getMultiAttributeValue ENTER");
	// Set a pointer to a pointer, that is an array of pointers to values for this attribute
	// Get the pointer to the value of this attribute with Id infoId
	int nextValuePosistion = 0;
	int count =0;
	bool isobjectAttributesType = false;
	SaNtfAttributeChangeT *changedAttributesPointer = NULL;
	SaNtfAttributeT *objectAttributesPointer = NULL;
	if (changedAttributes != NULL && objectAttributes == NULL)
	{
		changedAttributesPointer = changedAttributes;
		while (nextValuePosistion < numAttributes)
		{
			if (changedAttributesPointer->attributeId == infoId)
			{
				// Found one value, store the pointer
				if (ntfType != NULL) ntfType[count] = changedAttributesPointer->attributeType;
				*values = getOneAttributeValue(infoId, notificationHandle, changedAttributesPointer, objectAttributesPointer, numAttributes, isobjectAttributesType);
				++values;
				count++;
			}
			++nextValuePosistion;
			++changedAttributesPointer;
		}
	}
	else
	{
		objectAttributesPointer = objectAttributes;
		isobjectAttributesType = true;
		while (nextValuePosistion < numAttributes)
		{
			if (objectAttributesPointer->attributeId == infoId)
			{
				// Found one value, store the pointer
				if (ntfType != NULL) ntfType[count] = objectAttributesPointer->attributeType;
				*values = getOneAttributeValue(infoId, notificationHandle, changedAttributesPointer, objectAttributesPointer, numAttributes, isobjectAttributesType);
				++values;
				count++;
			}
			++nextValuePosistion;
			++objectAttributesPointer;
		}
	}
	// Get the pointer to the value of this attribute with Id infoId
	DEBUG_OAMSA_CMEVENT("getMultiAttributeValue LEAVE");
	LEAVE_OAMSA_CMEVENT();
	return;
}

/**
 * Read the attributeValue pointer value and search for the mandatory attributes (for config classes) SaImmOiCcbIdT and ccblast
 *
 * @param[out] values				Pointer to a pointer array to fill with values
 * @param[in] infoId 				Id of Attribute to convert
 * @param[in] notificationHandle 	The NTF notification handle for this 'session'
 * @param[in] changedAttributes     pointer to SaNtfAttributeChangeT containing the value of the attribute
 * @param[in] objectAttributes      pointer to SaNtfAttributeT containing the value of the attribute for create case
 * @param[in] numAttributes     	Number of attributes
 * @param[in] saAttributeName		Name of the current attribute to check for ccblast/
 *
 * @return  nothing
 */
void checkForMandatoryAttributes(cacheInformation *notifyCache,
		SaNtfElementIdT infoId, 						// Id of Attribute to convert
		SaNtfNotificationHandleT notificationHandle,	// Handle of transaction
		SaNtfAttributeChangeT *changedAttributes,		// Pointer to value array to search for the value, for modification
		SaNtfAttributeT *objectAttributes,				// Pointer to value array to search for the value, for creation
		SaUint16T numAttributes,						// Number of attributes
		char *saAttributeName )							// Name of attribute to scan for SaImmOiCcbIdT/ccblast
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("checkForMandatoryAttributes ENTER");
	if(saAttributeName == NULL)
	{
		WARN_OAMSA_CMEVENT("checkForMandatoryAttributes saAttributeName is NULL");
		DEBUG_OAMSA_CMEVENT("checkForMandatoryAttributes LEAVE");
		LEAVE_OAMSA_CMEVENT();
		return;
	}
	// SaIMMOiCcBIdT Name
	const char* SaImmOiCcbIdT = "SaImmOiCcbIdT";
	// SaIMMOiCcBIdT attributeId, if it exist.
	const SaNtfElementIdT SaImmOiCcbIdTattributeId = 2;
	// The Value of the CcbId
	SaUint64T SaImmOiCcbIdTValue = 0;
	// ccblast Name
	const char* ccbLast = "ccbLast";
	// SaIMMOiCcBIdT attributeId, if it exist.
	const SaNtfElementIdT ccbLastattributeId = 3;
	// The Value of the CcbId
	SaUint32T ccbLastValue = SA_FALSE;
	if (!strcmp(saAttributeName, SaImmOiCcbIdT))
	{
		// Ok, this is a  Object with a CcbId provided
		// Get the Value for this attribute.
		SaUint64T *value = &SaImmOiCcbIdTValue;
		getMultiAttributeValue(
				(void**)&value,						// The array to fill with values
				SaImmOiCcbIdTattributeId, 			// Id of Attribute to convert
				notificationHandle,					// Handle of transaction
				changedAttributes,					// Attributes new value, for modification
				objectAttributes,					// Attributes new value, for creation
				numAttributes, NULL);				// Number of attribute values in the array
		SaImmOiCcbIdTValue = *value;
		notifyCache->ccbId = SaImmOiCcbIdTValue;
		DEBUG_OAMSA_CMEVENT("checkForMandatoryAttributes: CcbId=>%llu", SaImmOiCcbIdTValue);
	}
	if (!strcmp(saAttributeName, ccbLast))
	{
		// Get the Value for this attribute.
		DEBUG_OAMSA_CMEVENT("checkForMandatoryAttributes: ccbLastValue before <%u>", ccbLastValue);

		SaUint32T *value = &ccbLastValue;
		getMultiAttributeValue(
				(void**)&value,						// The array to fill with values
				ccbLastattributeId, 				// Id of Attribute to convert
				notificationHandle,					// Handle of transaction
				changedAttributes,					// Attributes new value, for modification
				objectAttributes,					// Attributes new value, for creation
				numAttributes, NULL);				// Number of attribute values in the array
		ccbLastValue = *value;
		DEBUG_OAMSA_CMEVENT("checkForMandatoryAttributes: ccbLastValue is <%u>", ccbLastValue);
		notifyCache->ccbLast = ((ccbLastValue == SA_TRUE) ? true : false);
		DEBUG_OAMSA_CMEVENT("checkForMandatoryAttributes: ccbLast is <%i>", notifyCache->ccbLast);
	}
	DEBUG_OAMSA_CMEVENT("checkForMandatoryAttributes LEAVE");
	LEAVE_OAMSA_CMEVENT();
}

/**
 * Read the attributeValue pointer value and returns the result as a void pointer
 *
 * @param[in] infoId 				Id of Attribute to convert
 * @param[in] notificationHandle 	The NTF notification handle for this 'session'
 * @param[in] changedAttributes     pointer to SaNtfAttributeChangeT containing the value of the attribute
 * @param[in] numAttributes     	Number of attributes
 *
 * @return  The result of the read operation
 * @retval  NOT NULL	if read went fine
 * @retval  NULL		if the read failed
 */
void* getAttributeValue(
						SaNtfElementIdT infoId, 						// Id of Attribute to convert
						SaNtfNotificationHandleT notificationHandle,	// Handle of transaction
						SaNtfAttributeChangeT *changedAttributes,		// Pointer to value array to search for the value, for modification
						SaNtfAttributeT *objectAttributes,				// Pointer to value array to search for the value, for creation
						SaUint16T numAttributes)						// Number of attributes
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("getAttributeValue ENTER");
	// Get the pointer to the value of this attribute with Id infoId
	int nextValuePosistion = 0;
	bool foundValue = false;
	bool isobjectAttributesType = false;
	SaNtfAttributeChangeT *changedAttributesPointer = NULL;
	SaNtfAttributeT *objectAttributesPointer = NULL;
	if (changedAttributes != NULL && objectAttributes == NULL)
	{
		changedAttributesPointer = changedAttributes;
		while (nextValuePosistion < numAttributes)
		{
			// Ok, continue search for our value, first check if there is a next value to read
			if (changedAttributesPointer->attributeId == infoId)
			{
				foundValue = true;
				break;
			}
			++nextValuePosistion;
			++changedAttributesPointer;
		}
	}
	else
	{
		objectAttributesPointer = objectAttributes;
		isobjectAttributesType = true;
		while (nextValuePosistion < numAttributes)
		{
			// Ok, continue search for our value, first check if there is a next value to read
			if (objectAttributesPointer->attributeId == infoId)
			{
				foundValue = true;
				break;
			}
			++nextValuePosistion;
			++objectAttributesPointer;
		}
	}
	DEBUG_OAMSA_CMEVENT("getAttributeValue LEAVE");
	LEAVE_OAMSA_CMEVENT();
	if (foundValue)
		return getOneAttributeValue(infoId, notificationHandle, changedAttributesPointer, objectAttributesPointer, numAttributes, isobjectAttributesType);
	else
		return NULL;
}

/**
 * set the Value in the container for named attribute or action attribute values
 *
 * @param[in] Pointer 		pointer to a MafMoAttributeValue_3T.
 * @param[in] type			the type of the value.
 * @param[in] value			pointer to the value provided from Ntf.
 *
 * @return  The result of the set operation
 * @retval  true     if conversion is allowed and successful
 * @retval  false	Otherwise
 */
bool setValueInCmEvent( MafMoAttributeValue_3T* Pointer,
						MafOamSpiMoAttributeType_3T type,
						void* value,SaNtfValueTypeT ntfType)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("setValueInCmEvent ENTER type (%d)", (int) type);
	bool returnValue = true;
	SaNameT* atp = NULL;
	SaNtfValueT* pointerValue = (SaNtfValueT*)value;
	// Sanity check
	switch (type) {
	/**
	* An 8-bit integer.
	*/
	case    MafOamSpiMoAttributeType_3_INT8:
		Pointer->value.i8 = pointerValue->int8Val;
		break;
		/**
		* A 16-bit integer.
		*/
	case    MafOamSpiMoAttributeType_3_INT16:
		Pointer->value.i16 = pointerValue->int16Val;
		break;
		/**
		* A 32-bit integer.
		*/
	case    MafOamSpiMoAttributeType_3_INT32:
	case    MafOamSpiMoAttributeType_3_BOOL:
	case    MafOamSpiMoAttributeType_3_ENUM:
		Pointer->value.i32 = pointerValue->int32Val;
		break;
		/**
		* A 64-bit integer.
		*/
	case    MafOamSpiMoAttributeType_3_INT64:
		Pointer->value.i64 = pointerValue->int64Val;
		break;
		/**
		* An 8-bit unsigned integer.
		*/
	case    MafOamSpiMoAttributeType_3_UINT8:
		Pointer->value.u8 = pointerValue->uint8Val;
		break;
		/**
		* A 16-bit unsigned integer.
		*/
	case    MafOamSpiMoAttributeType_3_UINT16:
		Pointer->value.u16 = pointerValue->uint16Val;
		break;
		/**
		* A 32-bit unsigned integer.
		*/
	case    MafOamSpiMoAttributeType_3_UINT32:
		Pointer->value.u32 = pointerValue->uint32Val;
		break;
		/**
		* A 64-bit unsigned integer.
		*/
	case    MafOamSpiMoAttributeType_3_UINT64:
		Pointer->value.u64 = pointerValue->uint64Val;
		break;
		/**
		* A string value.
		*/
	case    MafOamSpiMoAttributeType_3_STRING:
		if (value != NULL)
		{
			Pointer->value.theString = (const char*)calloc(strlen(value)+1,  sizeof(char));
			strcpy((char*)Pointer->value.theString, value);
                        DEBUG_OAMSA_CMEVENT("setValueInCmEvent : read data <%s>", Pointer->value.theString);
		}
		else
		{
			// OK, set the theString to NULL, calloc do this automatic
			Pointer->value.theString = (const char*)calloc(1, sizeof(char));
		}
		break;
	case    MafOamSpiMoAttributeType_3_STRUCT:
		/**
		* A reference to another Managed Object (MO) class.
		*/
		// the SANAMET type comes here, which is handled a bit special.
		/*
		* NOTE, in this case the memory is allocated on the HEAP and must be deallocated here!
		*/
	case    MafOamSpiMoAttributeType_3_REFERENCE:
		/**
		* A struct or aggregated data type.
		*/
		atp = (SaNameT*) value;
		if (atp != NULL)
		{
			if (!convertTo3GPPDN(atp, (char**) &(Pointer->value.moRef))) {
				Pointer->value.moRef = (const char*)calloc(saNameLen(atp)+1, sizeof(char));
				memcpy((char*)(Pointer->value.moRef), saNameGet(atp), saNameLen(atp));
				// Since it is declared as a const char I need to do some casting here.
				((char*)(Pointer->value.moRef))[saNameLen(atp)] = '\0';
				ERR_OAMSA_CMEVENT("setValueInCmEvent : Failed to convert MOREF attribute value <%s> from IMMDN to 3GPPDN", Pointer->value.moRef);
			}
			else
			{
				DEBUG_OAMSA_CMEVENT("setValueInCmEvent : After 3GPPDN conversion MOREF attribute value <%s>", Pointer->value.moRef);
			}
			saNameDelete((SaNameT*) value, false);
			free(value);
		}
		else
		{
			// OK, set the moRef to NULL, calloc do this automatic
			Pointer->value.moRef = (const char*)calloc(1, sizeof(char));
		}
		break;
	case    MafOamSpiMoAttributeType_3_DECIMAL64:
		/**
		* A 64 bits floating point value
		*/
		if(ntfType == SA_NTF_VALUE_DOUBLE){
			Pointer->value.decimal64 = pointerValue->doubleVal;
		}
		else if(ntfType == SA_NTF_VALUE_FLOAT){
			Pointer->value.decimal64 = (double)pointerValue->floatVal;
		}
		else {returnValue = false;}
		break;
	default:
		WARN_OAMSA_CMEVENT("setValueInCmEvent reached default statement with type %i", type);
		returnValue = false;
		break;
	}
	DEBUG_OAMSA_CMEVENT("setValueInCmEvent LEAVE");
	LEAVE_OAMSA_CMEVENT();
	return returnValue;
}

char* getClassNameOfNotification(
		SaNtfNotificationHandleT notificationHandle,	// Handle of transaction
		SaNtfAttributeChangeT *changedAttributes,		// Pointer to value array to search for the value, for modification
		SaNtfAttributeT *objectAttributes,				// Pointer to value array to search for the value, for creation
		SaUint16T numAttributes)						// Number of attributes
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("getClassNameOfNotification ENTER");
	// Get the attribute Id.
	SaNtfElementIdT attributeId;
	attributeId = 1;
	// Get the value for this
	DEBUG_OAMSA_CMEVENT("getClassNameOfNotification attributeID=<%i>", attributeId);
	LEAVE_OAMSA_CMEVENT();
	return ((char*)getAttributeValue(
			attributeId, 									// Id of Attribute to convert
			notificationHandle,								// Handle of transaction
			changedAttributes,								// Attributes new value, for modification
			objectAttributes,								// Attributes new value, for creation
			numAttributes));									// Number of attribute values in the array

}

// NOTe this internal method returns memory that is allocated on the heap and must therefore be freed by a call to the free( .. ) method
MafMoAttributeValueContainer_3T* getAttributeValuesContainerValue(
		SaNtfElementIdT infoId, 						// Id of Attribute to convert
		SaNtfNotificationHandleT notificationHandle,	// Handle of transaction
		SaNtfAttributeChangeT *changedAttributes,		// Pointer to value array to search for the value, for modification
		SaNtfAttributeT *objectAttributes,				// Pointer to value array to search for the value, for creation
		SaUint16T numAttributes,						// Number of attributes
		char* immRdn,									// Imm RDN
		char *saAttributeName,							// Attribute name to convert
		bool isStruct,									// Flag that is true if this is a struct memebr that we convert
		char *structAttributeName)						// Valid if isStruct is true
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("getAttributeValuesContainerValue ENTER");
	MafMoAttributeValueContainer_3T *value = calloc(1, sizeof(MafMoAttributeValueContainer_3T));
	// Number of values
	int nrOfValues = getNumberOfAttributeValues(
			infoId,								// Id of Attribute to convert
			changedAttributes,					// Pointer to value array to search for the value, for modification
			objectAttributes,					// Pointer to value array to search for the value, for creation
			numAttributes);						// Total Number of attributes
	// Set (and Get) the Value array.
	void *values[nrOfValues+1];
	SaNtfValueTypeT ntfAttTypes [nrOfValues];
	getMultiAttributeValue(
			(void**)&values,					// The array to fill with values
			infoId,								// Id of Attribute to convert
			notificationHandle,					// Handle of transaction
			changedAttributes,					// Attributes new value, for modification
			objectAttributes,					// Attributes new value, for creation
			numAttributes,						// Number of attribute values in the array
			ntfAttTypes);						// Attribute type
	// Convert to the type we expect it to be (Go for MAF type directly ??
	if (isStruct)
		value->type = getTypeForStructMemberAttribute(structAttributeName, saAttributeName, immRdn);
	else
		value->type = getTypeForAttribute(saAttributeName, immRdn);

	if (0 == (int)value->type) {
		WARN_OAMSA_CMEVENT ("getAttributeValuesContainerValue(): returning NULL. value->type is 0");
		return NULL;
	}

	if ((values != NULL) && (nrOfValues != 0))
	{
		value->values = calloc(nrOfValues, sizeof(MafMoAttributeValue_3T));
		value->nrOfValues = nrOfValues;
		int loopCounter = 0;
		while (loopCounter < (nrOfValues))
		{
			if (!setValueInCmEvent(&(value->values[loopCounter]), value->type, values[loopCounter], ntfAttTypes[loopCounter]))
			{
				// Failed to convert value
				ERR_OAMSA_CMEVENT("getAttributeValuesContainerValue: Failed to convert Ntf value to a CM event");
				LEAVE_OAMSA_CMEVENT();
				return value;
			}
			++loopCounter;				// Set the the value according to the type.
		}
	}
	else
	{
		// No value forwarded, forward as is, this is a attribute that is deleted and assigned no value
		value->nrOfValues = 0;
		value->values = NULL;
	}
	DEBUG_OAMSA_CMEVENT("getAttributeValuesContainerValue(): value.type = (%d)",value->type);
	DEBUG_OAMSA_CMEVENT("getAttributeValuesContainerValue(): value.nrOfValues = (%u)",value->nrOfValues);
	DEBUG_OAMSA_CMEVENT("getAttributeValuesContainerValue(): value.values = (%p)",value->values);
	LEAVE_OAMSA_CMEVENT();
	return value;
}

/**
 * Translates NTF attributes format to the MAF attributes format.
 *
 * @param[in] notificationHandle The NTF notification handle for this 'session'
 * @param[in] numAdditionalInfo  Number of entries in the SaNtfAdditionalInfoT array
 * @param[in] additionalInfo     pointer to SaNtfAdditionalInfoT array containing the name of the attributes
 * @param[in] numAttributes      Number of entries in the SaNtfAttributeT array
 * @param[in] changedAttributes  pointer to SaNtfAttributeChangeT array containing the value of the attributes
 * @param[in] objectAttributes   pointer to SaNtfAttributeT array containing the value of the attributes
 * @param[in] immRdn			IMM RDN value for the notifying object
 * @param[out] notifyCache		The value of the CcbId/ccblasr for this call back
 *
 * @param[in] isCreate			The value is true if this is a MoCreate notification, else false
 *
 *
 * @return  The result of the conversion of an error code
 * @retval  true      if conversion went fine
 * @retval  false     if the conversion failed
 */
bool transformAttributeList(SaNtfNotificationHandleT notificationHandle,
		SaUint16T numAdditionalInfo,
		SaNtfAdditionalInfoT *additionalInfo,
		SaUint16T numAttributes,
		// One of this but not both!
		SaNtfAttributeChangeT *changedAttributes,
		SaNtfAttributeT *objectAttributes,
		char *immRdn,
		cacheInformation *notifyCache,
		bool isCreate)
{
	ENTER_OAMSA_CMEVENT();
	bool returnValue = false;
	MafReturnT result = MafOk;
	DEBUG_OAMSA_CMEVENT("transformAttributeList: ENTER");
	if (changedAttributes != NULL && objectAttributes != NULL)
	{
		// Serious problems here
		ERR_OAMSA_CMEVENT("transformAttributeList: Received inconsistent data");
		LEAVE_OAMSA_CMEVENT();
		return false;
	}
	// Get the second mandatory additionalInfo fields
	// Second one is saImmAttrClassName
	SaNtfAdditionalInfoT *nextAddInfo = NULL;
	// Get the value for this
	char *className = getClassNameOfNotification(
			notificationHandle,								// Handle of transaction
			changedAttributes,								// Attributes new value, for modification
			objectAttributes,								// Attributes new value, for creation
			numAttributes);									// Number of attribute values in the array
	if (className == NULL)
	{
		// OK, we can not do anything with this notification since it is unknown to us
		ERR_OAMSA_CMEVENT("transformAttributeList: Received notification without any class name specified");
		LEAVE_OAMSA_CMEVENT();
		return false;
	}
	DEBUG_OAMSA_CMEVENT("transformAttributeList: ClassName <%s>", className);
	if (isCreate && isNotified(NULL, className, immRdn))
	{
		// Class is notifed for create notifications
		returnValue = true;
	}
	// Number of attributes to Notify
	int numberOfAttributesToNotify = 0;
	// Allocated pointers to the names of the attributes that shall be notified
	// Set them all to NULL, then change them to the name if they are notifyable
	char *AttributeList[numAdditionalInfo];
	// Set up the list with Id of Attribute to convert
	// Set them to zero as unused
	SaNtfElementIdT infoIdList[numAdditionalInfo];
	int count = 0;
	for (count = 0; count < numAdditionalInfo; ++count)
	{
		AttributeList[count] = NULL;
		infoIdList[count] = 0;
	}

	// Iterate over the attribute list and check if they are to be notified
	int i = 0;
	DEBUG_OAMSA_CMEVENT("transformAttributeList: numAdditionalInfo=<%hu>", numAdditionalInfo);
	for (i=2; i<(numAdditionalInfo); i++)
	{
		DEBUG_OAMSA_CMEVENT("transformAttributeList: processing attribute number=<%i>", i);
		nextAddInfo = (additionalInfo+i);
		if (nextAddInfo != NULL && nextAddInfo->infoType == SA_NTF_VALUE_STRING)
		{
			char *saAttributeName = getAdditionalInfoName(notificationHandle, nextAddInfo);
			DEBUG_OAMSA_CMEVENT("transformAttributeList: attribute name=>%s", saAttributeName);
			// Check for ccblast and SaImmOiCcbIdT values
			checkForMandatoryAttributes(notifyCache,
					nextAddInfo->infoId, 							// Id of Attribute to convert
					notificationHandle,								// Handle of transaction
					changedAttributes,								// Pointer to value array to search for the value, for modification
					objectAttributes,								// Pointer to value array to search for the value, for creation
					numAttributes,									// Number of attributes
					saAttributeName );								// Name of attribute to scan for SaImmOiCcbIdT/ccblast
			if ((saAttributeName != NULL) && isNotified(saAttributeName, className, immRdn))
			{
				returnValue = true;
				// OK a Simple type of attribute found
				DEBUG_OAMSA_CMEVENT("transformAttributeList: Add attribute <%s> to Notified list", saAttributeName);
				DEBUG_OAMSA_CMEVENT("transformAttributeList: Add InfoId <%i> to Notified list", nextAddInfo->infoId);
				// OK add to list to notify
				AttributeList[i] = saAttributeName;
				// Add the InfoId
				infoIdList[i] = nextAddInfo->infoId;
				// Add to the counter of the size needed for the CM event struct
				++numberOfAttributesToNotify;
			}
		}
	}
	if(isDiscardedCcb(notifyCache->ccbId)) {
		return false;
	}
	DEBUG_OAMSA_CMEVENT("transformAttributeList: Number of Attributes to Notify <%i>", numberOfAttributesToNotify);
	// Loop over everything and send over to the cache
	for (i=2; i< numAdditionalInfo; i++)
	{
		if ((AttributeList[i] != NULL) && (infoIdList[i] != 0))
		{
			if (notifyCache->ccbId == 0)
			{
				// OK, runtime object here, check if it is a struct attribute that we face
				if ( isStructAttribute(immRdn, AttributeList[i]) )
				{
					// OK, handle struct separate here by reading it's value from IMM and send it separate to the Cache
					MafMoAttributeValueContainer_3T *structAttributePointer = calloc(1, sizeof(MafMoAttributeValueContainer_3T));
					// OK, we need to call the cache here
					char *structAttributeName = getStructAttributeName(immRdn, AttributeList[i]);
					if ( fillStructMembers(&structAttributePointer, immRdn, AttributeList[i]) )
					{
						/* Check in case of struct(s) provided (0 < nrOfValues), if they have their values in place.
						* e.g.: if nrOfValues=1 and values[0].value.structMember=NULL, then modify it to nrOfValues=0
						* This special case can happen when only the object created in IMM which contains the reference, but not the referenced object(struct object).
						* e.g.: in a runtime struct create case
						* If any of the structs is NULL then set nrOfValues=0.
						* Don't rearrange the values, since COM SA needs to provide the full struct array together and not just part of it.
						*/
						if(structAttributePointer->type == MafOamSpiMoAttributeType_3_STRUCT)
						{
							bool emptyStruct = false;
							unsigned int j;
							for(j = 0; j != structAttributePointer->nrOfValues; j++)
							{
								// Check if the list is empty by checking the first element in the linked list.
								if(structAttributePointer->values[j].value.structMember == NULL)
								{
									emptyStruct = true;
									break;
								}
							}
							if(emptyStruct)
							{
								DEBUG_OAMSA_CMEVENT("transformAttributeList: empty struct found");
								for(j = 0; j != structAttributePointer->nrOfValues; j++)
								{
									if(structAttributePointer->values[j].value.structMember != NULL)
									{
										DEBUG_OAMSA_CMEVENT("transformAttributeList: empty struct case: freeing memory of structAttributePointer->values[%u].value.structMember",j);
										struct MafMoAttributeValueStructMember_3 *SM = structAttributePointer->values[j].value.structMember;
										while(SM != NULL)
										{
											DEBUG_OAMSA_CMEVENT("transformAttributeList: empty struct case: freeing SM->memberName = (%s)",SM->memberName);
											free(SM->memberName);
											freeAVC(SM->memberValue);
											DEBUG_OAMSA_CMEVENT("transformAttributeList: empty struct case: freeing SM->memberValue");
											free(SM->memberValue);
											struct MafMoAttributeValueStructMember_3 *nextSM = SM->next;
											free(SM);
											SM =  nextSM;
										}
									}
								}
								DEBUG_OAMSA_CMEVENT("transformAttributeList: empty struct case: freeing memory of structAttributePointer->values");
								free((MafMoAttributeValue_3T*)structAttributePointer->values);
								DEBUG_OAMSA_CMEVENT("transformAttributeList: empty struct case: setting nrOfValues to 0");
								structAttributePointer->nrOfValues = 0;
							}
						}
					}
					// Handling the negative case here
					else
					{
						// send an empty struct to the Cache
						structAttributePointer->nrOfValues=0;
						structAttributePointer->type=MafOamSpiMoAttributeType_3_STRUCT;
						structAttributePointer->values=NULL;
					}						// Call the cache
					// Send the value to the cache
					result = addToCmCache(notifyCache->ccbId,
							structAttributeName,
							immRdn,
							NULL,
							structAttributePointer);
#ifndef UNIT_TEST
					freeConvertCMResources(structAttributeName);
#endif
					if(MafNoResources == result) {
						//if unable to add attribute to CM Cache, discard and return false.
						returnValue = false;
						break;
					}
					// OK, continue with the next attribute.
					continue;
				}
				// Else, do as we always do for simple attributes here
			}
			// OK this attribute shall be notified
			// Get the value for this attribute
			MafMoAttributeValueContainer_3T* AttributeValue = NULL;
			AttributeValue = getAttributeValuesContainerValue(
					infoIdList[i],
					notificationHandle,
					changedAttributes,
					objectAttributes,
					numAttributes,
					immRdn,
					AttributeList[i],
					false, NULL);
			// Call the cache
			// Send the value to the cache
			DebugDumpAttributeContainer(AttributeValue);
			result = addToCmCache(notifyCache->ccbId,
					AttributeList[i],
					immRdn,
					NULL,
					AttributeValue);
			if(MafNoResources == result) {
				//if unable to add attribute to CM Cache, discard and return false.
				returnValue = false;
				break;
			}
		}
	}
	LEAVE_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("transformAttributeList: RETURNS true");
	return returnValue;
}


/**
 * Translates NTF attributes format to the MAF attributes format for structs
 * And notify the Cache if needed.
 * @param[in] notificationHandle The NTF notification handle for this 'session'
 * @param[in] numAdditionalInfo  Number of entries in the SaNtfAdditionalInfoT array
 * @param[in] additionalInfo     pointer to SaNtfAdditionalInfoT array containing the name of the attributes
 * @param[in] numAttributes      Number of entries in the SaNtfAttributeT array
 * @param[in] changedAttributes  pointer to SaNtfAttributeChangeT array containing the value of the attributes
 * @param[in] objectAttributes   pointer to SaNtfAttributeT array containing the value of the attributes
 * @param[in] immRdn			IMM RDN value for the notifying object
 * @param[out] notifyCache		a pointer to a cacheInformation struct that tells the caller of this function if the cache has been sent any struct attribute fragments
 *
 * @return  The result of the conversion of an error code
 * @retval  true      if conversion went fine
 * @retval  false     if the conversion failed
 */
bool transformAttributeListStruct(SaNtfNotificationHandleT notificationHandle,
		SaUint16T numAdditionalInfo,
		SaNtfAdditionalInfoT *additionalInfo,
		SaUint16T numAttributes,
		// One of this but not both!
		SaNtfAttributeChangeT *changedAttributes,
		SaNtfAttributeT *objectAttributes,
		char *immRdn,
		cacheInformation *notifyCache)
{
	ENTER_OAMSA_CMEVENT();
	bool returnValue = false;
	MafReturnT result = MafOk;
	DEBUG_OAMSA_CMEVENT("transformAttributeListStruct: ENTER");
	if (changedAttributes != NULL && objectAttributes != NULL)
	{
		// Serious problems here
		ERR_OAMSA_CMEVENT("transformAttributeListStruct: Received inconsistent data");
		LEAVE_OAMSA_CMEVENT();
		return false;
	}
	// Get the second mandatory additionalInfo fields
	// Second one is saImmAttrClassName
	SaNtfAdditionalInfoT *nextAddInfo = NULL;
	// Get the value for this
	char *className = getClassNameOfNotification(
			notificationHandle,								// Handle of transaction
			changedAttributes,								// Attributes new value, for modification
			objectAttributes,								// Attributes new value, for creation
			numAttributes);									// Number of attribute values in the array
	if (className == NULL)
	{
		// OK, we can not do anything with this notification since it is unknown to us
		ERR_OAMSA_CMEVENT("transformAttributeListStruct: Received notification without any class name specified");
		LEAVE_OAMSA_CMEVENT();
		return false;
	}
	DEBUG_OAMSA_CMEVENT("transformAttributeListStruct: ClassName <%s>", className);
	// Now we must check if this notification concerns a struct attribute that belongs to a class that is a member of a MOM
	// that shall be notified. This must be done to avoid the case where we receive a NTF callback concerning a class/object
	// that shall be notified and the call back contains ccbLast=true, but NO attribute that is notifiable
	char *comClassName=NULL;
	if (getClassName(immRdn, &comClassName))
	{
		DEBUG_OAMSA_CMEVENT("transformAttributeListStruct: Received a struct attribute for class<%s>", comClassName);
		if (isNotified(NULL, comClassName, immRdn))
		{
			DEBUG_OAMSA_CMEVENT("transformAttributeListStruct: Is Notified class=<%s>", comClassName);
			returnValue = true;
		}
		// free allocated resources
#ifndef UNIT_TEST
		freeConvertCMResources(comClassName);
#endif
	}
	// Iterate over the attribute list and check if they are to be notified
	int i = 0;
	for (i=2; i< numAdditionalInfo; i++)
	{
		nextAddInfo = (additionalInfo+i);
		if (nextAddInfo != NULL && nextAddInfo->infoType == SA_NTF_VALUE_STRING)
		{
			char *saAttributeName = getAdditionalInfoName(notificationHandle, nextAddInfo);
			// Check if this is a cached Runtime Object or a Config Object
			// Runtime Object   --> No SaImmOiCcbIdT attribute
			// Config Attribute --> SaImmOiCcbIdT attribute is provided
			DEBUG_OAMSA_CMEVENT("transformAttributeListStruct: attribute name=>%s", saAttributeName);
			// Check for ccblast and SaImmOiCcbIdT values
			checkForMandatoryAttributes(notifyCache,
					nextAddInfo->infoId, 							// Id of Attribute to convert
					notificationHandle,								// Handle of transaction
					changedAttributes,								// Pointer to value array to search for the value, for modification
					objectAttributes,								// Pointer to value array to search for the value, for creation
					numAttributes,									// Number of attributes
					saAttributeName );								// Name of attribute to scan for SaImmOiCcbIdT/ccblast
			if(isDiscardedCcb(notifyCache->ccbId)) {
				returnValue = false;
				break;
			}
			if ((saAttributeName != NULL) && isNotified(saAttributeName, className, immRdn))
			{
				// Check if this is a runtime struct attribute, check the CCBId equal to zero.
				// Then handle this separately
				// Does immRdn starts with id or is it a struct reference directly?
				if ( (strncmp(immRdn, "id=",3) == 0) || isStructAttribute(immRdn, saAttributeName))
				{
					returnValue = true;
					// OK, we have to create a the struct CMEvent
					// Get the struct reference from the parent object
					// Create a value container for this struct attribute
					if (notifyCache->ccbId != 0)
					{
						// OK, we need to call the cache here
						char *structAttributeName = getStructAttributeName(immRdn, saAttributeName);
						if(structAttributeName != NULL)
						{
							DEBUG_OAMSA_CMEVENT("transformAttributeListStruct: structName is <%s>", structAttributeName);
						}
						//since, we're in a loop, we need to reset this everytime. just a precaution though not needed.
						result = MafOk;
						if (structAttributeName != NULL && strcmp(structAttributeName,saAttributeName))
						{
							// MemberName is saAttributeName
							DEBUG_OAMSA_CMEVENT("transformAttributeListStruct: MemberName is <%s>", saAttributeName);
							// Get the value for this attribute
							MafMoAttributeValueContainer_3T* AttributeValue = NULL;
							AttributeValue = getAttributeValuesContainerValue(
									nextAddInfo->infoId, 				// Id of Attribute to convert
									notificationHandle,					// Handle of transaction
									changedAttributes,					// Pointer to value array to search for the value, for modification
									objectAttributes,					// Pointer to value array to search for the value, for creation
									numAttributes,						// Number of attributes
									immRdn,								// Imm RDN
									saAttributeName, true, structAttributeName);					// Attribute name to convert
							// Send the value to the cache
							result = addToCmCache(notifyCache->ccbId,
									structAttributeName,
									immRdn,
									saAttributeName,
									AttributeValue);
						}
#ifndef UNIT_TEST
						freeConvertCMResources(structAttributeName);
#endif
						if(MafNoResources == result) {
							//if unable to add attribute to CM Cache, discard and return false.
							returnValue = false;
							break;
						}
					}
					else
					{
						// OK, this is a runtime struct case,
						// read the data from the IMM before sending it to the cache with the new API
						MafMoAttributeValueContainer_3T *structAttributePointer = calloc(1, sizeof(MafMoAttributeValueContainer_3T));
						char *structAttributeName = getStructAttributeName(immRdn, saAttributeName);
						if ( !fillStructMembers(&structAttributePointer, immRdn, saAttributeName))
						{
							// no Object in IMM, send an empty struct to the Cache
							structAttributePointer->nrOfValues=0;
							structAttributePointer->type=MafOamSpiMoAttributeType_3_STRUCT;
							structAttributePointer->values=NULL;
						}
						//since, we're in a loop, we need to reset this everytime. just a precaution though not needed.
						result = MafOk;
						// Send the value to the cache
						result = addToCmCache(notifyCache->ccbId,
								structAttributeName,
								immRdn,
								NULL,
								structAttributePointer);
#ifndef UNIT_TEST
						freeConvertCMResources(structAttributeName);
#endif
						if(MafNoResources == result) {
							//if unable to add attribute to CM Cache, discard and return false.
							returnValue = false;
							break;
						}
					}
				}
			}
		}
	}
	DEBUG_OAMSA_CMEVENT("transformAttributeListStruct LEAVE");
	LEAVE_OAMSA_CMEVENT();
	return returnValue;
}

void freeConvertCMResources(char *immRdn)
{
	ENTER_OAMSA_CMEVENT();
	if(immRdn != NULL)
	{
		DEBUG_OAMSA_CMEVENT("freeConvertCMResources(): freeing memory");
		free(immRdn);
	}
	LEAVE_OAMSA_CMEVENT();
}

/**
 * Converts CM notifications from Ntf-style to MAF CM Event notification style
 *
 * @param[in] type          type of event to convert
 * @param[in] nh            pointer to NTF notification data
 * @param[in] eventtime		time when the event was received by com_sa
 * @param[in,out] mafNot    pointer to MAF CM notification data header
 *
 * @return  The result of the conversion of an error code
 * @retval  true           if conversion went fine
 * @retval  false          in all other cases
 */
bool convertCMNotificationHeader(const SaNtfNotificationTypeT type,
										const SaNtfNotificationsT *nh)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("convertCMNotificationHeader ENTER type (%d)", (int)type);
	assert(nh != NULL);
	cacheInformation notifyCache = {0, false};
	// Admin Owner of operation
	char *AdminOwner = NULL;
	// Set the source Indicator
	SaNtfSourceIndicatorT *theSourceIndicator;
	// Create the CM Event Struct
	char *immRdn = NULL;
	// This flag is to true if the source of this change is OAMSA
	bool isCOMSAAdminOwner = false;
	SaNameT *no;
	// Flag to indicate delete call back
	bool isDeleteObject = false;
	MafOamSpiCmEvent_EventType_1T eventType;
	// Flag to indicate that a setDnInCmCache( .. ) to the cache is needed
	bool sendsetDnInCmCache = false;
	switch (type) {
	case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		theSourceIndicator = nh->notification.objectCreateDeleteNotification.sourceIndicator;
		no = nh->notification.objectCreateDeleteNotification.notificationHeader.notificationObject;

		if (no != NULL && saNameLen(no) != 0)
		{
			immRdn = calloc(saNameLen(no)+1, sizeof(char));
			strncpy(immRdn, saNameGet(no), saNameLen(no));
		}
		SaNtfAttributeT *objectAttributes = nh->notification.objectCreateDeleteNotification.objectAttributes;
		SaNtfNotificationHandleT notificationHandle = nh->notification.objectCreateDeleteNotification.notificationHandle;
		// Get the attribute list as it is called in the design document
		SaUint16T numAttributes = nh->notification.objectCreateDeleteNotification.numAttributes;

		// if objectAttributes is not NULL: configuration object case
		// else: it must be a runtime object case and then isCOMSAAdminOwner false as default
		if(objectAttributes != NULL)
		{
			// Get the first mandatory attribute[0]
			AdminOwner = getAttributeValue(
					objectAttributes->attributeId, 		// Id of Attribute to convert
					notificationHandle,					// Handle of transaction
					NULL,								// Attributes new value, for modification
					objectAttributes,					// Attributes new value, for creation
					numAttributes);						// Number of attribute values in the array
			if ((AdminOwner != NULL) && (strncmp(AdminOwner, "OAMSA", 5) == 0))
			{
				// OK, OAMSA is the source of this change
				isCOMSAAdminOwner = true;
			}
		}
		if (*(nh->notification.objectCreateDeleteNotification.notificationHeader.eventType) == SA_NTF_OBJECT_CREATION)
		{
			eventType = MafOamSpiCmEvent_MoCreated_1;
			// Get the additionalInof list
			SaUint16T numAdditionalInfo = nh->notification.objectCreateDeleteNotification.notificationHeader.numAdditionalInfo;
			SaNtfAdditionalInfoT *additionalInfo = nh->notification.objectCreateDeleteNotification.notificationHeader.additionalInfo;

			// Forward the attributes to the Cache, they are either a structs or simple types
			if (strncmp(immRdn, "id=",3) == 0)
			{
				// Handle the config case struct
				sendsetDnInCmCache = transformAttributeListStruct(notificationHandle, numAdditionalInfo, additionalInfo,
														numAttributes, NULL, objectAttributes, immRdn, &notifyCache);
			}
			else
			{
				// Send config and runtime simple types to the cache here
				sendsetDnInCmCache = transformAttributeList(notificationHandle, numAdditionalInfo, additionalInfo,
						numAttributes, NULL, objectAttributes, immRdn, &notifyCache, true);
			}
		}
		else
		{
			eventType = MafOamSpiCmEvent_MoDeleted_1;
			// OK in this case we do not have a class name, only a DN
			// Get the class name and then forward to the cache
			DEBUG_OAMSA_CMEVENT("convertCMNotificationHeader: Received a SA_NTF_OBJECT_DELETION event");
			char* className;
			// Get the class name from the immRdn
			if (getClassName(immRdn, &className))
			{
				DEBUG_OAMSA_CMEVENT("convertCMNotificationHeader: ClassName=<%s>", className);
				if (isNotified(NULL, className, immRdn))
				{
					// OK, is this a runtime or a config object?
					if (numAttributes != 0)
					{
						// OK, CONFIG Object
						// Read the CCBID and the CCBLAST
						if(objectAttributes != NULL)
						{
							// Get the ccbLast Mandatory attribute
							// In position 2
							SaUint32T *value = getAttributeValue(
									(objectAttributes+2)->attributeId, 	// Id of Attribute to convert
									notificationHandle,					// Handle of transaction
									NULL,								// Attributes new value, for modification
									objectAttributes,					// Attributes new value, for creation
									numAttributes);						// Number of attribute values in the array
							if ( value != NULL)
							{
								if (((*value) == SA_TRUE))
									notifyCache.ccbLast = true;
								else
									notifyCache.ccbLast = false;
							}
							// Get the ccbId Mandatory attribute
							// In position 1
							SaUint64T *SaImmOiCcbIdTValue= getAttributeValue(
									(objectAttributes+1)->attributeId, 	// Id of Attribute to convert
									notificationHandle,					// Handle of transaction
									NULL,								// Attributes new value, for modification
									objectAttributes,					// Attributes new value, for creation
									numAttributes);						// Number of attribute values in the array
							if ( SaImmOiCcbIdTValue != NULL)
							{
								notifyCache.ccbId = *SaImmOiCcbIdTValue;
							}
							// Now send the set call for this event, it will happen further down in the code.
						}
						else
						{
							// OK, we have a bad situation here, no objectAttributes provided
							DEBUG_OAMSA_CMEVENT("convertCMNotificationHeader(): No ccbId/ccbLast provided for MafOamSpiCmEvent_MoDeleted_1");
							LEAVE_OAMSA_CMEVENT();
							return false;
						}

					}
					// OK runtime case here, no ccbID or ccblast available, use default values in the set call.
				}
				isDeleteObject = true;
				sendsetDnInCmCache = !isDiscardedCcb(notifyCache.ccbId);
				free((void*)(className));
			}
		}
		break;
	case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		eventType = MafOamSpiCmEvent_AttributeValueChange_1;
		theSourceIndicator = nh->notification.attributeChangeNotification.sourceIndicator;
		no = nh->notification.attributeChangeNotification.notificationHeader.notificationObject;
		if (no != NULL && saNameLen(no) != 0)
		{
			immRdn = calloc(saNameLen(no)+1, sizeof(char));
			strncpy(immRdn, saNameGet(no), saNameLen(no));
		}
		// Get the additionalInof list
		SaUint16T numAdditionalInfo = nh->notification.attributeChangeNotification.notificationHeader.numAdditionalInfo;
		SaNtfAdditionalInfoT *additionalInfo = nh->notification.attributeChangeNotification.notificationHeader.additionalInfo;
		// Get the attribute list as it is called in the design document
		numAttributes = nh->notification.attributeChangeNotification.numAttributes;
		SaNtfAttributeChangeT *changedAttributes = nh->notification.attributeChangeNotification.changedAttributes;
		notificationHandle = nh->notification.attributeChangeNotification.notificationHandle;

		// if changedAttributes is not NULL: configuration object case
		// else: runtime object case
		if(changedAttributes != NULL)
		{
			// Check if the attribute[0] is equal to COMSA Admin owner name, something like OAMSA38
			// Get the first mandatory attribute[0]
			AdminOwner = getAttributeValue(
					changedAttributes->attributeId, 		// Id of Attribute to convert
					notificationHandle,						// Handle of transaction
					changedAttributes,						// Attributes new value, for modification
					NULL,									// Attributes new value, for creation
					numAttributes);							// Number of attribute values in the array
			if ((AdminOwner != NULL) && (strncmp(AdminOwner, "OAMSA", 5) == 0))
			{
				// OK, OAMSA is the source of this change
				isCOMSAAdminOwner = true;
			}
		}

		// Forward possible config structs to the Cache
		if (strncmp(immRdn, "id=",3) == 0)
		{
			sendsetDnInCmCache = transformAttributeListStruct(notificationHandle, numAdditionalInfo, additionalInfo,
					numAttributes, changedAttributes, NULL, immRdn, &notifyCache);
		}
		else
		{
			// Send config simple types to the cache here
			sendsetDnInCmCache = transformAttributeList(notificationHandle, numAdditionalInfo, additionalInfo,
					numAttributes, changedAttributes, NULL, immRdn, &notifyCache, false);
		}
		break;
	default : // What do we do here
		ERR_OAMSA_CMEVENT("convertCMNotificationHeader(): switch: default case, call freeConvertCMResources()");
		LEAVE_OAMSA_CMEVENT();
		return false;
	}

	// OK convert the dn, common operation to all, except the delete, because there we do not have
	// a object in IMM and can therefore not translate the normal way.
	char *dnToUse = NULL;
	if (!isDeleteObject)
	{
		if ((!convertTo3GPPDN(no, (char**)&(dnToUse))))
		{
			ERR_OAMSA_CMEVENT("convertCMNotificationHeader : Failed to translate dn");
			// Failure free resources and return
			freeConvertCMResources(immRdn);
			LEAVE_OAMSA_CMEVENT();
			return false;
		}
	}
	else
	{
		// Use same method as in OIProxy create case
		dnToUse = convertTo3GPPDNCOMCASE(immRdn);
	}
	// Quick removal of all parts that have id in them
	char *out = NULL;
	bool deleteRuntimeReferencedObject = false;
	if (removeStructPartOfDn(&out, (const char**)&(dnToUse)))
	{
		free(dnToUse);
		dnToUse = out;
		// In case of delete runtime object and that object is a struct object in IMM (that is the referenced object),
		// then set "deleteRuntimeReferencedObject" variable to true.
		if(notifyCache.ccbId == 0 && eventType == MafOamSpiCmEvent_MoDeleted_1)
		{
			deleteRuntimeReferencedObject = true;
		}
	}
	/** Indicates the source of the change */
	MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator;
	// Set the source Indicator
	if ((*theSourceIndicator) == SA_NTF_OBJECT_OPERATION)
	{
		sourceIndicator = MafOamSpiCmEvent_ResourceOperation_1;
	}
	else if (((*theSourceIndicator) == SA_NTF_MANAGEMENT_OPERATION) && isCOMSAAdminOwner)
	{
		sourceIndicator = MafOamSpiCmEvent_ManagementOperation_1;
	}
	else
	{
		// Ok, this is what is left
		sourceIndicator = MafOamSpiCmEvent_Unknown_1;
	}

	// Call the cache, when we have something to deliver, which not always is true...
	if (sendsetDnInCmCache && !deleteRuntimeReferencedObject)
	{
		DEBUG_OAMSA_CMEVENT("convertCMNotificationHeader(): setDnInCmCache: CCBID<%llu>", notifyCache.ccbId);
		DEBUG_OAMSA_CMEVENT("convertCMNotificationHeader(): setDnInCmCache: DN<%s>", dnToUse);

		bool  resultOfCleaning = removeStructPartOfDn(&out, (const char**)&(dnToUse));
		DEBUG_OAMSA_CMEVENT("convertCMNotificationHeader(): setDnInCmCache: out<%s>", out);

		// OK, if this concerns a runtime object then always set ccblast to true!
		if (notifyCache.ccbId == 0)
		{
			// OK, ccblast shall be true in this case
			notifyCache.ccbLast = true;
		}

		if (!resultOfCleaning)
		{
			setDnInCmCache(notifyCache.ccbId,
					sourceIndicator,
					dnToUse,
					eventType,
					notifyCache.ccbLast);
		}
		else
		{
			DEBUG_OAMSA_CMEVENT("convertCMNotificationHeader(): setDnInCmCache: dn<%s>", out);
			setDnInCmCache(notifyCache.ccbId,
					sourceIndicator,
					out,
					eventType,
					notifyCache.ccbLast);
			free(out);
			out = NULL;
		}

		DEBUG_OAMSA_CMEVENT("convertCMNotificationHeader(): last phase, call freeConvertCMResources");
	}

	if (notifyCache.ccbLast) {
		removeDiscardedCcb(notifyCache.ccbId);
	}
	// Freeing the memory of "dnToUse" and "out".
	// The following logic is needed since in some case (see the code above) "dnToUse" is equal to "out"
	if(dnToUse != NULL && out != NULL && dnToUse == out)
	{
		free(out);
		dnToUse = NULL;
		out = NULL;
	}
	else if(dnToUse != NULL && out != NULL && dnToUse != out)
	{
		free(dnToUse);
		free(out);
		dnToUse = NULL;
		out = NULL;
	}
	else if(dnToUse != NULL)
	{
		free(dnToUse);
		dnToUse = NULL;
	}
	else if(out != NULL)
	{
		free(out);
		out = NULL;
	}

	freeConvertCMResources(immRdn);
	DEBUG_OAMSA_CMEVENT("convertCMNotificationHeader(): RETURN true");
	LEAVE_OAMSA_CMEVENT();
	return true;
}


/**
 * Converts notification from SA-style to COM-style
 *
 * @param[in] notification  pointer to NTF notification
 * @param[in,out] comNot    pointer to COM notification data header
 *
 * @return  The result of the conversion of an error code
 * @retval  MafOk           if conversion went fine
 * @retval  MafNotExist     if no valid filters exist
 * @retval  MafNoResources  if out of memory
 * @retval  MafInvalidArgument  Couldn't parse the DN of notifying object for PM-services alarm
 */
#ifndef UNIT_TEST
static MafReturnT convertHeader4(const SaNtfNotificationsT *notification, MafOamSpiNotificationFmStruct_4T *comNot)
#else
MafReturnT convertHeader4(const SaNtfNotificationsT *notification, MafOamSpiNotificationFmStruct_4T *comNot)
#endif
{
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("convertHeader4 ENTER");

	const SaNtfNotificationHeaderT *nh = NULL;
	SaNtfNotificationHandleT notificationHandle = 0 ;
	if(notification->notificationType == SA_NTF_TYPE_SECURITY_ALARM){
		nh = &notification->notification.securityAlarmNotification.notificationHeader;
		notificationHandle = notification->notification.securityAlarmNotification.notificationHandle;
	}else{ // SA_NTF_TYPE_ALARM or SA_NTF_ALARM_QOS
		nh = &notification->notification.alarmNotification.notificationHeader;
		notificationHandle = notification->notification.alarmNotification.notificationHandle;
	}
	assert(nh != NULL && comNot != NULL);

	if(!nFilter_V4.filter) {
		DEBUG_PMTSA_EVENT("convertHeader4 LEAVE !nFilter_V4.filter");
		LEAVE_PMTSA_EVENT();
		return MafNotExist;
	}

	/*
	* When the PM-services sends a Threshold-alarm/notification, it will
	* need some extra handling
	*/
	if ((nh->notificationClassId->vendorId == SA_NTF_VENDOR_ID_SAF) &&
			/*
			* NOTE! OpenSAF/CoreMW doesn't have a 'last' enum-value for
			*       for their enumerations, so at the time of writing
			*       this code, PM will send MAX(majorId)+5 as majorId.
			*       How-ever, when OpenSAF evolves, this might become
			*       invalid as the majorId might describe a totally
			*       different service.
			*/
			(nh->notificationClassId->majorId == 20) &&
			(nh->notificationClassId->minorId == 1))
	{
		DEBUG_PMTSA_EVENT("convertHeader4 convertPmThresholdHeader4");
		MafReturnT pmRetCode = convertPmThresholdHeader4(notification, comNot);
		if (pmRetCode == MafOk)
		{
			DEBUG_PMTSA_EVENT("convertHeader4 LEAVE pmRetCode == MafOk");
			LEAVE_PMTSA_EVENT();
			return pmRetCode;
		}
		else
		{
			ERR_PMTSA_EVENT("convertHeader4: Couldn't convert PM notification, doing legacy transformation instead, rc=%d", pmRetCode);
		}
	}

	SaNameT *no = nh->notificationObject;
	comNot->eventTime = *nh->eventTime;

	if (MafOamSpiNotificationFm_3 != _mafSpiNtfFmVersion)
	{
		// MR38690 - UUID is included
		copyAdditionalInfo(nh, notificationHandle, comNot);
	}
	char *uuidList = NULL;
	if(nh->additionalText)
	{
		if (uuid_cfg_flag == UuidMapValueToAddText && (uuidList = getUUIDFromAdditionalInfo(nh, notificationHandle)))
		{
			comNot->additionalText = (char*)calloc(strlen((char*)nh->additionalText) + strlen(uuidList) + 1, sizeof(char));
			if (!comNot->additionalText)
			{
				DEBUG_PMTSA_EVENT("convertHeader4 LEAVE !comNot->additionalText");
				free(uuidList);
				LEAVE_PMTSA_EVENT();
				return MafNoResources;
			}
			char* tempPtr = comNot->additionalText;
			memcpy(tempPtr, (char*)nh->additionalText, strlen((char*)nh->additionalText));
			tempPtr += strlen((char*)nh->additionalText);
			memcpy(tempPtr, uuidList, strlen(uuidList));

			free(uuidList);
		}
		else
		{
			comNot->additionalText = strdup((char*)nh->additionalText);
			if (!comNot->additionalText)
			{
				DEBUG_PMTSA_EVENT("convertHeader4 LEAVE !comNot->additionalText");
				LEAVE_PMTSA_EVENT();
				return MafNoResources;
			}
		}
	}
	else
	{
		comNot->additionalText = NULL;
	}
	if (!convertTo3GPPDN(no, &(comNot->dn)))
	{
		DEBUG_PMTSA_EVENT("convertHeader4 LEAVE !convertTo3GPPDN(no, &(comNot->dn))");
		free(comNot->additionalText);
		LEAVE_PMTSA_EVENT();
		return MafNoResources;
	}

	DEBUG_PMTSA_EVENT("convertHeader4: comNot->dn = %s",comNot->dn);
	comNot->majorType = nh->notificationClassId->vendorId;
	comNot->minorType = nh->notificationClassId->majorId;
	comNot->minorType = ((comNot->minorType << 16)|nh->notificationClassId->minorId);
	DEBUG_PMTSA_EVENT("convertHeader4 LEAVE");
	LEAVE_PMTSA_EVENT();
	return MafOk;
}

/**
 * Converts notification from COM-style to SA-style
 * nh argument must be initialized by saNtfAlarmNotificationAllocate before this function is called.
 *
 * @param[in]  comNot        pointer to COM notification data header
 * @param[out] nh            pointer to NTF notification data header
 * @param[in]  notificationHandle  Pointer to the internal notification structure
 *
 * @return  The result of the conversion of an error code
 * @retval  MafOk           if conversion went fine
 * @retval  MafNotExist     if no valid filters exist
 * @retval  MafNoResources  if out of memory
 * @retval  MafInvalidArgument  Couldn't parse the DN of notifying object for PM-services alarm
 */
#ifndef UNIT_TEST
static MafReturnT convertMafHeader4ToNtf(MafOamSpiNotificationFmStruct_4T *comNot, SaNtfNotificationHeaderT *nh, const SaNtfNotificationHandleT notificationHandle)
#else
MafReturnT convertMafHeader4ToNtf(MafOamSpiNotificationFmStruct_4T *comNot, SaNtfNotificationHeaderT *nh, const SaNtfNotificationHandleT notificationHandle)
#endif
{
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("convertMafHeader4ToNtf ENTER");

	if(!comNot || !nh) {
		DEBUG_PMTSA_EVENT("convertMafHeader4ToNtf (!comNot || !nh) = MafInvalidArgument");
		LEAVE_PMTSA_EVENT();
		return MafInvalidArgument;
	}

	if(!comNot->dn) {
		DEBUG_PMTSA_EVENT("convertMafHeader4ToNtf (!comNot->dn) = MafFailure");
		LEAVE_PMTSA_EVENT();
		return MafFailure;
	}

	MafReturnT retVal = MafFailure;
	if (strcmp("lm", _CC_NAME) == 0)
	{
		enum{
			DN_ERROR = -1,
			DN_LMID = 0,
			DN_IMM = 2
		};

		//Applicable for LM only as LM is the only user for FM NTF SPI v3
		*(nh->eventType) = SA_NTF_ALARM_QOS;
		DEBUG_PMTSA_EVENT("convertMafHeader4ToNtf() eventType = (SA_NTF_ALARM_QOS)quality of service");

		unsigned size = 0;
#ifndef UNIT_TEST
		char immDn[strlen(comNot->dn) + 1];
		int statusPrefix = removeMOMPrefix(comNot->dn, &size, immDn);
		DEBUG_PMTSA_EVENT("statusPrefix %d", statusPrefix);
		if(statusPrefix == DN_ERROR){ // if input dn is NULL then no conversion
			return MafFailure;
		}
		DEBUG_PMTSA_EVENT("immDn = |%s| size = %d ", immDn, size);
#else
		const char* immDn = NULL;
#endif
		if (statusPrefix == DN_IMM)
		{
			/*
			 * Either the DN conversion is not successful
			 * Or, 3GPP DN from LM(FM NTF SPI v3) doesn't contain 'licenseId'
			 * Either way, we go with usual conversion to IMM with convertToImm()
			 */
			if(convertToImm(comNot->dn, nh->notificationObject))
			{
				retVal = MafOk;
			}
		}
		else
		{
			saNameSetLen(immDn, size, nh->notificationObject);
			DEBUG_PMTSA_EVENT("immDn = %s ", immDn);
			retVal = MafOk;
		}
		saNameSetLen("lmId=1", 6, nh->notifyingObject);
	}
	else
	{
		*(nh->eventType) = SA_NTF_TYPE_ALARM;

		if(convertToImm(comNot->dn, nh->notificationObject))
		{
			retVal = MafOk;
		}
		saNameSet(saNameGet(nh->notificationObject), nh->notifyingObject);
	}

	if (MafOk != retVal)
	{
		ERR_PMTSA_EVENT("convertMafHeader4ToNtf convertToImm failed: %d", (int)retVal);
		//TODO: saNameDelet ntion object, ntfyn object.
		LEAVE_PMTSA_EVENT();
		return retVal;
	}
	DEBUG_PMTSA_EVENT("convertMafHeader4ToNtf() comNot->dn %s majorType %d minorType %d", comNot->dn, comNot->majorType, comNot->minorType);

	nh->notificationClassId->vendorId = comNot->majorType;
	nh->notificationClassId->majorId = comNot->minorType / 65536;
	nh->notificationClassId->minorId = comNot->minorType % 65536;

	*(nh->eventTime) = (SaTimeT)comNot->eventTime;

	// the length of additionalText is set and allocated by saNtfAlarmNotificationAllocate
	if(comNot->additionalText) {
		strcpy(nh->additionalText, comNot->additionalText);
	}

	// copy the additionalInfo
	if (MafOamSpiNotificationFm_3 == _mafSpiNtfFmVersion)
	{
		nh->numAdditionalInfo = 0;
		nh->additionalInfo = NULL;
	}
	else
	{
		DEBUG_PMTSA_EVENT("convertMafHeader4ToNtf - additionalInfo size=%d", comNot->additionalInfo.size);
		int iAddInfo = 0;
		SaStringT destPtr = NULL;
		SaUint16T additionalInfoIdent = 20;  // Application specific value
		nh->numAdditionalInfo = comNot->additionalInfo.size;
		for (iAddInfo = 0; iAddInfo < comNot->additionalInfo.size; iAddInfo++)
		{
			DEBUG_PMTSA_EVENT("convertMafHeader4ToNtf iAddInfo (%d) name (%s) value (%s)",
			                   iAddInfo, comNot->additionalInfo.additionalInfoArr[iAddInfo].name,
			                   comNot->additionalInfo.additionalInfoArr[iAddInfo].value);
			// Copy comNot->additionalInfo to nh->additionalInfo
			nh->additionalInfo[iAddInfo].infoId = additionalInfoIdent;
			nh->additionalInfo[iAddInfo].infoType = SA_NTF_VALUE_STRING;
			int lenValue = strlen(comNot->additionalInfo.additionalInfoArr[iAddInfo].value) + 1;
			DEBUG_PMTSA_EVENT("convertMafHeader4ToNtf saNtfPtrValAllocate ntfHandle (%llu) strlen(value) (%d) destPtr (%p)",
			                   notificationHandle, lenValue, &destPtr);

			SaAisErrorT ret = saNtfPtrValAllocate(notificationHandle,
			                                      lenValue,
			                                      (void**) &destPtr,
			                                      &(nh->additionalInfo[iAddInfo].infoValue));
			if (ret != SA_AIS_OK) {
				LOG_PMTSA_EVENT("convertMafHeader4ToNtf - could not allocate ptr value for additional info");
				LEAVE_PMTSA_EVENT();
				return MafFailure;
			}
			strcpy(destPtr, comNot->additionalInfo.additionalInfoArr[iAddInfo].value);
		}
	}
	DEBUG_PMTSA_EVENT("convertMafHeader4ToNtf LEAVE");
	LEAVE_PMTSA_EVENT();
	return MafOk;
}

/* Subscribe */
static SaAisErrorT subscribeForNotifications(SaNtfSubscriptionIdT subscriptionId)
{
	ENTER_OAMSA_ALARM();
	DEBUG_OAMSA_ALARM("subscribeForNotifications ENTER");
	SaAisErrorT rc = SA_AIS_OK;
	SaNtfAlarmNotificationFilterT myAlarmFilter;
	SaNtfSecurityAlarmNotificationFilterT secAlarmFilter;
	SaNtfNotificationTypeFilterHandlesT notificationFilterHandles;
	memset(&notificationFilterHandles, 0, sizeof notificationFilterHandles);

	rc = saNtfAlarmNotificationFilterAllocate(ntfHandle, &myAlarmFilter,0,0,0,0,0,0,0);
	if (rc != SA_AIS_OK) {
		ERR_OAMSA_ALARM("subscribeForNotifications saNtfAlarmNotificationFilterAllocate failed - %d",  rc);
		LEAVE_OAMSA_ALARM();
		return rc;
	}
	notificationFilterHandles.alarmFilterHandle = myAlarmFilter.notificationFilterHandle;
	rc = saNtfSecurityAlarmNotificationFilterAllocate(ntfHandle, &secAlarmFilter,0,0,0,0,0,0,0,0,0);
	if (rc != SA_AIS_OK) {
		ERR_OAMSA_ALARM("subscribeForNotifications saNtfSecurityAlarmNotificationFilterAllocate failed - %d",  rc);
		LEAVE_OAMSA_ALARM();
		return rc;
	}
	notificationFilterHandles.securityAlarmFilterHandle = secAlarmFilter.notificationFilterHandle;

	DEBUG_OAMSA_ALARM("subscribeForNotifications saNtfNotificationSubscribe");

	rc = saNtfNotificationSubscribe(&notificationFilterHandles, subscriptionId);
	if (SA_AIS_OK == rc)
	{
		DEBUG_OAMSA_ALARM("subscribeForNotifications saNtfNotificationSubscribe ok");
	}
	else
	{
		ERR_OAMSA_ALARM("subscribeForNotifications saNtfNotificationSubscribe failed - %d",  rc);
		// Only use the value of rc for return.
		SaAisErrorT rc2 = saNtfNotificationFilterFree (notificationFilterHandles.alarmFilterHandle);
		if (SA_AIS_OK != rc2)
		{
			ERR_OAMSA_ALARM("subscribeForNotifications saNtfNotificationFilterFree failed for alarms - %d",  rc2);
		}
		else
		{
			rc2 = saNtfNotificationFilterFree (notificationFilterHandles.securityAlarmFilterHandle);
			if (SA_AIS_OK != rc2)
			{
				ERR_OAMSA_ALARM("subscribeForNotifications saNtfNotificationFilterFree failed for securityAlarms - %d",  rc2);
			}
		}
	}
	DEBUG_OAMSA_ALARM("subscribeForNotifications return with %d",rc);
	LEAVE_OAMSA_ALARM();
	return rc;
}

static SaNtfNotificationTypeFilterHandlesT notificationFilterHandles;
static bool isSubscribed = false;

/* Subscribe */
static SaAisErrorT subscribeFor_CM_Notifications(void)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("subscribeFor_CM_Notifications ENTER");
	SaAisErrorT rc = SA_AIS_OK;
	SaNtfObjectCreateDeleteNotificationFilterT objectCreateDeleteFilter;
	SaNtfAttributeChangeNotificationFilterT attributeChangeFilter;
	SaNtfStateChangeNotificationFilterT stateChangeFilter;
	memset(&notificationFilterHandles, 0, sizeof notificationFilterHandles);

	rc = saNtfObjectCreateDeleteNotificationFilterAllocate(ntfHandle, &objectCreateDeleteFilter,0,0,0,1,0);
	if (rc != SA_AIS_OK)
	{
		ERR_OAMSA_CMEVENT("subscribeFor_CM_Notifications(): saNtfObjectCreateDeleteNotificationFilterAllocate failed - %d",  rc);
		LEAVE_OAMSA_CMEVENT();
		return rc;
	}

	rc = saNtfStateChangeNotificationFilterAllocate(ntfHandle, &stateChangeFilter,0,0,0,1,0,0);
	if (rc != SA_AIS_OK)
	{
		ERR_OAMSA_CMEVENT("subscribeFor_CM_Notifications(): saNtfStateChangeNotificationFilterAllocate_2 failed - %d",  rc);
		LEAVE_OAMSA_CMEVENT();
		return rc;
	}

	rc = saNtfAttributeChangeNotificationFilterAllocate(ntfHandle, &attributeChangeFilter,0,0,0,1,0);
	if (rc != SA_AIS_OK)
	{
		ERR_OAMSA_CMEVENT("subscribeFor_CM_Notifications(): saNtfAttributeChangeNotificationFilterAllocate failed - %d",  rc);
		LEAVE_OAMSA_CMEVENT();
		return rc;
	}

	objectCreateDeleteFilter.notificationFilterHeader.notificationClassIds->vendorId = (SaUint32T)32993;
	objectCreateDeleteFilter.notificationFilterHeader.notificationClassIds->majorId = (SaUint16T)8;
	objectCreateDeleteFilter.notificationFilterHeader.notificationClassIds->minorId = (SaUint16T)0;

	attributeChangeFilter.notificationFilterHeader.notificationClassIds->vendorId = (SaUint32T)32993;
	attributeChangeFilter.notificationFilterHeader.notificationClassIds->majorId = (SaUint16T)8;
	attributeChangeFilter.notificationFilterHeader.notificationClassIds->minorId = (SaUint16T)0;

	stateChangeFilter.notificationFilterHeader.notificationClassIds->vendorId = (SaUint32T)32993;
	stateChangeFilter.notificationFilterHeader.notificationClassIds->majorId = (SaUint16T)8;
	stateChangeFilter.notificationFilterHeader.notificationClassIds->minorId = (SaUint16T)0;

	notificationFilterHandles.objectCreateDeleteFilterHandle = objectCreateDeleteFilter.notificationFilterHandle;
	notificationFilterHandles.attributeChangeFilterHandle = attributeChangeFilter.notificationFilterHandle;
	notificationFilterHandles.stateChangeFilterHandle = stateChangeFilter.notificationFilterHandle;

	DEBUG_OAMSA_CMEVENT("subscribeFor_CM_Notifications(): saNtfNotificationSubscribe");

	rc = saNtfNotificationSubscribe(&notificationFilterHandles, CM_NOTIFICATIONS_SUBS_ID);
	if (SA_AIS_OK == rc)
	{
		isSubscribed = true;
		DEBUG_OAMSA_CMEVENT("subscribeFor_CM_Notifications saNtfNotificationSubscribe ok");
	}
	else
	{
		ERR_OAMSA_CMEVENT("subscribeFor_CM_Notifications saNtfNotificationSubscribe failed - %d", rc);
		rc = saNtfNotificationFilterFree(notificationFilterHandles.objectCreateDeleteFilterHandle);
		if (SA_AIS_OK != rc)
		{
			ERR_OAMSA_CMEVENT("subscribeFor_CM_Notifications saNtfNotificationFilterFree failed for objectCreateDelete - %d", rc);
		}
		else
		{
			rc = saNtfNotificationFilterFree(notificationFilterHandles.attributeChangeFilterHandle);
			if (SA_AIS_OK != rc)
			{
				ERR_OAMSA_CMEVENT("subscribeFor_CM_Notifications saNtfNotificationFilterFree failed for attributeChange - %d", rc);
			}
			else
			{
				rc = saNtfNotificationFilterFree(notificationFilterHandles.stateChangeFilterHandle);
				if (SA_AIS_OK != rc)
				{
					ERR_OAMSA_CMEVENT("subscribeFor_CM_Notifications saNtfNotificationFilterFree failed for stateChange - %d", rc);
				}
			}
		}
	}
	DEBUG_OAMSA_CMEVENT("subscribeFor_CM_Notifications(): return with %d",rc);
	LEAVE_OAMSA_CMEVENT();
	return rc;
}

/* Unsubscribe */
static SaAisErrorT unsubscribeFor_CM_Notifications(void)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("unsubscribeFor_CM_Notifications ENTER");
	SaAisErrorT rc = SA_AIS_OK;

	if(isSubscribed)
	{
		rc = saNtfNotificationFilterFree(notificationFilterHandles.objectCreateDeleteFilterHandle);
		if (SA_AIS_OK != rc)
		{
			ERR_OAMSA_CMEVENT("unsubscribeFor_CM_Notifications saNtfNotificationFilterFree failed for objectCreateDelete - %d", rc);
		}
		else
		{
			rc = saNtfNotificationFilterFree(notificationFilterHandles.attributeChangeFilterHandle);
			if (SA_AIS_OK != rc)
			{
				ERR_OAMSA_CMEVENT("unsubscribeFor_CM_Notifications saNtfNotificationFilterFree failed for attributeChange - %d", rc);
			}
			else
			{
				rc = saNtfNotificationFilterFree(notificationFilterHandles.stateChangeFilterHandle);
				if (SA_AIS_OK != rc)
				{
					ERR_OAMSA_CMEVENT("unsubscribeFor_CM_Notifications saNtfNotificationFilterFree failed for stateChange - %d", rc);
				}
				else
				{
					OAM_SA_NTF_RETRY(saNtfNotificationUnsubscribe(CM_NOTIFICATIONS_SUBS_ID));
					if (rc != SA_AIS_OK)
					{
						WARN_OAMSA_CMEVENT("unsubscribeFor_CM_Notifications: Warning: saNtfNotificationUnsubscribe failed for CM NTF %u", rc);
					}
					else
					{
						isSubscribed = false;
					}
				}
			}
		}
	}

	DEBUG_OAMSA_CMEVENT("unsubscribeFor_CM_Notifications(): return with %d",rc);
	LEAVE_OAMSA_CMEVENT();
	return rc;
}

void setMafStateChangeReason(MafStateChangeReasonT reason)
{
	_reason = reason;

}

static SaAisErrorT readOldAlarms(SaUint32T numDiscarded, const SaNtfIdentifierT *discardedIds, const bool isDiscarded)
{
	ENTER_OAMSA_ALARM();
	DEBUG_OAMSA_ALARM("readOldAlarms ENTER");
	SaNtfSearchCriteriaT searchCriteria;
	SaNtfAlarmNotificationFilterT af;
	SaNtfSecurityAlarmNotificationFilterT saf;
	SaNtfNotificationTypeFilterHandlesT fhdls;
	SaNtfReadHandleT readHandle;
	SaNtfNotificationsT n;
	searchCriteria.searchMode = SA_NTF_SEARCH_ONLY_FILTER;
	SaNtfNotificationHandleT nh;
	SaAisErrorT rc;
	int i, j = 1;

	searchCriteria.eventTime  = 0;

	if (is_ntf_stop_component == true && _reason == MafFailingOver)
	{
		ERR_OAMSA_ALARM("This is very rare scenario where ReadoldAlarms are called after maf_ComNtfServiceClose with reason Maf fail over");
		LEAVE_OAMSA_ALARM();

		/* As components already stoppped at maf_ComNtfServiceClose, crash can be
		 * observed if continued further. So we are returing from here. This is very rare case
		 * The return status from hereshould not have any impact*/
		rc = SA_AIS_OK;
		return rc;
	}

	if (pthread_mutex_lock(&notify_lock) != 0)
	{
		ERR_OAMSA_ALARM("readOldAlarms Failed locking mutex, aborting(!)");
		LEAVE_OAMSA_ALARM();
		abort();
	}
	// Use the correct filter for the Fm version
	if (nFilter_V4.filter != NULL)
	{
		if (nFilter_V4.filter[0] != NULL)
		{
			if (nFilter_V4.filter[0]->value != NULL)
			{
				searchCriteria.eventTime  = *(SaTimeT*)nFilter_V4.filter[0]->value;
				DEBUG_OAMSA_ALARM("readOldAlarms Setting search time to %llu",searchCriteria.eventTime );
			}
			else
			{
				DEBUG_OAMSA_ALARM("readOldAlarms Filter time value is EMPTY! ColdStart assumed ");
			}
		}
		else
		{
			ERR_OAMSA_ALARM("readOldAlarms Filter Arrays is EMPTY! ");
		}
	}
	else
	{
		ERR_OAMSA_ALARM("readOldAlarms Filter value is NULL");
	}

	if (pthread_mutex_unlock(&notify_lock) != 0)
	{
		ERR_OAMSA_ALARM("readOldAlarms Failed unlocking mutex, aborting(!)");
		LEAVE_OAMSA_ALARM();
		abort();
	}
	rc = saNtfAlarmNotificationFilterAllocate(ntfHandle, &af,0,0,0,0,0,0,0);
	if (rc != SA_AIS_OK) {
		ERR_OAMSA_ALARM("readOldAlarms saNtfAlarmNotificationFilterAllocate failed - %d",  rc);
		goto done;
	}
	rc = saNtfSecurityAlarmNotificationFilterAllocate(ntfHandle, &saf,0,0,0,0,0,0,0,0,0);
	if (rc != SA_AIS_OK) {
		ERR_OAMSA_ALARM("readOldAlarms saNtfSecurityAlarmNotificationFilterAllocate failed - %d",  rc);
		goto free_afilter;
	}
	// initialize unused handles with NULL to avoid saNtfNotificationReadInitialize error in some cases.
	fhdls.alarmFilterHandle = af.notificationFilterHandle;
	fhdls.attributeChangeFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
	fhdls.objectCreateDeleteFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
	fhdls.securityAlarmFilterHandle = saf.notificationFilterHandle;
	fhdls.stateChangeFilterHandle = SA_NTF_FILTER_HANDLE_NULL;

	// Search Criteria for retry call
	if(!isDiscarded) {
		sReaderMaxRetries++;
		if(readOldAlarm) { //only executed after first iteration onwards
			LOG_OAMSA_ALARM("readOldAlarms: searchCriteria NotificationID %lu", readerLastNotificationID);
			searchCriteria.notificationId = readerLastNotificationID;
			searchCriteria.searchMode =SA_NTF_SEARCH_NOTIFICATION_ID;
		} else {
			readOldAlarm = true;
		}
	}
	DEBUG_OAMSA_ALARM("saNtfNotificationReadInitialize");
	rc = saNtfNotificationReadInitialize(searchCriteria, &fhdls, &readHandle);
	if (rc != SA_AIS_OK) {
		ERR_OAMSA_ALARM("saNtfNotificationReadInitialize failed - %d",  rc);
		goto free_filter;
	}
	DEBUG_OAMSA_ALARM("saNtfNotificationReadInitialize ok");
	/* read as many matching notifications as exist for the time period between
	the last received one and now */
	/*The notifications can be processed only till NtfService is running. ntf_stop_thread is set to true when ntf service
	is closed to inform the readerThread to exit.*/
	while (((rc = saNtfNotificationReadNext(readHandle, SA_NTF_SEARCH_YOUNGER, &n)) == SA_AIS_OK) && (!ntf_stop_thread)) {
		DEBUG_OAMSA_ALARM("readOldAlarms read alarm no %d from NTF", j);
		if (numDiscarded) {
			for (i=0; i< numDiscarded; i++) {
				if (discardedIds[i] == *n.notification.alarmNotification.notificationHeader.notificationId) {
					saNtfNotificationCallback4(0, &n);
				} else {
					if (n.notificationType == SA_NTF_TYPE_ALARM || n.notificationType == SA_NTF_ALARM_QOS) {
						nh = n.notification.alarmNotification.notificationHandle;
					} else if (n.notificationType == SA_NTF_TYPE_SECURITY_ALARM) {
						nh = n.notification.securityAlarmNotification.notificationHandle;
					}
					rc = saNtfNotificationFree(nh);
					assert(rc == SA_AIS_OK);
				}
			}
		} else {
			/* work around: search by time not implemented in NTF */
			DEBUG_OAMSA_ALARM("readOldAlarms searchCriteria.eventTime: %llu", searchCriteria.eventTime );
			DEBUG_OAMSA_ALARM("readOldAlarms *n.notification.alarmNotification.notificationHeader.eventTime: %llu", *n.notification.alarmNotification.notificationHeader.eventTime );
			if (*n.notification.alarmNotification.notificationHeader.eventTime > searchCriteria.eventTime ||
					searchCriteria.eventTime == 0) {
				DEBUG_OAMSA_ALARM("readOldAlarms calls saNtfNotificationCallback4");
				if(!isDiscarded) {
					// Save the reader notification ID to match it with last notification received
					// from coreMW, this is need to sync the reader API and the alarmQueue.
					readerLastNotificationID = getNotificationId(&n);
					DEBUG_OAMSA_ALARM("readOldAlarms: readerLastNotificationID %lu", readerLastNotificationID);
					if (readerLastNotificationID == callbackLastCmwNotificationID) {
						LOG_OAMSA_ALARM("readOldAlarms: Matched readerLastNotificationID = %lu callbackLastCmwNotificationID = %lu", readerLastNotificationID, callbackLastCmwNotificationID);
						lastNotificationMatched = true;
						saNtfNotificationCallback4(0, &n);
						break;
					}
				}
				saNtfNotificationCallback4(0, &n);
			} else {
				DEBUG_OAMSA_ALARM("readOldAlarms calls saNtfNotificationFree");
				if (n.notificationType == SA_NTF_TYPE_ALARM || n.notificationType == SA_NTF_ALARM_QOS) {
					nh = n.notification.alarmNotification.notificationHandle;
				} else if (n.notificationType == SA_NTF_TYPE_SECURITY_ALARM) {
					nh = n.notification.securityAlarmNotification.notificationHandle;
				}
				rc = saNtfNotificationFree(nh);
				assert(rc == SA_AIS_OK);
			}
		}
		j++;
	}
	if (rc == SA_AIS_ERR_NOT_EXIST)
		rc = SA_AIS_OK; /* no more notification exists */
	if (!isDiscarded){
		// Process the Queued alarms from coreMW based on if there is match in reader API
		// or if there is no notification received from coreMW after NTF service start
		if (lastNotificationMatched || callbackLastCmwNotificationID == 0) {
			LOG_OAMSA_ALARM("readOldAlarms: lastNotificationMatched = %d", lastNotificationMatched);
			sendQueuedAlarms();
		} else {
			//retry only for a limited number of times with very minimum sleep interval
			if (OAM_SA_NTF_READER_MAX_RETRIES > sReaderMaxRetries) {
				DEBUG_OAMSA_ALARM("readOldAlarms: sReaderMaxRetries is: %u", sReaderMaxRetries);
				usleep(OAM_SA_NTF_RETRY_SLEEP); //sleep for 100ms
				rc = readOldAlarms(0, NULL, false);
			} else {
				//If max retries reached, then abort recursion and release all queued alarms to MAF.
				sendQueuedAlarms();
			}
		}

	}
free_filter:
	saNtfNotificationFilterFree(fhdls.securityAlarmFilterHandle);
	saNtfNotificationReadFinalize(readHandle);
free_afilter:
	saNtfNotificationFilterFree(fhdls.alarmFilterHandle);
done:
	DEBUG_OAMSA_ALARM("readOldAlarms LEAVE");
	LEAVE_OAMSA_ALARM();
	return rc;
}

// This function handles the callbacks received only with the subscription id of CM notifications.
static void cmNotificationCallback(const SaNtfNotificationsT *notification)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("cmNotificationCallback ENTER");
	/* Notification processing is skipped if COM is already shutting down to avoid deadlock on ntfHandle*/
        if(ntf_stop_thread){
		LOG_OAMSA_CMEVENT("cmNotificationCallback: COM is shutting down . Skipping notification processing...");
		return;
	}
	if (pthread_mutex_lock(&cm_notification_lock) != 0)
	{
		ERR_OAMSA_CMEVENT("cmNotificationCallback: Failed to lock mutex");
	}
	switch (notification->notificationType)
	{
	case SA_NTF_TYPE_STATE_CHANGE:
		DEBUG_OAMSA_CMEVENT("ComSANf.cmNotificationCallback: type SA_NTF_TYPE_STATE_CHANGE received");
		// Create an overflow CMEvent here
		MafOamSpiCmEvent_Notification_1T mafCmNotOverFlow;
		// Set txHandle to zero, limitation in this version.
		mafCmNotOverFlow.txHandle = 0;
		// Set event time
		mafCmNotOverFlow.eventTime = getTime();
		if(mafCmNotOverFlow.eventTime == 0)
		{
			// Error situation, no time available.
			ERR_OAMSA_CMEVENT("ComSANf.cmNotificationCallback : Failed to get time");
		}
		// Create the CM Event Struct
		// Only support one event in this release
		mafCmNotOverFlow.events = (MafOamSpiCmEvent_1T**)( calloc(2, sizeof(MafOamSpiCmEvent_1T*)));
		mafCmNotOverFlow.events[0] = calloc(1, sizeof(MafOamSpiCmEvent_1T));
		mafCmNotOverFlow.events[1] = NULL;
		// Overflow!
		mafCmNotOverFlow.events[0]->eventType = MafOamSpiCmEvent_Overflow_1;
		mafCmNotOverFlow.events[0]->attributes = calloc(1, sizeof(MafMoNamedAttributeValueContainer_3T*));
		mafCmNotOverFlow.events[0]->attributes[0] = NULL;
		// Add an empty string here
		mafCmNotOverFlow.events[0]->dn = calloc(1, sizeof(char*));
		mafCmNotOverFlow.sourceIndicator = MafOamSpiCmEvent_ResourceOperation_1;

		DEBUG_OAMSA_CMEVENT("cmNotificationCallback convertCMNotificationHeader : returned true");
		if(MafOk != push_CmEventHandler(&mafCmNotOverFlow))
		{
			// FIXME: HOW DO WE HANDLE MEMORY IN THIS CASE?
			DEBUG_OAMSA_CMEVENT("cmNotificationCallback: push_CmEventHandler : returned failure");
		}

		saNtfNotificationFree(notification->notification.stateChangeNotification.notificationHandle);
		if (pthread_mutex_unlock(&cm_notification_lock) != 0)
		{
			ERR_OAMSA_CMEVENT("cmNotificationCallback: Failed to unlock mutex");
		}
		DEBUG_OAMSA_CMEVENT("cmNotificationCallback LEAVE");
		LEAVE_OAMSA_CMEVENT();
		return;

	case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		DEBUG_OAMSA_CMEVENT("cmNotificationCallback: type SA_NTF_TYPE_OBJECT_CREATE_DELETE received");

		if(convertCMNotificationHeader(notification->notificationType, notification))
		{
			DEBUG_OAMSA_CMEVENT("cmNotificationCallback: convertCMNotificationHeader : returned true");
		}

		saNtfNotificationFree(notification->notification.objectCreateDeleteNotification.notificationHandle);
		if (pthread_mutex_unlock(&cm_notification_lock) != 0)
		{
			ERR_OAMSA_CMEVENT("cmNotificationCallback: Failed to unlock mutex");
		}
		DEBUG_OAMSA_CMEVENT("cmNotificationCallback LEAVE");
		LEAVE_OAMSA_CMEVENT();
		return;

	case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		DEBUG_OAMSA_CMEVENT("cmNotificationCallback: type SA_NTF_TYPE_ATTRIBUTE_CHANGE received");
		if(convertCMNotificationHeader(notification->notificationType, notification))
		{
			DEBUG_OAMSA_CMEVENT("cmNotificationCallback convertCMNotificationHeader : returned true");
		}

		saNtfNotificationFree(notification->notification.attributeChangeNotification.notificationHandle);
		if (pthread_mutex_unlock(&cm_notification_lock) != 0)
		{
			ERR_OAMSA_CMEVENT("cmNotificationCallback: Failed to unlock mutex");
		}
		DEBUG_OAMSA_CMEVENT("cmNotificationCallback LEAVE");
		LEAVE_OAMSA_CMEVENT();
		return;

	default:
		ERR_OAMSA_CMEVENT("cmNotificationCallback: unknown notification type %d", (int)notification->notificationType);
		if (pthread_mutex_unlock(&cm_notification_lock) != 0)
		{
			ERR_OAMSA_CMEVENT("cmNotificationCallback: Failed to unlock mutex");
		}
		DEBUG_OAMSA_CMEVENT("cmNotificationCallback LEAVE");
		LEAVE_OAMSA_CMEVENT();
		return;
	}
}

uint64_t getNotificationId(const SaNtfNotificationsT *notification) {
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("getNotificationId ENTER");
	uint64_t notificationID = 0;
	if (notification->notificationType == SA_NTF_TYPE_ALARM || notification->notificationType == SA_NTF_ALARM_QOS) {
		notificationID = (uint64_t)(notification->notification.alarmNotification.notificationHeader.notificationId);
	} else if (notification->notificationType == SA_NTF_TYPE_SECURITY_ALARM) {
		notificationID = (uint64_t)(notification->notification.securityAlarmNotification.notificationHeader.notificationId);
	}
	DEBUG_OAMSA_CMEVENT("getNotificationId LEAVE");
	LEAVE_OAMSA_CMEVENT();
	return notificationID;
}

// Function to deliver notification to MAF event router.
void sendSaNtfNotification(MafOamSpiNotificationFmStruct_4T *comNot)
{
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("sendSaNtfNotification ENTER");
	valueList4T* ntfContainer = addNotValue4(comNot);

	if(event_router_MAF != NULL) {
		DEBUG_PMTSA_EVENT("saNtfNotificationCallback4: event_router_MAF->notify consumerHandle=%lu, eventType=[%s], filter=%p", nFilter_V4.consumerHandle, nFilter_V4.eventType, nFilter_V4.filter);
		if (MafOamSpiNotificationFm_4 == _mafSpiNtfFmVersion) {
			event_router_MAF->notify(producer_handle, nFilter_V4.consumerHandle, MafOamSpiNotificationFmEventType_4, nFilter_V4.filter, ntfContainer->comNot);
		}
		else if (MafOamSpiNotificationFm_3 == _mafSpiNtfFmVersion) {
			event_router_MAF->notify(producer_handle, nFilter_V4.consumerHandle, MafOamSpiNotificationFmEventType_2, nFilter_V4.filter, ntfContainer->comNot_2);
		}
	}
	DEBUG_PMTSA_EVENT("sendSaNtfNotification LEAVE");
	LEAVE_PMTSA_EVENT();
}

static void sendQueuedAlarms() {
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("sendQueuedAlarms ENTER");
	// Deliver the notifications queued while reading old alarms
	/*The notifications can be processed only till NtfService is running. ntf_stop_thread is set to true when ntf service
	is closed to inform the readerThread to exit.*/
	while ((true) && (!ntf_stop_thread)) {
		if (0 != (pthread_mutex_lock(&alarm_queue_lock))) {
			ERR_OAMSA_ALARM("saNtfNotificationCallback4 Failed locking alarm_queue_lock mutex");
		}
		if(alarmQueueFront == NULL) {
			readOldAlarm = false;
			alarmQueue = NULL;
			if (0 != (pthread_mutex_unlock(&alarm_queue_lock))) {
				ERR_OAMSA_ALARM("saNtfNotificationCallback4 Failed locking alarm_queue_lock mutex");
			}
			break;
		} else {
			sendSaNtfNotification(alarmQueueFront->alarm);
			alarmQueueFront->alarm = NULL;
			AlarmObjectT *deleteAlarm = NULL;
			deleteAlarm = alarmQueueFront;
			alarmQueueFront = alarmQueueFront->next;
			if (deleteAlarm) {
				free(deleteAlarm);
				deleteAlarm = NULL;
			}

			if (0 != (pthread_mutex_unlock(&alarm_queue_lock))) {
				ERR_OAMSA_ALARM("saNtfNotificationCallback4 Failed locking alarm_queue_lock mutex");
			}
		}
	}
	DEBUG_OAMSA_CMEVENT("sendQueuedAlarms LEAVE");
	LEAVE_OAMSA_CMEVENT();
}

/* ----------------------------------------------------------------------
 * OpenSAF callback functions;
 */

/* ----------------------------------------------------------------------
 * OpenSAF callback functions;
 */

/**
 * The Notification Service invokes this callback function to deliver a notification to the subscriber.
 *
 * @param[in]  subscriptionId   An identifier that the subscriber supplied in an
 *                              saNtfNotificationSubscribe() invocation and that enables the subscriber
 *                              to determine which subscription resulted in the delivery of the notification.
 * @param[int] notification     A pointer to the notification delivered by this callback.
 *
 * @return  No return value
 */
void saNtfNotificationCallback4(SaNtfSubscriptionIdT subscriptionId, const SaNtfNotificationsT *notification)
{
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("saNtfNotificationCallback4 ENTER with subscriptionId: %u", subscriptionId);

	// If the received subscriptionId is the subscriptionId of the subscription of the CM notifications,
	// then call the method which handles these callbacks.
	if(subscriptionId == CM_NOTIFICATIONS_SUBS_ID)
	{
		cmNotificationCallback(notification);
		DEBUG_PMTSA_EVENT("saNtfNotificationCallback4 cmNotificationCallback LEAVE ");
		LEAVE_PMTSA_EVENT();
		return;
	}

	MafReturnT maf_rc = MafOk;
	SaNtfSubscriptionIdT localSubId = 0;

	MafOamSpiNotificationFmStruct_4T *comNot = calloc(1, sizeof(MafOamSpiNotificationFmStruct_4T));
	if(!comNot) {
		ERR_PMTSA_EVENT("saNtfNotificationCallback4: malloc failed");
		LEAVE_PMTSA_EVENT();
		abort();
	}

	DEBUG_PMTSA_EVENT("saNtfNotificationCallback4: notification->notificationType %d", notification->notificationType);
	switch (notification->notificationType) {
	case SA_NTF_TYPE_ALARM:
	case SA_NTF_ALARM_QOS: //for LM.
		DEBUG_PMTSA_EVENT("saNtfNotificationCallback4: type SA_NTF_TYPE_ALARM(or SA_NTF_ALARM_QOS): received");
		maf_rc = convertHeader4((SaNtfNotificationsT*)notification, comNot);
		assert(maf_rc == MafOk||maf_rc == MafNotExist);
		comNot->severity = (MafOamSpiNotificationFmSeverityT)*notification->notification.alarmNotification.perceivedSeverity;
		break;
	case SA_NTF_TYPE_SECURITY_ALARM:
		DEBUG_PMTSA_EVENT("saNtfNotificationCallback4: type SA_NTF_TYPE_SECURITY_ALARM: received");
		maf_rc = convertHeader4((SaNtfNotificationsT*)notification, comNot);
		assert(maf_rc == MafOk||maf_rc == MafNotExist);
		comNot->severity = (MafOamSpiNotificationFmSeverityT)*notification->notification.securityAlarmNotification.severity;
		break;
	case SA_NTF_TYPE_STATE_CHANGE:
		ERR_PMTSA_EVENT("saNtfNotificationCallback4: invalid notification type received: SA_NTF_TYPE_STATE_CHANGE");
		saNtfNotificationFree(notification->notification.stateChangeNotification.notificationHandle);
		free(comNot); comNot = NULL;
		DEBUG_PMTSA_EVENT("saNtfNotificationCallback4 LEAVE ");
		LEAVE_PMTSA_EVENT();
		return;
	case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		ERR_PMTSA_EVENT("saNtfNotificationCallback4: invalid notification type received: SA_NTF_TYPE_OBJECT_CREATE_DELETE");
		saNtfNotificationFree(notification->notification.objectCreateDeleteNotification.notificationHandle);
		free(comNot); comNot = NULL;
		DEBUG_PMTSA_EVENT("saNtfNotificationCallback4 LEAVE");
		LEAVE_PMTSA_EVENT();
		return;
	case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		ERR_PMTSA_EVENT("saNtfNotificationCallback4: invalid notification type received: SA_NTF_TYPE_ATTRIBUTE_CHANGE");
		saNtfNotificationFree(notification->notification.attributeChangeNotification.notificationHandle);
		free(comNot); comNot = NULL;
		DEBUG_PMTSA_EVENT("saNtfNotificationCallback4 LEAVE");
		LEAVE_PMTSA_EVENT();
		return;
	default:
		ERR_PMTSA_EVENT("saNtfNotificationCallback4: unknown notification type %d", (int)notification->notificationType);
		break;
	}
	DEBUG_PMTSA_EVENT("saNtfNotificationCallback4 nId: %lu", getNotificationId(notification));
	if (pthread_mutex_lock(&notify_lock) != 0)
	{
		ERR_PMTSA_EVENT("saNtfNotificationCallback4: Failed locking mutex, aborting(!)");
		LEAVE_PMTSA_EVENT();
		abort();
	}
	localSubId = nFilter_V4.subscriptionId;
	if (pthread_mutex_unlock(&notify_lock) != 0)
	{
		ERR_PMTSA_EVENT("saNtfNotificationCallback4: Failed unlocking mutex, aborting(!)");
		LEAVE_PMTSA_EVENT();
		abort();
	}
	DEBUG_PMTSA_EVENT("saNtfNotificationCallback4 localSubId (%d) maf_rc (%d)", localSubId, maf_rc);
	if (localSubId && maf_rc == MafOk) {
		if(subscriptionId == 0 || readOldAlarmDone){
			sendSaNtfNotification(comNot);
		} else {
			// All the notification from coreMW should be queued up when consumer requests for the missed
			// alarms during consumer down time or startup. This design is to improve the processing
			// of the FM notifications and avoid notifications getting discarded/missed while reader API
			// try's to fetch the notifications and in concurrently notifications received from ntf service.
			if (0 != (pthread_mutex_lock(&alarm_queue_lock))) {
				ERR_OAMSA_ALARM("saNtfNotificationCallback4 Failed locking alarm_queue_lock mutex");
			}
			// Fetch last notification ID before we start processing the reader API
			if(!readOldAlarm){
				callbackLastCmwNotificationID = getNotificationId(notification);
				DEBUG_OAMSA_ALARM("saNtfNotificationCallback4: callbackLastCmwNotificationID %lu", callbackLastCmwNotificationID);
				if (0 != (pthread_mutex_unlock(&alarm_queue_lock))) {
					ERR_OAMSA_ALARM("saNtfNotificationCallback4 Failed locking alarm_queue_lock mutex");
				}
				sendSaNtfNotification(comNot);
			} else {

				// Queue the notifications received from CMW notification service until Old alarms are read completely
				DEBUG_OAMSA_ALARM("saNtfNotificationCallback4 queuing alarm");
				AlarmObjectT *newAlarm = NULL;
				newAlarm = (AlarmObjectT *)malloc(sizeof(AlarmObjectT));
				newAlarm->alarm = comNot;
				newAlarm->next = NULL;
				if(!alarmQueue) {
					LOG_OAMSA_ALARM("saNtfNotificationCallback4: callbackLastCmwNotificationID = %lu", callbackLastCmwNotificationID);
					alarmQueue = newAlarm;
					alarmQueueFront = alarmQueue;
				} else {
					alarmQueue->next = newAlarm;
					alarmQueue = alarmQueue->next;
				}
				if (0 != (pthread_mutex_unlock(&alarm_queue_lock))) {
					ERR_OAMSA_ALARM("saNtfNotificationCallback4 Failed locking alarm_queue_lock mutex");
				}
			}
		}
	} else {
		WARN_PMTSA_EVENT("saNtfNotificationCallback4: COM notify function not called no filter added");
		free(comNot->dn); comNot->dn = NULL;
		free(comNot->additionalText); comNot->additionalText = NULL;
		free(comNot); comNot = NULL;
	}

	switch (notification->notificationType) {
	case SA_NTF_TYPE_ALARM:
		DEBUG_PMTSA_EVENT("saNtfNotificationCallback4 saNtfNotificationFree SA_NTF_TYPE_ALARM");
		saNtfNotificationFree(notification->notification.alarmNotification.notificationHandle);
		break;
	case SA_NTF_TYPE_SECURITY_ALARM:
		DEBUG_PMTSA_EVENT("saNtfNotificationCallback4 saNtfNotificationFree SA_NTF_TYPE_SECURITY_ALARM");
		saNtfNotificationFree(notification->notification.securityAlarmNotification.notificationHandle);
		break;
	default:
		ERR_PMTSA_EVENT("saNtfNotificationCallback4: unknown notification type %d", (int)notification->notificationType);
		break;
	}
	DEBUG_PMTSA_EVENT("saNtfNotificationCallback4 LEAVE");
	LEAVE_PMTSA_EVENT();
}

#ifndef UNIT_TEST
static void saNtfNotificationDiscardedCallback(SaNtfSubscriptionIdT subscriptionId,
							SaNtfNotificationTypeT notificationType,
							SaUint32T numberDiscarded,
							const SaNtfIdentifierT *discardedNotificationIdentifiers)
#else
void saNtfNotificationDiscardedCallback(SaNtfSubscriptionIdT subscriptionId,
							SaNtfNotificationTypeT notificationType,
							SaUint32T numberDiscarded,
							const SaNtfIdentifierT *discardedNotificationIdentifiers)
#endif
{
	ENTER_OAMSA_CMEVENT();
	unsigned int i = 0;
	DEBUG_OAMSA_CMEVENT("saNtfNotificationDiscardedCallback ENTER");

	DEBUG_OAMSA_CMEVENT("saNtfNotificationDiscardedCallback Discarded callback function  notificationType: %d\n\
				subscriptionId  : %u \n\
				numberDiscarded : %u\n", (int)notificationType, (unsigned int)subscriptionId, (unsigned int)numberDiscarded);
	for (i = 0; i < numberDiscarded; i++)
		DEBUG_OAMSA_CMEVENT("saNtfNotificationDiscardedCallback [%u]", (unsigned int)discardedNotificationIdentifiers[i]);
	DEBUG_OAMSA_CMEVENT("\n");
	readOldAlarms(numberDiscarded, discardedNotificationIdentifiers, true);
	DEBUG_OAMSA_CMEVENT("saNtfNotificationDiscardedCallback LEAVE");
	LEAVE_OAMSA_CMEVENT();
}

#ifdef UNIT_TEST
int poll_execute(void* handle)
{
	return 1;
}
int ntf_stop_thread = 0;
int ntf_event_pipe[2];
#endif

void stop_ntf_polling(struct pollfd* pfd, void *ref)
{
	ENTER_OAMSA_CMEVENT();
	char buffer[16] = {0};
	int rc = read(ntf_event_pipe[0], buffer, sizeof(buffer));
	if (rc < 0) {
		if (errno != EWOULDBLOCK) err_quit(
			"Failed to read from pipe [%s]", strerror(errno));
		DEBUG_OAMSA_CMEVENT("EWOULDBLOCK on pipe");
	} else if (rc > 1) {
		WARN_OAMSA_CMEVENT("ntf_event_pipe, read=%d", rc);
	}
	ntf_stop_thread = 1;
	poll_unsetcallback(comSASThandle_ntf, ntf_event_pipe[0]);
	LEAVE_OAMSA_CMEVENT();
}

/* ----------------------------------------------------------------------
 * Interface functions;
 */


static void* readerThread(void* arg)
{
	ENTER_OAMSA_CMEVENT();
	SaAisErrorT rc;
	unsigned int nRetries;
	DEBUG_OAMSA_CMEVENT("readerThread ENTER");
	OAM_SA_NTF_RETRY(readOldAlarms(0, NULL, false));
	readOldAlarmDone = true;
	DEBUG_OAMSA_CMEVENT("readerThread LEAVE");
	LEAVE_OAMSA_CMEVENT();
	return NULL;
}

static MafReturnT createReaderThread()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("createReaderThread called");
	if (pthread_create(&read_pthread, NULL, readerThread, NULL) != 0) {
		ERR_OAMSA_CMEVENT("Failed to start readerThread");
		LEAVE_OAMSA_CMEVENT();
		return MafFailure;
	}
	if (pthread_detach(read_pthread) != 0) {
		ERR_OAMSA_CMEVENT("pthread_detach failed");
		LEAVE_OAMSA_CMEVENT();
		return MafFailure;
	}
	DEBUG_OAMSA_CMEVENT("createReaderThread return OK");
	LEAVE_OAMSA_CMEVENT();
	return MafOk;
}

/**
 * setupInterface is called to setup the interface for MafOamSpiNotificationFmEventComponent_4
 *
 * @return No return value.
 */

 void setupInterface(void) {
	ENTER_PMTSA_EVENT();
	MafReturnT retval = MafFailure;
	DEBUG_PMTSA_EVENT("setupInterface ENTER");
	if (!addFilter_setup) {
		if (event_router_MAF != NULL) {
			// Register event type
			DEBUG_PMTSA_EVENT("setupInterface - addSubscription: Register event type for MafOamSpiEvent type version %d, consumer_handle (%d)", (int)_mafSpiNtfFmVersion, (int)consumer_handle );
			if (MafOamSpiNotificationFm_4 == _mafSpiNtfFmVersion)
			{
				retval = event_router_MAF->addSubscription(
						(MafOamSpiEventConsumerHandleT)consumer_handle,
						MafOamSpiNotificationFmEventComponent_4,
						(MafNameValuePairT **)nullNameValuePair);
			}
			else if (MafOamSpiNotificationFm_3 == _mafSpiNtfFmVersion)
			{
				retval = event_router_MAF->addSubscription(
						(MafOamSpiEventConsumerHandleT)consumer_handle,
						MafOamSpiNotificationFmEventComponent_3,
						(MafNameValuePairT **)nullNameValuePair);
			}
			if( MafOk != retval ) {
				ERR_PMTSA_EVENT( "setupInterface - Failed addSubscription MafOamSpiNotificationFmEventComponent_%d ( retval: %d ).", (int)_mafSpiNtfFmVersion, retval );
				LEAVE_PMTSA_EVENT();
				return;
			}

		}
		addFilter_setup = true;
	}
	DEBUG_PMTSA_EVENT("setupInterface LEAVE");
	LEAVE_PMTSA_EVENT();
}

/**
 * addFilter_V4 is called by the event router whenever the producer needs
 * to be updated about what events are subscribed for.
 *
 * @param consumerId [in] identifier of the consumer
 *
 * @param eventType [in] the type of event.
 *
 * @param filter [in] the filter specifies in more detail what it is the
 *        consumer will be notified for. filter is a null terminated array and must always
 *        have a value even if it contains an empty array. The
 *        consumer must ensure that the instance of the filter
 *        exists during the time that the consumer is
 *        subscribing for the filtered events. See also above about Event Filtering.
 *
 * @return MafOk, or @n
 * MafInvalidArgument if a parameter is not valid, or @n
 * MafFailure if an internal error occurred.
 */
MafReturnT addFilter_V4(
	MafOamSpiEventConsumerHandleT consumerHandle,
	const char * eventType,
	MafNameValuePairT ** filter)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("addFilter_V4 ENTER consumerHandle=%lu, eventType=[%s], filter=%p", consumerHandle, eventType, filter);
	SaAisErrorT rc = SA_AIS_OK;
	MafReturnT rc_maf = MafOk;

	if ((MafOamSpiNotificationFm_4 == _mafSpiNtfFmVersion)
		 && (strcmp( MafOamSpiNotificationFmEventType_4, eventType ) != 0))
	{
		ERR_OAMSA_CMEVENT("addFilter_V4: Unknown event type eventType=[%s]", eventType);
		LEAVE_OAMSA_CMEVENT();
		return MafNotExist;
	}
	if ((MafOamSpiNotificationFm_3 == _mafSpiNtfFmVersion)
		 && (strcmp( MafOamSpiNotificationFmEventType_2, eventType ) != 0))
	{
		ERR_OAMSA_CMEVENT("addFilter_V4: Unknown event type eventType=[%s]", eventType);
		LEAVE_OAMSA_CMEVENT();
		return MafNotExist;
	}
	setupInterface();

	if (pthread_mutex_lock(&notify_lock) != 0) {
		ERR_OAMSA_CMEVENT("addFilter_V4 pthread_mutex_lock(&notify_lock) != 0");
		LEAVE_OAMSA_CMEVENT();
		abort();
	}

	if(nFilter_V4.filter){
		WARN_OAMSA_CMEVENT("addFilter_V4 rc_com = MafAlreadyExist (%p)", nFilter_V4.filter);
		rc_maf = MafAlreadyExist;
	}
	else {
		nFilter_V4.subscriptionId = COMSA_NTF_SUBSCRIPTION_ID; /* only one subscriptionId/filter currently allowed */
		DEBUG_OAMSA_CMEVENT("addFilter_V4: subscriptionId (%d)", nFilter_V4.subscriptionId);
		// TODO: work around solution
		// Only subscribe the same ID once.
		rc = SA_AIS_OK;
		OAM_SA_NTF_RETRY(subscribeForNotifications(nFilter_V4.subscriptionId));
		if (SA_AIS_OK != rc) {
			ERR_OAMSA_CMEVENT("addFilter_V4: subscribeForNotifications failed - %d", rc);
			nFilter_V4.subscriptionId = 0;
			rc_maf = maf_coremw_rc(rc);
		}
		else {
			nFilter_V4.consumerHandle = consumerHandle;
			nFilter_V4.eventType = eventType;
			nFilter_V4.filter = filter;
			rc_maf = maf_coremw_rc(rc);
			createReaderThread();
		}
	}

	if (pthread_mutex_unlock(&notify_lock) != 0)  {
		ERR_OAMSA_CMEVENT("addFilter_V4 pthread_mutex_unlock(&notify_lock) != 0");
		LEAVE_OAMSA_CMEVENT();
		abort();
	}
	DEBUG_OAMSA_CMEVENT("addFilter_V4 RETURN (%d)", rc_maf);
	DEBUG_OAMSA_CMEVENT("addFilter_V4: consumerHandle=%lu, eventType=[%s], filter=%p", consumerHandle, eventType, filter);
	LEAVE_OAMSA_CMEVENT();
	return rc_maf;
}

/**
 * When a producer sends a notification for an event type, this function
 * is called on all registered subscribers that want this type of notification,
 * and that matches their filters. (Note that the filtering is done by the
 * producer).
 *
 * If the filters for consumers of this notification type is an null
 * terminated array, then all these consumers will be notified.
 * The value can only be expected to exist during the call, thus it must
 * be copied if kept.
 *
 * Notify function is called from the event router in a separate thread.
 * The information is just transferred to this call.
 *
 * NOTE: also supports MafOamSpiNotificationFmEventComponent_3 for LM.
 *
 * @param handle [in] identifier of the consumer
 *
 * @param eventType [in] the type of notification is a unique string.
 *
 * @param filter [in] filter registered by the consumer
 *
 * @param value [in] The value that the is associated with the event, e.g. A DN or a MOC
 * The Subscriber must cast the value to the correct type as defined for this unique type.
 *
 * @return MafOk, or @n
 * MafFailure if an internal error occurred during the notification.
 */
#ifndef UNIT_TEST
static MafReturnT comsaNotify4(MafOamSpiEventConsumerHandleT handle, const char * eventType, MafNameValuePairT **filter, void * value)
#else
MafReturnT comsaNotify4(MafOamSpiEventConsumerHandleT handle, const char * eventType, MafNameValuePairT **filter, void * value)
#endif
{
	ENTER_PMTSA_EVENT();
	SaNtfAlarmNotificationT ntfAlarm;
	SaAisErrorT status = SA_AIS_OK;
	DEBUG_PMTSA_EVENT("comsaNotify4 ENTER eventType=%s", eventType);
	int lenFmEventComponent = strlen(MafOamSpiNotificationFmEventComponent_4); //strlen is same for MafOamSpiNotificationFmEventComponent_3
	if(handle <= 0 || eventType == NULL || filter == NULL || value == NULL) {
		ERR_PMTSA_EVENT("comsaNotify4 one(or more) of the arguments is NULL");
		LEAVE_PMTSA_EVENT();
		return MafInvalidArgument;
	}

	if (MafOamSpiNotificationFm_3 == _mafSpiNtfFmVersion) {
		if (strncmp(eventType, MafOamSpiNotificationFmEventComponent_3, lenFmEventComponent-1)) {
			ERR_PMTSA_EVENT("comsaNotify4 one(or more) of the arguments is NULL");
			LEAVE_PMTSA_EVENT();
			return MafInvalidArgument;
		}
	}
	else {
		if (strncmp(eventType, MafOamSpiNotificationFmEventComponent_4, lenFmEventComponent-1)) {
			ERR_PMTSA_EVENT("comsaNotify4 one(or more) of the arguments is NULL");
			LEAVE_PMTSA_EVENT();
			return MafInvalidArgument;
		}
	}

	MafOamSpiNotificationFmStruct_4T* notification = NULL;
	if (MafOamSpiNotificationFm_3 == _mafSpiNtfFmVersion)
	{
		DEBUG_PMTSA_EVENT("comsaNotify4 MafOamSpiNotificationFmStruct_2T");
		MafOamSpiNotificationFmStruct_2T* notification_2 = (MafOamSpiNotificationFmStruct_2T*)value;
		notification                                     = malloc(sizeof(MafOamSpiNotificationFmStruct_4T));
		notification->dn                                 = notification_2->dn;
		notification->majorType                          = notification_2->majorType;
		notification->minorType                          = notification_2->minorType;
		notification->eventTime                          = notification_2->eventTime;
		notification->additionalText                     = notification_2->additionalText;
		notification->severity                           = notification_2->severity;
		notification->additionalInfo.additionalInfoArr   = NULL;
		notification->additionalInfo.size                = 0;
	}
	else
	{
		DEBUG_PMTSA_EVENT("comsaNotify4 MafOamSpiNotificationFmStruct_4T");
		notification = (MafOamSpiNotificationFmStruct_4T*)value;
	}

	status = saNtfAlarmNotificationAllocate(ntfHandle, /* handle to Notification Service instance */
			&ntfAlarm, /* notification structure */
			0, /* number of correlated notifications */
			notification->additionalText ? strlen((char*)notification->additionalText) + 1 : 0,  /* length of additional text */
			notification->additionalInfo.size, /* number of additional info items */
			0, /* number of specific problems */
			0, /* number of monitored attributes */
			0, /* number of proposed repair actions */
			SA_NTF_ALLOC_SYSTEM_LIMIT); /* use default allocation size */

	if(status != SA_AIS_OK) {
		DEBUG_PMTSA_EVENT("comsaNotify4 saNtfAlarmNotificationAllocate (%d) LEAVE", status);
		if (MafOamSpiNotificationFm_3 == _mafSpiNtfFmVersion) {
			free(notification);
		}
		LEAVE_PMTSA_EVENT();
		return MafFailure;
	}

	if(convertMafHeader4ToNtf(notification, &ntfAlarm.notificationHeader, ntfAlarm.notificationHandle) != MafOk) {
		ERR_PMTSA_EVENT("comsaNotify4 convertMafHeader4ToNtf LEAVE");
		saNtfNotificationFree(ntfAlarm.notificationHandle);
		if (MafOamSpiNotificationFm_3 == _mafSpiNtfFmVersion) {
			free(notification);
		}
		LEAVE_PMTSA_EVENT();
		return MafFailure;
	}
	*ntfAlarm.perceivedSeverity = (SaNtfSeverityT)notification->severity;

	*ntfAlarm.probableCause = SA_NTF_UNSPECIFIED_REASON;

	if (strcmp("lm", _CC_NAME) == 0) {
		*ntfAlarm.trend = SA_NTF_TREND_NO_CHANGE; //for LM.
	}

	(*ntfAlarm.thresholdInformation).thresholdId = 0xFF;

	status = saNtfNotificationSend(ntfAlarm.notificationHandle);
	DEBUG_PMTSA_EVENT("comsaNotify4 saNtfNotificationSend %d", status);

	saNtfNotificationFree(ntfAlarm.notificationHandle);

	if (MafOamSpiNotificationFm_3 == _mafSpiNtfFmVersion) {
		free(notification);
	}

	if(status != SA_AIS_OK) {
		ERR_PMTSA_EVENT("comsaNotify4 saNtfNotificationFree (%d) LEAVE", status);
		LEAVE_PMTSA_EVENT();
		return MafFailure;
	}

	DEBUG_PMTSA_EVENT("comsaNotify4 (MafOk) LEAVE");
	LEAVE_PMTSA_EVENT();
	return MafOk;
}

/**
 * removeFilter is called by the event service whenever the producer
 * needs to be updated about events that are no longer subscribed for
 *
 * @param consumerId [in] consumerId identifier of the consumer
 *
 * @param eventType [in] the type of event
 *
 * @param filter [in] the filter instance that was previously added.
 *
 * @return MafOk, or @n
 * MafInvalidArgument if a parameter is not valid, or @n
 * MafFailure if an internal error occurred.
 */
MafReturnT removeFilter_V4(
	MafOamSpiEventConsumerHandleT consumerHandle,
	const char * eventType,
	MafNameValuePairT ** filter)
{
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("removeFilter_V4 ENTER eventType %s consumerHandle %lu filter %p", eventType, consumerHandle, filter);
	SaAisErrorT rc = SA_AIS_OK;
	MafReturnT rc_maf = MafOk;

	if (pthread_mutex_lock(&notify_lock) != 0) {
		ERR_PMTSA_EVENT("removeFilter_V4 pthread_mutex_lock(&notify_lock) != 0");
		LEAVE_PMTSA_EVENT();
		abort();
	}

	if(nFilter_V4.consumerHandle == consumerHandle &&
				nFilter_V4.filter == filter)
	{
		OAM_SA_NTF_RETRY(saNtfNotificationUnsubscribe(nFilter_V4.subscriptionId));
		if (SA_AIS_OK == rc || SA_AIS_ERR_NOT_EXIST == rc) {
			nFilter_V4.consumerHandle = 0;
			nFilter_V4.eventType = NULL;
			nFilter_V4.filter = NULL;
			nFilter_V4.subscriptionId = 0;
		}
		else
		{
			WARN_PMTSA_EVENT("removeFilter_V4 - saNtfNotificationUnsubscribe Failed to unsubscribe for %s, rc = %d ", nFilter_V4.filter[0]->name,rc);
			rc_maf = MafFailure;
		}
	}
	else
	{
		WARN_PMTSA_EVENT("removeFilter_V4 - filter not found");
		rc_maf = MafFailure;
	}

	if (pthread_mutex_unlock(&notify_lock) != 0) {
		ERR_PMTSA_EVENT("removeFilter_V4 pthread_mutex_unlock(&notify_lock) != 0");
		LEAVE_PMTSA_EVENT();
		abort();
	}
	DEBUG_PMTSA_EVENT("removeFilter_V4 LEAVE");
	LEAVE_PMTSA_EVENT();
	return rc_maf;
}


/**
 * clearAll can be called by the Event Router to restart from a known state.
 * When called the producer will discard all filters.
 *
 * @return MafOk, or @n
 * MafInvalidArgument if a parameter is not valid, or @n
 * MafFailure if an internal error occurred.
 */
MafReturnT clearAll_V4( void ){
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("clearAll_V4 ENTER");
	MafReturnT rc = MafOk;
	if (nFilter_V4.consumerHandle != 0) {
		rc = removeFilter_V4(nFilter_V4.consumerHandle, nFilter_V4.eventType, nFilter_V4.filter);
	}
	if (rc != MafOk) {
		rc = MafFailure;
	}
	DEBUG_PMTSA_EVENT("clearAll_V4 LEAVE");
	LEAVE_PMTSA_EVENT();
	return rc;
}

static MafReturnT doneWithValue(
	const char *eventType,
	void * value)
{
	ENTER_OAMSA_ALARM();
	MafReturnT rv = MafNotExist;
	DEBUG_OAMSA_ALARM("doneWithValue ENTER");
	if (pthread_mutex_lock(&list_lock) != 0) {
		LEAVE_OAMSA_ALARM();
		abort();
	}
	if (eventType != NULL)
		DEBUG_OAMSA_ALARM("doneWithValue value (%s)", eventType);
	if (value == NULL) {
		WARN_OAMSA_ALARM( "doneWithValue value == NULL" );
		rv = MafFailure;
	}
	// Determine which notification event type has been used.
	else if ((eventType == NULL)
			 || (strcmp( MafOamSpiNotificationFmEventType_4, eventType ) == 0)
			 || (strcmp( MafOamSpiNotificationFmEventType_2, eventType ) == 0))
	{
		valueList4T *tmp = nVlist4;
		valueList4T *prev = nVlist4;
		MafOamSpiNotificationFmStruct_4T* inV;

		// Find and delete the notification from the value list
		if (MafOamSpiNotificationFm_4 == _mafSpiNtfFmVersion)
		{
			DEBUG_OAMSA_CMEVENT("doneWithValue MafOamSpiNotificationFmEventType_4");
			inV = (MafOamSpiNotificationFmStruct_4T*)value;
			while (tmp != NULL) {
				if (tmp->comNot == inV) {
					break;
				}
				prev = tmp; tmp = tmp->next;
			}
		}
		else if (MafOamSpiNotificationFm_3 == _mafSpiNtfFmVersion)
		{
			DEBUG_OAMSA_CMEVENT("doneWithValue MafOamSpiNotificationFmEventType_2");
			MafOamSpiNotificationFmStruct_2T* inV_2 = (MafOamSpiNotificationFmStruct_2T*)value;
			while (tmp != NULL) {
				if (tmp->comNot_2 == inV_2) {
					inV = tmp->comNot;
					free(inV_2);
					break;
				}
				prev = tmp; tmp = tmp->next;
			}
		}

		if (NULL != tmp)
		{
			if(tmp == nVlLast4){
				if(nVlist4 == nVlLast4){ /* last left in list */
					nVlLast4 = NULL;
					nVlist4 = NULL;
				} else { /* last in list */
					nVlLast4 = prev;
					nVlLast4->next = NULL;
				}
			} else  if (tmp == nVlist4) { /* first in list */
				nVlist4 = tmp->next;
			} else  {
				prev->next = tmp->next;
			}
			/* free and unlink from list */
			if (NULL != inV->dn) {
				free(inV->dn);
				inV->dn = NULL;
			}
			if (NULL != inV->additionalText) {
				free(inV->additionalText);
				inV->additionalText = NULL;
			}
			freeAdditionalInfo(inV);
			free(inV);
			free(tmp);
			rv = MafOk;
		}
	}
	else {
		WARN_OAMSA_ALARM( "doneWithValue Faulty call to doneWithValue." );
		rv = MafFailure;
	}
	if (pthread_mutex_unlock(&list_lock) != 0) {
		LEAVE_OAMSA_ALARM();
		abort();
	}
	DEBUG_OAMSA_ALARM("doneWithValue LEAVE");
	LEAVE_OAMSA_ALARM();
	return rv;
}

/**
 * doneWithValue, Event router calls this function to notify the Producer that the
 * the value is not needed anymore. The producer will know how to delete the value.
 *
 * @param eventType [in] the type of event. It will help the producer how to cast the
 *                  value before deleting it.
 *
 * @param value [in] the value that is no longer needed.
 *
 * @return MafOk, or @n
 * MafInvalidArgument if a parameter is not valid, or @n
 * MafFailure if an internal error occurred.
 */
static MafReturnT doneWithValue_V4(
	const char *eventType,
	void * value)
{
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("doneWithValue_V4 ENTER");
	LEAVE_PMTSA_EVENT();
	return (MafReturnT) doneWithValue(eventType, value);
}

#ifndef UNIT_TEST
static MafReturnT comsaNotify(MafOamSpiEventConsumerHandleT handle, const char * eventType, MafNameValuePairT **filter, void * value) {
#else
MafReturnT comsaNotify(MafOamSpiEventConsumerHandleT handle, const char * eventType, MafNameValuePairT **filter, void * value) {
#endif
	ENTER_OAMSA_ALARM();
	MafReturnT retval = MafInvalidArgument;
	DEBUG_OAMSA_ALARM("comsaNotify eventType=%s", eventType);
	retval = comsaNotify4(handle, eventType, filter, value);
	DEBUG_OAMSA_ALARM("comsaNotify - leave");
	LEAVE_OAMSA_ALARM();
	return retval;
}

/* SDP1694 - support MAF SPI */

/* ----------------------------------------------------------------------
 * Interface functions;
 */

/**
	* Sets up things needed for the service and calls
	* saNtfInitialize, saNtfSelectionObjectGet to enable use of
	* the SA Forum Ntf service.
	*
	* @return MafOk or MafFailure.
	*/
MafReturnT maf_ComNtfServiceOpen(MafMgmtSpiInterfacePortal_3T* portal_MAF)
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("maf_ComNtfServiceOpen ENTER");
	SaAisErrorT rc;
	MafReturnT retval;
	int rv;
	SaVersionT ver = ntfApiVersion;
	SaSelectionObjectT selObj;
	event_router = NULL;
	event_router_MAF = NULL;
	addFilter_setup = false;
	is_ntf_stop_component = false;

	//Reset the global variables
	readOldAlarm = false;
	lastNotificationMatched = false;
	readOldAlarmDone = false;
	sReaderMaxRetries = 0;
	callbackLastCmwNotificationID = 0;
	readerLastNotificationID = 0;
	alarmQueue = NULL;
	alarmQueueFront = NULL;

	// Coremw / opensaf callbacks
	struct pollfd fds[1];
	SaNtfCallbacksT ntfCallbacks = {
		saNtfNotificationCallback4,
		saNtfNotificationDiscardedCallback
	};

	nFilter_V4.consumerHandle = 0;
	nFilter_V4.eventType = NULL;
	nFilter_V4.filter = NULL;
	nFilter_V4.subscriptionId = 0;

	nRetries = 0;
	do {
		ver = ntfApiVersion; /* version is in/out parameter */
		// Initializes the Notification Service
		// Change this LOG to DEBUG after initial testing
		LOG_OAMSA_CMEVENT("saNtfInitialize ntfCallbacks, expecting version %c %u %u", ver.releaseCode, ver.majorVersion, ver.minorVersion);
		rc = saNtfInitialize(&ntfHandle, &ntfCallbacks, &ver);
		if (nRetries)
			usleep(OAM_SA_NTF_RETRY_SLEEP);
	} while(rc == SA_AIS_ERR_TRY_AGAIN && nRetries++ < OAM_SA_NTF_MAX_RETRIES);
	if (SA_AIS_OK != rc) {
		ERR_OAMSA_CMEVENT("saNtfInitialize failed - %d", rc);
		LEAVE_OAMSA_CMEVENT();
		return maf_coremw_rc(rc);
	}
	else
	{
		// Change this LOG to DEBUG after initial testing
		LOG_OAMSA_CMEVENT("saNtfInitialize ntfCallbacks, returned version %c %u %u", (char) ver.releaseCode, ver.majorVersion, ver.minorVersion);
	}

	rc = saNtfSelectionObjectGet(ntfHandle, &selObj);
	if (SA_AIS_OK != rc) {
		err_quit("saNtfSelectionObjectGet failed %u", rc);
	}
	fds[0].fd = (int)selObj;
	fds[0].events = POLLIN;
	rv = poll_setcallback(comSASThandle_ntf, dispatch_ntf, fds[0], NULL);
	if (rv != 0) {
		err_quit("poll_setcallback FAILED %u", rc);
	}

	// LM also using MafOamSpiNotificationFm_4 instead MafOamSpiNotificationFm_3
	DEBUG_OAMSA_CMEVENT("maf_ComNtfServiceOpen() MafOamSpiNotificationFm_4 in use.");
	_mafSpiNtfFmVersion = MafOamSpiNotificationFm_4;

	// Get event router interface
	/*
	* If MAF portal is not NULL then get event router interface from MAF portal
	* This is only for safety. Anyway we will get the MAF portal if there is no MAF!!!
	*/
	DEBUG_OAMSA_CMEVENT("maf_ComNtfServiceOpen - portal_MAF->getInterface");
	retval =  portal_MAF->getInterface(
			MafOamSpiEventService_1Id,
			(MafMgmtSpiInterface_1T**)&event_router_MAF);
	if( MafOk != retval ) {
		LOG_OAMSA_CMEVENT( "Failed to get event router interface from MAF portal( retval: %d ). Continuing without opening MAF services", retval);
	}

	/*
	* Starting CM event handler
	* which handles the wrapper of event producer SPI for CM events
	*/
	if(portal_MAF != NULL)
	{
		start_CmEventHandler(portal_MAF, &subscribeFor_CM_Notifications, &unsubscribeFor_CM_Notifications);
	} // end of starting CM event handler

	if(portal_MAF != NULL && event_router_MAF != NULL)
	{
		DEBUG_OAMSA_CMEVENT("maf_ComNtfServiceOpen - event_router_MAF->registerProducer(producer_if_V4)");
		// register producer
		retval = event_router_MAF->registerProducer(&producer_if_V4,
		                                            &producer_handle);
		if( MafOk != retval ) {
			ERR_OAMSA_CMEVENT( "maf_ComNtfServiceOpen Failed registerProducer producer_if_V4 ( retval: %d ).", retval );
			LEAVE_OAMSA_CMEVENT();
			return retval;
		}

		if (MafOamSpiNotificationFm_3 == _mafSpiNtfFmVersion)
		{
			DEBUG_OAMSA_CMEVENT("maf_ComNtfServiceOpen - event_router_MAF->addProducerEvent(MafOamSpiNotificationFmEventType_2)");
			// Register event type
			retval = event_router_MAF->addProducerEvent(producer_handle,
			                                            MafOamSpiNotificationFmEventType_2);
			if( MafOk != retval ) {
				ERR_OAMSA_CMEVENT( "maf_ComNtfServiceOpen Failed addProducerEvent MafOamSpiNotificationFmEventType_2 ( retval: %d ).", retval );
				LEAVE_OAMSA_CMEVENT();
				return retval;
			}
		}
		else
		{
			DEBUG_OAMSA_CMEVENT("maf_ComNtfServiceOpen - event_router_MAF->addProducerEvent(MafOamSpiNotificationFmEventType_4)");
			// Register event type
			retval = event_router_MAF->addProducerEvent(producer_handle,
			                                            MafOamSpiNotificationFmEventType_4);
			if( MafOk != retval ) {
				ERR_OAMSA_CMEVENT( "maf_ComNtfServiceOpen Failed addProducerEvent MafOamSpiNotificationFmEventType_4 ( retval: %d ).", retval );
				LEAVE_OAMSA_CMEVENT();
				return retval;
			}
		}

		// Register consumer
		DEBUG_OAMSA_CMEVENT("maf_ComNtfServiceOpen - event_router_MAF->registerConsumer(consumer_if) notify (%p)", consumer_if.notify);
		retval = event_router_MAF->registerConsumer((MafOamSpiEventConsumer_1T*) &consumer_if,
		                                            (MafOamSpiEventConsumerHandleT*) &consumer_handle);
		if( MafOk != retval ) {
			ERR_OAMSA_CMEVENT( "Failed registerConsumer consumer_if ( retval: %d ).", retval );
			LEAVE_OAMSA_CMEVENT();
			return retval;
		}

		setupInterface();
	}
	LOG_OAMSA_CMEVENT("com_sa ntf interface opened");
	LEAVE_OAMSA_CMEVENT();
	return retval;
}


/**
	* Cleans things up. Calls saNtfFinalize to end use of the SA
	* Forum NTF service.
	*
	* @return MafOk or MafFailure.
	*/
MafReturnT maf_ComNtfServiceClose()
{
	ENTER_OAMSA_CMEVENT();
	DEBUG_OAMSA_CMEVENT("maf_ComNtfServiceClose ENTER");
	SaAisErrorT rc;
	MafReturnT retval = MafOk;
	SaSelectionObjectT selObj;
	ntf_stop_thread = 1;
	/*When there are ongoing notifications being processed by the ntf_thread (cmNotificationCallback),
	  amf_thread blocks until the processing is completed before closing the NtfService*/
	if (pthread_mutex_lock(&cm_notification_lock) != 0)
        {
                ERR_OAMSA_CMEVENT("maf_ComNtfServiceClose: Failed to lock mutex");
        }

	//ntf_stop_thread = 1;
	// unregister producer
	if( event_router ) {
		// Remove event router interface
		event_router = NULL;
	}
	rc = saNtfSelectionObjectGet(ntfHandle, &selObj);
	if (SA_AIS_OK != rc) {
		WARN_OAMSA_CMEVENT("Warning: saNtfSelectionObjectGet failed %u", rc);
	} else {
		poll_unsetcallback(comSASThandle_ntf,(int)selObj);
	}

	if(event_router_MAF != NULL)
	{
		if (MafOamSpiNotificationFm_3 == _mafSpiNtfFmVersion)
		{
			DEBUG_OAMSA_CMEVENT("maf_ComNtfServiceClose - removeProducerEvent MafOamSpiNotificationFmEventType_2");
			retval = event_router_MAF->removeProducerEvent(
					producer_handle,
					MafOamSpiNotificationFmEventType_2);
		}
		else
		{
			DEBUG_OAMSA_CMEVENT("maf_ComNtfServiceClose - removeProducerEvent MafOamSpiNotificationFmEventType_4");
			retval = event_router_MAF->removeProducerEvent(
					producer_handle,
					MafOamSpiNotificationFmEventType_4);
		}

		if( MafOk != retval ) {
			WARN_OAMSA_CMEVENT("Failed to remove producer event MafOamSpiNotificationFmEventType_4 ( retval: %d ).", retval );
			LEAVE_OAMSA_CMEVENT();
			return retval;
		}

		DEBUG_OAMSA_CMEVENT("maf_ComNtfServiceClose - unregisterProducer producer_if_V4");
		retval = event_router_MAF->unregisterProducer(
				producer_handle,
				&producer_if_V4);
		if( MafOk != retval ) {
			WARN_OAMSA_CMEVENT( "Failed to remove producer interface producer_if_V4 ( retval: %d ).", retval );
			LEAVE_OAMSA_CMEVENT();
			return retval;
		}

		if (MafOamSpiNotificationFm_3 == _mafSpiNtfFmVersion)
		{
			DEBUG_OAMSA_CMEVENT("maf_ComNtfServiceClose - removeSubscription MafOamSpiNotificationFmEventComponent_3");
			retval = event_router_MAF->removeSubscription((MafOamSpiEventConsumerHandleT)consumer_handle,
			                                               MafOamSpiNotificationFmEventComponent_3,
			                                              (MafNameValuePairT **)nullNameValuePair);
		}
		else
		{
			DEBUG_OAMSA_CMEVENT("maf_ComNtfServiceClose - removeSubscription MafOamSpiNotificationFmEventComponent_4");
			retval = event_router_MAF->removeSubscription((MafOamSpiEventConsumerHandleT)consumer_handle,
			                                               MafOamSpiNotificationFmEventComponent_4,
			                                              (MafNameValuePairT **)nullNameValuePair);
		}
		if( MafOk != retval ) {
			WARN_OAMSA_CMEVENT( "Failed to remove a subscription V4 ( retval: %d ).", retval );
			LEAVE_OAMSA_CMEVENT();
			return retval;
		}

		DEBUG_OAMSA_CMEVENT("maf_ComNtfServiceClose - unregisterConsumer");
		retval = event_router_MAF->unregisterConsumer((MafOamSpiEventConsumerHandleT)consumer_handle,
		                                              (MafOamSpiEventConsumer_1T *)&consumer_if);
		if( MafOk != retval ) {
			WARN_OAMSA_CMEVENT( "Failed to unregister a consumer ( retval: %d ).", retval );
			LEAVE_OAMSA_CMEVENT();
			return retval;
		}
		// Remove MAF event router interface
		event_router_MAF = NULL;

		addFilter_setup = false;
	}

	if (strcmp("lm", _CC_NAME)) {
		/*
		 * Stop CmEventHandler
		 * This will unregister the producer of CM events
		 */
		stop_CmEventHandler();
		// end of stopping CM event handler
	}

	OAM_SA_NTF_RETRY(saNtfFinalize(ntfHandle));
	if (SA_AIS_OK != rc) {
		ERR_OAMSA_CMEVENT("saNtfFinalize failed - %d", rc);
	}
	ntf_stop_thread = 0;
	is_ntf_stop_component = true;
	cleanVlist4();
	if (pthread_mutex_unlock(&cm_notification_lock) != 0)
                {
                        ERR_OAMSA_CMEVENT("Failed to unlock mutex");
                }
	LOG_OAMSA_CMEVENT("com_sa ntf interface closed");
	retval = maf_coremw_rc(rc);
	DEBUG_OAMSA_CMEVENT("maf_ComNtfServiceClose LEAVE");
	LEAVE_OAMSA_CMEVENT();
	return retval;
}

/* MR38690 additions - Support UUID in addtionalText */
/**
 * Return the UUID in additional information from the SaNtfNotificationHeaderT. Multiple UUID is supported
 *
 * @param[in]               nh                  Pointer to NTF header struct
 * @param[in]               notificationHandle  Pointer to the internal notification structure
 * @return a string with format: "";" + ("uuid:" + uuid1) + ("," + uuid2) + ("," + uuid3) ... ("," + uuid_last)" or NULL if cannot get any UUID
 * Should free memory after finishing with it
 */

char* getUUIDFromAdditionalInfo(const SaNtfNotificationHeaderT *nh, const SaNtfNotificationHandleT notificationHandle)
{
	ENTER_PMTSA_EVENT();
	DEBUG_PMTSA_EVENT("getUUIDFromAdditionalInfo ENTER");

	char *retUUIDs = NULL;

	if (uuid_cfg_flag == UuidMapValueToAddText && nh->notificationClassId->vendorId == ERICSSON_VENDOR_ID && nh->numAdditionalInfo > 0)
	{
		int iAddInfo = 0;
		int iNumUUIDInfo = 0;

		// First loop through to count number of UUID to allocate memory for strValue
		for (iAddInfo = 0; iAddInfo < nh->numAdditionalInfo; iAddInfo++)
		{
			if (nh->additionalInfo[iAddInfo].infoId == UUID && nh->additionalInfo[iAddInfo].infoType == SA_NTF_VALUE_STRING)
			{
				iNumUUIDInfo++;
			}
		}

		if (iNumUUIDInfo > 0)
		{
			char *strValue = NULL;
			char *header = UUID_ADDTEXT_HEADER;
			int lenOfHeader = strlen(header);
			// for each addInfo, allocate UUID_LENGTH plus 1 for the "," separator
			// No need to allocate for terminated character since we have a redundant "," at the end
			retUUIDs = (char *)calloc(lenOfHeader + iNumUUIDInfo*(UUID_LENGTH + 1),sizeof(char));
			if (retUUIDs == NULL)
			{
				DEBUG_PMTSA_EVENT("getUUIDFromAdditionalInfo Couldn't allocate memory for retUUIDs!");
				LEAVE_PMTSA_EVENT();
				return NULL;
			}

			char* destPtr = retUUIDs;
			destPtr += lenOfHeader;
			char* startingPtr = destPtr;
			for (iAddInfo = 0; iAddInfo < nh->numAdditionalInfo; iAddInfo++)
			{
				if (nh->additionalInfo[iAddInfo].infoId == UUID && nh->additionalInfo[iAddInfo].infoType == SA_NTF_VALUE_STRING)
				{
					strValue = getAdditionalInfoName(notificationHandle, &nh->additionalInfo[iAddInfo]);
					if (strValue != NULL && strlen(strValue) <= UUID_LENGTH)
					{
						memcpy(destPtr, strValue, strlen(strValue));
						destPtr += strlen(strValue);
						// Append ","
						memcpy(destPtr, ",", 1);
						destPtr++;
					}
				}
			}

			if (destPtr > startingPtr)
			{
				// Replace last comma with the terminating null-character and Append header
				destPtr--;
				memset(destPtr, '\0', 1);
				// Append header
				memcpy(retUUIDs, header, lenOfHeader);
			}
			else
			{
				// There is no UUID, free retUUIDs and return NULL
				free(retUUIDs);
				retUUIDs = NULL;
			}
		}
		else
		{
			DEBUG_PMTSA_EVENT("getUUIDFromAdditionalInfo No UUID in additional Info");
		}
	}
	DEBUG_PMTSA_EVENT("getUUIDFromAdditionalInfo retUUIDs: %s", retUUIDs != NULL ? retUUIDs : "NULL");
	DEBUG_PMTSA_EVENT("getUUIDFromAdditionalInfo LEAVE");
	LEAVE_PMTSA_EVENT();
	return retUUIDs;
}
