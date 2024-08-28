/*
 * ComSANtf_unittest.cc
 *
 *  Created on: May 24, 2012
 *      Author: ejnolsz
 *      This is the unit test file for test of code in the scr/ntf directory and particularly of the functions
 *      comsaNotify and convertMafHeaderToNtf
 *
 *
 *      Modified: xanhdao 2013-10-23: MR24146 support Floation point
 *      Modified: xanhdao 2014-02-27  MR-29443 - align to ECIM FM 4.0
 *      Modified: xdonngu 2014-05-23  Test case for HS62388
 *      Modified: xadaleg 2014-06-10  Add additional coverage
 *      Modified: xadaleg 2014-08-02  MR35347 - increase DN length
 */

#include <list>
#include <map>
#include <string>
#include <iostream>
#include <assert.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <poll.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#include "ComSANtf.h"
#include "CmEventProducer.h"
#include <tr1/memory>
#include "debug_log.h"

typedef struct SaNameInit : SaNameT {
	SaNameInit()
	{
#ifdef SA_EXTENDED_NAME_SOURCE
		memset(_opaque, 0, sizeof(_opaque));
#else
		length = 0;
		memset(value, 0, sizeof(value));
#endif
	}
} SaNameInitT;

// To make this output DEBUG printouts to std out. Change gLog level variable to LOG_LEVEL_DEBUG and remove the comment below
#define REDIRECT_LOG
#ifdef REDIRECT_LOG
#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG printf
#else
#include "trace.h"
#endif

int trace_flag = 0;
UuidCfg uuid_cfg_flag = UuidDisableHandle;

extern "C" void dispatch_ntf(struct pollfd* pfd, void *ref);
extern "C" void cleanVlist(void);
extern "C" void DebugDumpAttributeContainer(MafMoAttributeValueContainer_3T *ctp);
extern "C" int parseDnValues(const char* hayStack, const char* needle, char* resultBuffer, const int maxlen);
extern "C" char* findThatEcimMeasReader(SaImmHandleT immOmHandle, const SaNameT* ecimMeasTypeDn, const char* theJobName);
extern "C" MafReturnT convertPmThresholdHeader4(const SaNtfNotificationsT *notification, MafOamSpiNotificationFmStruct_4T *comNot);
extern "C" bool convertTo3GPPDN(SaNameT *no, char **dn);
extern "C" bool setValueInCmEvent( MafMoAttributeValue_3T* Pointer, MafOamSpiMoAttributeType_3T type, void* value, SaNtfValueTypeT ntfType);
extern "C" MafReturnT comsaNotify(MafOamSpiEventConsumerHandleT handle, const char * eventType, MafNameValuePairT **filter, void * value);
extern "C" MafReturnT comsaNotify4(MafOamSpiEventConsumerHandleT handle, const char * eventType, MafNameValuePairT **filter, void * value);
extern "C" MafReturnT convertHeader4(const SaNtfNotificationsT *notification, MafOamSpiNotificationFmStruct_4T *comNot);
extern "C" MafReturnT clearAll_V4(void);
extern "C" void saNtfNotificationCallback4(SaNtfSubscriptionIdT subscriptionId, const SaNtfNotificationsT *notification);
extern "C" void freeAdditionalInfo(MafOamSpiNotificationFmStruct_4T *comNot);
extern "C" void cleanVlist4( void );
extern "C" void addNotValue4(MafOamSpiNotificationFmStruct_4T *comNot);
extern "C" void saNtfNotificationDiscardedCallback(SaNtfSubscriptionIdT subscriptionId, SaNtfNotificationTypeT notificationType, SaUint32T numberDiscarded, const SaNtfIdentifierT *discardedNotificationIdentifier);

extern MafOamSpiEventRouter_1T *event_router_MAF = NULL;
extern MafFilterT nFilter_V4;
extern valueList4T *nVlist4 = NULL;
extern valueList4T *nVlLast4 = NULL;

std::map<int, std::string> myTestDataAttributesNames;
std::map<int, std::string> myTestDataValues;
bool saNtfPtrValGetTestReturnValue = true;
void setSaNtfAdditionalInfoT(int infoId, SaNtfValueTypeT infoType, SaNtfAdditionalInfoT *additionalInfo, SaNtfValueT infoValue);

SaAisErrorT saNtfDispatch(SaNtfHandleT ntfHandle, SaDispatchFlagsT dispatchFlags){
	return SA_AIS_OK;
}

struct timespec ts;
int clock_gettime (clockid_t __clock_id, struct timespec *__tp)
{
	ts.tv_nsec = 10;
	ts.tv_sec = 10;
	return 0;
}


typedef std::vector<std::string> filterListT;

SaAisErrorT subscriberFunction(void)
{
	return SA_AIS_OK;
}

SaAisErrorT unsubscriberFunction(void)
{
	return SA_AIS_OK;
}

bool returnValuefillStructMembers = true;
bool  fillStructMembers(MafMoAttributeValueContainer_3T **structAttributePointer, char *immRdn, char *saAttributeName)
{
	*structAttributePointer = (MafMoAttributeValueContainer_3T*)calloc(1, sizeof(MafMoAttributeValueContainer_3T));
	MafMoAttributeValueContainer_3T *pointCmevent = *structAttributePointer;
	pointCmevent->nrOfValues = 1;
	pointCmevent->type = MafOamSpiMoAttributeType_3_STRING;
	pointCmevent->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	pointCmevent->values->value.theString = (const char*)calloc(strlen("TESTNAMEVALUE")+1, sizeof(char));
	strcpy((char*)(pointCmevent->values->value.theString), "TESTNAMEVALUE");
	return returnValuefillStructMembers;
}

char *structTest = "JOHNNYTESTATTRIBUTE";
char *getStructAttributeName(char *immRdn, char *saAttributeName)
{
	return (char*)structTest;
}

void setCmNotificationStruct(const char *dn, MafOamSpiCmEvent_Notification_1T *cmNotStruct)
{
	cmNotStruct->txHandle = 0;
	cmNotStruct->eventTime = 1346475867467378548LL;
	cmNotStruct->sourceIndicator = MafOamSpiCmEvent_ManagementOperation_1;
	cmNotStruct->events = (MafOamSpiCmEvent_1T **) malloc(2 * sizeof(MafOamSpiCmEvent_1T *));
	cmNotStruct->events[0] = (MafOamSpiCmEvent_1T *) malloc(sizeof(MafOamSpiCmEvent_1T));
	cmNotStruct->events[0]->dn = strdup(dn);
	cmNotStruct->events[0]->eventType = MafOamSpiCmEvent_AttributeValueChange_1;
	cmNotStruct->events[0]->attributes = NULL;
	cmNotStruct->events[1] = NULL;
}

void setFilterArray(filterListT cmFilterList, MafNameValuePairT **filter)
{
	// allocate memory for the filters
	for(size_t i = 0; i < cmFilterList.size(); i++)
	{
		filter[i] = new MafNameValuePairT;
		// fill in the data
		filter[i]->name = strdup(MafOamSpiCmEvent_FilterTypeRegExp_1);
		filter[i]->value = strdup(cmFilterList.at(i).c_str());
	}
	filter[cmFilterList.size()] = NULL;
}

// This function tests the regexp filter functionality inside the CM event producer
bool testRegexpFilter(const char *dn, filterListT filterList)
{
	// Create CM event struct instance
	MafOamSpiCmEvent_Notification_1T cmEventStruct;

	// Fill in the instance with the given DN.
	// Other params are also filled in but with hardcoded values, because they are not used for this test, but must be filled in anyway.
	setCmNotificationStruct(dn, &cmEventStruct);

	// Create CmNotification instance. This will be the input for the isMatch function
	CmEventProducer::CmNotification CmNot(cmEventStruct);

	// Allocate memory for the CM notification filter array
	MafNameValuePairT** regexpFilters = new MafNameValuePairT*[filterList.size()+1];
	// Fill in the CM notification filter array with the filter expressions
	setFilterArray(filterList, regexpFilters);

	// Create CmEventProducer instance to be able to create RegexpFilter which is its child class
	CmEventProducer EP(&subscriberFunction, &unsubscriberFunction);

	// Create RegexpFilter instance
	CmEventProducer::RegexpFilter *RF = EP.createRegexpFilterInstance(regexpFilters);

	// Check if the regular expressions in the filter array are valid
	if(!RF->isValid())
	{
		for(int i = 0; i < filterList.size(); i++)
		{
			free((void *)regexpFilters[i]->name);
			free((void *)regexpFilters[i]->value);
			delete regexpFilters[i];
		}
		delete []regexpFilters;
		delete RF;
		return false;
	}
	// Test isMatch function
	// Note: isMatch tests only the filter elements (regular expressions) against the previously set DN inside the CmNot (CM event struct).
	bool result = RF->isMatch(CmNot);
	for(int i = 0; i < filterList.size(); i++)
	{
		free((void *)regexpFilters[i]->name);
		free((void *)regexpFilters[i]->value);
		delete regexpFilters[i];
	}
	delete []regexpFilters;
	delete RF;
	return result;
}

SaAisErrorT saImmOmSearchFinalize(SaImmSearchHandleT searchHandle) {
	printf("----> saImmOmSearchFinalize \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmInitialize(SaImmHandleT *immHandle,
		const SaImmCallbacksT *immCallbacks, SaVersionT *version) {
	printf("----> saImmOmInitialize \n");
	(*immHandle) = (SaImmHandleT) 1; // set to value != 0 to inidcate success!
	return SA_AIS_OK;
}


SaAisErrorT saImmOmFinalize(SaImmHandleT immHandle) {
	printf("----> saImmOmFinalize \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmAccessorFinalize(SaImmAccessorHandleT accessorHandle) {
	printf("----> saImmOmAccessorFinalize \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmAccessorInitialize(SaImmHandleT immHandle,
		SaImmAccessorHandleT *accessorHandle) {
	printf("----> saImmOmAccessorInitialize \n");
	(*accessorHandle) = (SaImmAccessorHandleT) 1; // set to value != 0 to show success
	return SA_AIS_OK;
}

SaNameT *saImmOm_objectName1 = NULL;
SaNameT *saImmOm_objectName2 = NULL;
SaNameT *saImmOm_objectName3 = NULL;
SaNameT *saImmOm_objectName4 = NULL;
SaNameT *saImmOm_objectName5 = NULL;
SaImmAttrValuesT_2 **saImmOm_attributes1 = NULL;
SaImmAttrValuesT_2 **saImmOm_attributes2 = NULL;
SaImmAttrValuesT_2 **saImmOm_attributes3 = NULL;
SaImmAttrValuesT_2 **saImmOm_attributes4 = NULL;
SaImmAttrValuesT_2 **saImmOm_attributes5 = NULL;
int iNumCalls = 0;

SaAisErrorT saImmOmAccessorGet_2(SaImmAccessorHandleT accessorHandle,
		const SaNameT *objectName, const SaImmAttrNameT *attributeNames,
		SaImmAttrValuesT_2 ***attributes) {
	SaAisErrorT ret = SA_AIS_OK;
	// used directly by cache
	printf("----> saImmOmAccessorGet_2 \n");
	printf("objectName %s\n", saNameGet(objectName));
	if (attributeNames[0] != NULL)
		printf("attributeNames %s\n", attributeNames[0]);
	switch (iNumCalls) {
	case 1:
		*attributes = saImmOm_attributes1;
		ret = SA_AIS_OK;
		break;
	case 2:
		*attributes = saImmOm_attributes2;
		ret = SA_AIS_OK;
		break;
	case 3:
		*attributes = saImmOm_attributes3;
		ret = SA_AIS_OK;
		break;
	case 4:
		*attributes = saImmOm_attributes4;
		ret = SA_AIS_OK;
		break;
	case 5:
		*attributes = saImmOm_attributes5;
		ret = SA_AIS_OK;
		break;
	default:
		*attributes = NULL;
		ret = SA_AIS_ERR_NOT_EXIST;
		break;
	}
	iNumCalls++;
	return ret;
}

SaAisErrorT saImmOmSearchInitialize_2(SaImmHandleT immHandle,
			   const SaNameT *rootName,
			   SaImmScopeT scope,
			   SaImmSearchOptionsT searchOptions,
			   const SaImmSearchParametersT_2 *searchParam,
			   const SaImmAttrNameT *attributeNames, SaImmSearchHandleT *searchHandle){
	return SA_AIS_OK;
}

SaAisErrorT saImmOmSearchNext_2(SaImmSearchHandleT searchHandle, SaNameT *objectName, SaImmAttrValuesT_2 ***attributes){
	SaAisErrorT ret = SA_AIS_OK;
	printf("----> saImmOmSearchNext_2 iNumCalls %d\n", iNumCalls);
	switch (iNumCalls) {
	case 1:
		*objectName = *saImmOm_objectName1;
		*attributes = saImmOm_attributes1;
		break;
	case 2:
		*objectName = *saImmOm_objectName2;
		*attributes = saImmOm_attributes2;
		break;
	case 3:
		*objectName = *saImmOm_objectName3;
		*attributes = saImmOm_attributes3;
		break;
	case 4:
		*objectName = *saImmOm_objectName4;
		*attributes = saImmOm_attributes4;
		break;
	case 5:
		*objectName = *saImmOm_objectName5;
		*attributes = saImmOm_attributes5;
		break;
	default:
		objectName = NULL;
		*attributes = NULL;
		ret = SA_AIS_ERR_NOT_EXIST;
		break;
	}
	iNumCalls++;
	return ret;
}

SaAisErrorT saNtfAlarmNotificationFilterAllocate(
	SaNtfHandleT ntfHandle,
	SaNtfAlarmNotificationFilterT *notificationFilter,
	SaUint16T numEventTypes,
	SaUint16T numNotificationObjects,
	SaUint16T numNotifyingObjects,
	SaUint16T numNotificationClassIds,
	SaUint16T numProbableCauses,
	SaUint16T numPerceivedSeverities,
	SaUint16T numTrends){
	return SA_AIS_OK;
}

SaAisErrorT saNtfSecurityAlarmNotificationFilterAllocate(
	SaNtfHandleT ntfHandle,
	SaNtfSecurityAlarmNotificationFilterT *notificationFilter,
	SaUint16T numEventTypes,
	SaUint16T numNotificationObjects,
	SaUint16T numNotifyingObjects,
	SaUint16T numNotificationClassIds,
	SaUint16T numProbableCauses,
	SaUint16T numSeverities,
	SaUint16T numSecurityAlarmDetectors,
	SaUint16T numServiceUsers,
	SaUint16T numServiceProviders){
	return SA_AIS_OK;
}

SaAisErrorT saNtfObjectCreateDeleteNotificationFilterAllocate(
	SaNtfHandleT ntfHandle,
	SaNtfObjectCreateDeleteNotificationFilterT *notificationFilter,
	SaUint16T numEventTypes,
	SaUint16T numNotificationObjects,
	SaUint16T numNotifyingObjects,
	SaUint16T numNotificationClassIds,
	SaUint16T numSourceIndicators){
	return SA_AIS_OK;
}

SaAisErrorT saNtfAttributeChangeNotificationFilterAllocate(
	SaNtfHandleT ntfHandle,
	SaNtfAttributeChangeNotificationFilterT *notificationFilter,
	SaUint16T numEventTypes,
	SaUint16T numNotificationObjects,
	SaUint16T numNotifyingObjects,
	SaUint16T numNotificationClassIds,
	SaUint16T numSourceIndicators){
	return SA_AIS_OK;
}

SaAisErrorT
saNtfStateChangeNotificationFilterAllocate(
	SaNtfHandleT ntfHandle,
	SaNtfStateChangeNotificationFilterT *notificationFilter,
	SaUint16T numEventTypes,
	SaUint16T numNotificationObjects,
	SaUint16T numNotifyingObjects,
	SaUint16T numNotificationClassIds,
	SaUint16T numSourceIndicators,
	SaUint16T numChangedStates){
	return SA_AIS_OK;
}

SaAisErrorT saNtfNotificationSubscribe(
	const SaNtfNotificationTypeFilterHandlesT *notificationFilterHandles,
	SaNtfSubscriptionIdT subscriptionId){
	return SA_AIS_OK;
}


SaAisErrorT saNtfNotificationFilterFree(
	SaNtfNotificationFilterHandleT notificationFilterHandle){
	return SA_AIS_OK;
}

SaAisErrorT saNtfNotificationReadInitialize(
	SaNtfSearchCriteriaT searchCriteria,
	const SaNtfNotificationTypeFilterHandlesT *notificationFilterHandles,
	SaNtfReadHandleT *readHandle){
	return SA_AIS_OK;
}

SaAisErrorT saNtfNotificationFree(SaNtfNotificationHandleT notificationHandle){
	return SA_AIS_OK;
}

SaAisErrorT saNtfNotificationReadNext(
	SaNtfReadHandleT readHandle,
	SaNtfSearchDirectionT searchDirection,
	SaNtfNotificationsT *notification){
	return SA_AIS_OK;
}

SaAisErrorT saNtfNotificationReadFinalize(
	SaNtfReadHandleT readHandle){
	return SA_AIS_OK;
}

SaAisErrorT saNtfInitialize(
	SaNtfHandleT *ntfHandle,
	const SaNtfCallbacksT *ntfCallbacks,
	SaVersionT *version){
	return SA_AIS_OK;
}

SaAisErrorT saNtfSelectionObjectGet(
	SaNtfHandleT ntfHandle,
	SaSelectionObjectT *selectionObject){
	return SA_AIS_OK;
}


void* comSASThandle = NULL;
void* comSASThandle_ntf = NULL;

/**
 * Set a call-back function for events on a file descriptor.
 * @param handle A handle returned by ::timerCreateHandle_r.
 * @fn The call-back function.
 * @pfd Defines the file descriptor and the events to monitor.
 * @ref A user reference that is passed back in the call-back.
 * @return 0 on success, -1 if failed.
 */

int poll_setcallback(void* handle, FdCallbackFn fn, struct pollfd pfd, void *ref)
{
	return 0;
}

SaAisErrorT saNtfNotificationUnsubscribe(
	SaNtfSubscriptionIdT subscriptionId)
{
	return SA_AIS_OK;
}

SaAisErrorT saNtfFinalize(
	SaNtfHandleT ntfHandle)
{
	return SA_AIS_OK;
}

SaAisErrorT saNtfNotificationSend(
	SaNtfNotificationHandleT notificationHandle){
	return SA_AIS_OK;
}

void poll_unsetcallback(void* handle, int fd){}

MafReturnT coremw_rc(SaAisErrorT rc)
{
	ENTER();
	LEAVE();
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

/* SDP1694 -support MAF SPI */
MafReturnT maf_coremw_rc(SaAisErrorT rc)
{
	ENTER();
	LEAVE();
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


typedef struct SaNtfAlarmNotificationAuto : SaNtfAlarmNotificationT{
	~SaNtfAlarmNotificationAuto()
	{
		saNameDelete(notificationHeader.notificationObject, true);
		saNameDelete(notificationHeader.notifyingObject, true);
		delete notificationHeader.notificationClassId;
		delete notificationHeader.eventTime;
		delete notificationHeader.eventType;
		delete probableCause;
		delete perceivedSeverity;
		delete trend;
		free(notificationHeader.additionalText);
		delete thresholdInformation;
		delete []notificationHeader.additionalInfo;
	}
} SaNtfAlarmNotificationAutoT;

std::tr1::shared_ptr<SaNtfAlarmNotificationAutoT> Globalnotification;

char * notificationContainer [5] = {NULL,NULL,NULL,NULL,NULL};
int notificationContainerIndex =0;

SaAisErrorT saNtfPtrValAllocate(
	SaNtfNotificationHandleT notificationHandle,
	SaUint16T dataSize,
	void **dataPtr,
	SaNtfValueT *value)
{
	char * tmpStr = (char *)malloc(dataSize);
	*dataPtr = tmpStr;
	notificationContainer[notificationContainerIndex] =tmpStr;
	value->ptrVal.dataOffset =notificationContainerIndex++;
	value->ptrVal.dataSize =dataSize;
	return SA_AIS_OK;
}
 SaAisErrorT saNtfPtrValGet(
	SaNtfNotificationHandleT notificationHandle,
	const SaNtfValueT *value,
	void **dataPtr,
	SaUint16T *dataSize){
	 *dataPtr = notificationContainer[value->ptrVal.dataOffset];
	 return SA_AIS_OK;
 }
void freeAllAdditionInfo()
{
	 for(int i =0 ; i<=4; i++)
	 {
		 if(NULL != notificationContainer[i]) {
			 free(notificationContainer[i]);
			 notificationContainer[i] = NULL;
		 }
	 }
	 notificationContainerIndex = 0;
 }

SaAisErrorT saNtfAlarmNotificationAllocate(
	SaNtfHandleT ntfHandle,
	SaNtfAlarmNotificationT *notification,
	SaUint16T numCorrelatedNotifications,
	SaUint16T lengthAdditionalText,
	SaUint16T numAdditionalInfo,
	SaUint16T numSpecificProblems,
	SaUint16T numMonitoredAttributes,
	SaUint16T numProposedRepairActions,
	SaInt16T variableDataSize)
{
	notification->notificationHeader.notificationObject = new SaNameInitT;
	notification->notificationHeader.notifyingObject = new SaNameInitT;
	notification->notificationHeader.notificationClassId = new SaNtfClassIdT;
	notification->notificationHeader.eventTime = new SaTimeT;
	notification->notificationHeader.eventType = new SaNtfEventTypeT;
	notification->probableCause = new SaNtfProbableCauseT;
	notification->perceivedSeverity = new SaNtfSeverityT;
	notification->trend = new SaNtfSeverityTrendT;
	notification->notificationHeader.additionalText = (SaStringT) malloc (lengthAdditionalText+1);
	notification->thresholdInformation = new SaNtfThresholdInformationT;
	notification->notificationHeader.additionalInfo = new SaNtfAdditionalInfoT [numAdditionalInfo];
	notification->notificationHandle = 0;
	//notification->notificationHandle = new SaNtfNotificationHandleT;
	Globalnotification = std::tr1::shared_ptr<SaNtfAlarmNotificationAutoT>(new SaNtfAlarmNotificationAutoT);
	memcpy(Globalnotification.get(), notification, sizeof(SaNtfAlarmNotificationT));

	return SA_AIS_OK;
}

// MOCKING THe OamSACAche.cc file.
/* Globals */
bool convertToImm(const char *the3GppDN, SaNameT *immDn)
{
	if (strlen(the3GppDN) >= saNameMaxLen()) {
		return false;
	}else {
		return true;
	}
}

/* Globals */
char* convertTo3Gpp(SaNameT* theImmName_p)
{
	std::string text = "JOHNNY";
	char* ReturnString3GPP = (char*)calloc(text.length()+1,sizeof(char));
	strcpy(ReturnString3GPP,text.c_str());
	return ReturnString3GPP;
}
 // End of ...
 // MOCKING THe OamSACAche.cc file.

TEST (NtfTest, dispatch_ntf1)
{
	dispatch_ntf(NULL, NULL);
}

TEST (NtfTest, DebugDumpAttributeContainer1)
{
	MafMoAttributeValueContainer_3T valueCont;
	valueCont.nrOfValues = 1;
	valueCont.values = new MafMoAttributeValue_3T[2];
	DebugDumpAttributeContainer(NULL);

	valueCont.type = MafOamSpiMoAttributeType_3_INT16;
	valueCont.values[0].value.i16 = 1;
	DebugDumpAttributeContainer(&valueCont);

	valueCont.type = MafOamSpiMoAttributeType_3_INT32;
	valueCont.values[0].value.i32 = 2;
	DebugDumpAttributeContainer(&valueCont);

	valueCont.type = MafOamSpiMoAttributeType_3_INT64;
	valueCont.values[0].value.i64 = 3;
	DebugDumpAttributeContainer(&valueCont);

	valueCont.type = MafOamSpiMoAttributeType_3_UINT8;
	valueCont.values[0].value.u8 = 4;
	DebugDumpAttributeContainer(&valueCont);

	valueCont.type = MafOamSpiMoAttributeType_3_UINT16;
	valueCont.values[0].value.u16 = 5;
	DebugDumpAttributeContainer(&valueCont);

	valueCont.type = MafOamSpiMoAttributeType_3_UINT32;
	valueCont.values[0].value.u32 = 6;
	DebugDumpAttributeContainer(&valueCont);

	valueCont.type = MafOamSpiMoAttributeType_3_UINT64;
	valueCont.values[0].value.u64 = 7;
	DebugDumpAttributeContainer(&valueCont);

	valueCont.type = MafOamSpiMoAttributeType_3_BOOL;
	valueCont.values[0].value.theBool = true;
	DebugDumpAttributeContainer(&valueCont);

	valueCont.type = MafOamSpiMoAttributeType_3_REFERENCE;
	valueCont.values[0].value.moRef = "reference";
	DebugDumpAttributeContainer(&valueCont);

	valueCont.type = MafOamSpiMoAttributeType_3_REFERENCE;
	valueCont.values[0].value.moRef = NULL;
	DebugDumpAttributeContainer(&valueCont);

	valueCont.type = MafOamSpiMoAttributeType_3_ENUM;
	valueCont.values[0].value.theEnum = 8;
	DebugDumpAttributeContainer(&valueCont);

	valueCont.type = MafOamSpiMoAttributeType_3_STRUCT;
	valueCont.values[0].value.moRef = "struct";
	DebugDumpAttributeContainer(&valueCont);

	delete [] valueCont.values;
}

TEST (NtfTest, parseDnValues)
{
	char tmpStr1[] = "MRt3";
	char resultBuffer[saNameMaxLen()];

	int nRet = parseDnValues("id=1,measurementReaderId=MRt3,pmJobId=ThresJob1,CmwPmpmId=1", "measurementReaderId=", resultBuffer, saNameMaxLen());

	EXPECT_EQ ( nRet, 1 );
	EXPECT_EQ(0, strcmp( tmpStr1, resultBuffer));
}

TEST (NtfTest, findThatEcimMeasReader1)
{
	SaNameT* ecimMeasTypeDn = makeSaNameT("ecimMeasTypeDn");
	char theJobName[] = "theJobName";
	char expected[] = "MRt3";
	saImmOm_objectName1 = makeSaNameT("id=1,measurementReaderId=MRt3,pmJobId=ThresJob1,CmwPmpmId=1");

	iNumCalls = 1;
	saImmOm_attributes1 = new SaImmAttrValuesT_2*[5];
	saImmOm_attributes1[0] = new SaImmAttrValuesT_2;
	saImmOm_attributes1[0]->attrName = "measurementTypeRef";
	saImmOm_attributes1[0]->attrValueType = SA_IMM_ATTR_SANAMET;
	saImmOm_attributes1[0]->attrValues = (void**) &ecimMeasTypeDn;
	saImmOm_attributes1[0]->attrValuesNumber = 1;
	saImmOm_attributes1[1] = NULL;

	char* result = findThatEcimMeasReader(0, ecimMeasTypeDn, theJobName);

	EXPECT_EQ(0, strcmp( expected, result));

	delete ecimMeasTypeDn;
	delete saImmOm_objectName1;
	saImmOm_objectName1 = NULL;
	delete saImmOm_attributes1[0];
	delete [] saImmOm_attributes1;
	saImmOm_attributes1 = NULL;
	free(result);
}

TEST (NtfTest, convertPmThresholdHeader4)
{
	// In this test case we send Threshold-alarm/notification (PM-services)
	// test for string
	MafReturnT rc = MafOk;

	SaNtfNotificationsT ntf;
	memset(&ntf, 0, sizeof(SaNtfNotificationsT));
	ntf.notificationType = SA_NTF_TYPE_ALARM;
	SaNtfNotificationHeaderT *hd = &ntf.notification.alarmNotification.notificationHeader;
	SaInt64T  tmplli = 1346475867467378548;
	hd->eventTime = &tmplli;
	hd->notificationObject = new SaNameInitT;
	hd->notifyingObject = makeSaNameT("safThreshold=TM1-Warning,SaPmfJobMT=safMT=CcMT-1\\,safMeasObjClass=CcGroup1\\,safPm=1,safJob=ThresJob1,safPm=1");
	hd->additionalText = "measurementTypeId=CcMT-1,pmGroupId=CcGroup1,CmwPmpmId=1";
	hd->notificationId = new SaNtfIdentifierT;
	hd->notificationClassId = new SaNtfClassIdT;
	hd->notificationClassId->vendorId = SA_NTF_VENDOR_ID_SAF;
	hd->notificationClassId->majorId = 20;
	hd->notificationClassId->minorId = 1;

	MafOamSpiNotificationFmStruct_4T *comNot = new MafOamSpiNotificationFmStruct_4T;
	memset(comNot, 0, sizeof(MafOamSpiNotificationFmStruct_4T));


	char value[] = "1234567";
	MafNameValuePairT *filterMaf = new MafNameValuePairT[1];
	nFilter_V4.filter = &filterMaf;
	nFilter_V4.filter[0]->name = NULL;
	nFilter_V4.filter[0]->value = (const char *) &value;

	char theJobName[] = "theJobName";
	char expected[] = "MRt3";
	saImmOm_objectName1 = makeSaNameT("id=1,measurementReaderId=MRt3,pmJobId=ThresJob1,CmwPmpmId=1");

	SaNameT* ecimMeasTypeDn = makeSaNameT("fmAlarmTypeId=PMThresholdCrossed,fmAlarmModelId=PM,fmId=1");
	iNumCalls = 1;
	saImmOm_attributes1 = new SaImmAttrValuesT_2*[5];
	saImmOm_attributes1[0] = new SaImmAttrValuesT_2;
	saImmOm_attributes1[0]->attrName = "fmAlarmType";
	saImmOm_attributes1[0]->attrValueType = SA_IMM_ATTR_SANAMET;
	saImmOm_attributes1[0]->attrValues = (void**) &ecimMeasTypeDn;
	saImmOm_attributes1[0]->attrValuesNumber = 1;
	saImmOm_attributes1[1] = NULL;

	SaUint32T uint32a = 193;
	SaUint32T *aUint32a = &uint32a;
	printf("uint32a %u %p\n", uint32a, &uint32a);
	SaUint32T uint32b = 321;
	SaUint32T *aUint32b = &uint32b;
	saImmOm_attributes2 = new SaImmAttrValuesT_2*[5];
	saImmOm_attributes2[0] = new SaImmAttrValuesT_2;
	saImmOm_attributes2[1] = new SaImmAttrValuesT_2;
	saImmOm_attributes2[2] = new SaImmAttrValuesT_2;
	saImmOm_attributes2[3] = NULL;
	saImmOm_attributes2[0]->attrName = "majorType";
	saImmOm_attributes2[0]->attrValueType = SA_IMM_ATTR_SAUINT32T;
	saImmOm_attributes2[0]->attrValues = (void**) &aUint32a;
	saImmOm_attributes2[0]->attrValuesNumber = 1;
	printf("saImmOm_attributes2[0]->attrValues %p\n", saImmOm_attributes2[0]->attrValues);
	saImmOm_attributes2[1]->attrName = "minorType";
	saImmOm_attributes2[1]->attrValueType = SA_IMM_ATTR_SAUINT32T;
	saImmOm_attributes2[1]->attrValues = (void**) &aUint32b;
	saImmOm_attributes2[1]->attrValuesNumber = 1;
	SaStringT addText = "Default additionalText";
	SaStringT* strAddText = &addText;
	saImmOm_attributes2[2]->attrName = "additionalText";
	saImmOm_attributes2[2]->attrValueType = SA_IMM_ATTR_SASTRINGT;
	saImmOm_attributes2[2]->attrValues = (void**) &strAddText;
	saImmOm_attributes2[2]->attrValuesNumber = 1;

	saImmOm_objectName3 = makeSaNameT("id=1,measurementReaderId=MRt3,pmJobId=ThresJob1,CmwPmpmId=1");
	SaNameT* measurementTypeId = makeSaNameT("measurementTypeId=CcMT-1,pmGroupId=CcGroup1,CmwPmpmId=1");
	saImmOm_attributes3 = new SaImmAttrValuesT_2*[2];
	saImmOm_attributes3[0] = new SaImmAttrValuesT_2;
	saImmOm_attributes3[1] = NULL;
	saImmOm_attributes3[0]->attrName = "measurementTypeRef";
	saImmOm_attributes3[0]->attrValueType = SA_IMM_ATTR_SANAMET;
	saImmOm_attributes3[0]->attrValues = (void**) &measurementTypeId;
	saImmOm_attributes3[0]->attrValuesNumber = 1;

	MafReturnT com_ret = convertPmThresholdHeader4(&ntf, comNot);

	EXPECT_EQ(com_ret, MafOk);

	saNameDelete(measurementTypeId, true);
	free(comNot->dn);
	free(comNot->additionalText);
	delete comNot;
	delete ecimMeasTypeDn;
	delete saImmOm_objectName1;
	delete saImmOm_objectName3;
	delete saImmOm_attributes1[0];
	delete [] saImmOm_attributes1;
	delete saImmOm_attributes2[0];
	delete saImmOm_attributes2[1];
	delete saImmOm_attributes2[2];
	delete [] saImmOm_attributes2;
	delete saImmOm_attributes3[0];
	delete [] saImmOm_attributes3;
	saImmOm_attributes1 = NULL;
	saImmOm_attributes2 = NULL;
	saImmOm_attributes3 = NULL;
	delete []filterMaf;
	delete hd->notificationClassId;
	saNameDelete(hd->notificationObject, true);
	saNameDelete(hd->notifyingObject, true);
}

TEST (NtfTest, convertPmThresholdHeader4GenericAlarm)
{
	// In this test case we send Threshold-alarm/notification (PM-services)
	// test for string
	MafReturnT rc = MafOk;

	SaNtfNotificationsT ntf;
	memset(&ntf, 0, sizeof(SaNtfNotificationsT));
	ntf.notificationType = SA_NTF_TYPE_ALARM;
	SaNtfNotificationHeaderT *hd = &ntf.notification.alarmNotification.notificationHeader;
	SaInt64T  tmplli = 1346475867467378548;
	hd->eventTime = &tmplli;
	hd->notificationObject = new SaNameInitT;
	hd->notifyingObject = makeSaNameT("safThreshold=TM1-Warning,SaPmfJobMT=safMT=CcMT-1\\,safMeasObjClass=CcGroup1\\,safPm=1,safJob=ThresJob1,safPm=1");
	hd->additionalText = "measurementTypeId=CcMT-1,pmGroupId=CcGroup1,CmwPmpmId=1";
	hd->notificationId = new SaNtfIdentifierT;
	hd->notificationClassId = new SaNtfClassIdT;
	hd->notificationClassId->vendorId = SA_NTF_VENDOR_ID_SAF;
	hd->notificationClassId->majorId = 20;
	hd->notificationClassId->minorId = 1;

	MafOamSpiNotificationFmStruct_4T *comNot = new MafOamSpiNotificationFmStruct_4T;
	memset(comNot, 0, sizeof(MafOamSpiNotificationFmStruct_4T));


	char value[] = "1234567";
	MafNameValuePairT *filterMaf = new MafNameValuePairT[1];
	nFilter_V4.filter = &filterMaf;
	nFilter_V4.filter[0]->name = NULL;
	nFilter_V4.filter[0]->value = (const char *) &value;

	char theJobName[] = "theJobName";
	char expected[] = "MRt3";
	saImmOm_objectName1 = makeSaNameT("id=1,measurementReaderId=MRt3,pmJobId=ThresJob1,CmwPmpmId=1");

	SaNameT* ecimMeasTypeDn = makeSaNameT("fmAlarmTypeId=PMThresholdCrossedOrReached,fmAlarmModelId=PM,fmId=1");
	iNumCalls = 1;
	saImmOm_attributes1 = new SaImmAttrValuesT_2*[5];
	saImmOm_attributes1[0] = new SaImmAttrValuesT_2;
	saImmOm_attributes1[0]->attrName = "fmAlarmType";
	saImmOm_attributes1[0]->attrValueType = SA_IMM_ATTR_SANAMET;
	saImmOm_attributes1[0]->attrValues = (void**) &ecimMeasTypeDn;
	saImmOm_attributes1[0]->attrValuesNumber = 1;
	saImmOm_attributes1[1] = NULL;

	SaUint32T uint32a = 193;
	SaUint32T *aUint32a = &uint32a;
	printf("uint32a %u %p\n", uint32a, &uint32a);
	SaUint32T uint32b = 321;
	SaUint32T *aUint32b = &uint32b;
	saImmOm_attributes2 = new SaImmAttrValuesT_2*[5];
	saImmOm_attributes2[0] = new SaImmAttrValuesT_2;
	saImmOm_attributes2[1] = new SaImmAttrValuesT_2;
	saImmOm_attributes2[2] = new SaImmAttrValuesT_2;
	saImmOm_attributes2[3] = NULL;
	saImmOm_attributes2[0]->attrName = "majorType";
	saImmOm_attributes2[0]->attrValueType = SA_IMM_ATTR_SAUINT32T;
	saImmOm_attributes2[0]->attrValues = (void**) &aUint32a;
	saImmOm_attributes2[0]->attrValuesNumber = 1;
	printf("saImmOm_attributes2[0]->attrValues %p\n", saImmOm_attributes2[0]->attrValues);
	saImmOm_attributes2[1]->attrName = "minorType";
	saImmOm_attributes2[1]->attrValueType = SA_IMM_ATTR_SAUINT32T;
	saImmOm_attributes2[1]->attrValues = (void**) &aUint32b;
	saImmOm_attributes2[1]->attrValuesNumber = 1;
	SaStringT addText = "Default additionalText";
	SaStringT* strAddText = &addText;
	saImmOm_attributes2[2]->attrName = "additionalText";
	saImmOm_attributes2[2]->attrValueType = SA_IMM_ATTR_SASTRINGT;
	saImmOm_attributes2[2]->attrValues = (void**) &strAddText;
	saImmOm_attributes2[2]->attrValuesNumber = 1;

	saImmOm_objectName3 = makeSaNameT("id=1,measurementReaderId=MRt3,pmJobId=ThresJob1,CmwPmpmId=1");
	SaNameT* measurementTypeId = makeSaNameT("measurementTypeId=CcMT-1,pmGroupId=CcGroup1,CmwPmpmId=1");
	saImmOm_attributes3 = new SaImmAttrValuesT_2*[2];
	saImmOm_attributes3[0] = new SaImmAttrValuesT_2;
	saImmOm_attributes3[1] = NULL;
	saImmOm_attributes3[0]->attrName = "measurementTypeRef";
	saImmOm_attributes3[0]->attrValueType = SA_IMM_ATTR_SANAMET;
	saImmOm_attributes3[0]->attrValues = (void**) &measurementTypeId;
	saImmOm_attributes3[0]->attrValuesNumber = 1;

	SaInt32T incr = 1;  //thresholdDirection=INCREASING
	SaInt32T *incrPtr = &incr;

	SaInt32T decr = 2;  //thresholdDirection=DECREASING
	SaInt32T *decrPtr = &decr;

	//Measurement Reader, ThresholdDirection
	saImmOm_attributes4 = new SaImmAttrValuesT_2*[2];
	saImmOm_attributes4[0] = new SaImmAttrValuesT_2;
	saImmOm_attributes4[0]->attrName = "thresholdDirection";
	saImmOm_attributes4[0]->attrValueType = SA_IMM_ATTR_SAINT32T;
	saImmOm_attributes4[0]->attrValues = (void**) &incrPtr;
	saImmOm_attributes4[0]->attrValuesNumber = 1;
	saImmOm_attributes4[1] = NULL;

	MafReturnT com_ret = convertPmThresholdHeader4(&ntf, comNot);

	EXPECT_EQ(com_ret, MafOk);

	EXPECT_EQ( 193, comNot->majorType);
	EXPECT_EQ( 321, comNot->minorType);
	std::string expAddText = "Observed value: ; Threshold level: ; MeasurementType: JOHNNY; Threshold Direction: INCREASING; MO instance: ";
	EXPECT_EQ( expAddText, std::string(comNot->additionalText));

	//Measurement Type, ThresholdDirection
	saImmOm_attributes4[0]->attrValuesNumber = 0 ; //unset thresholdDirection in Measurement Reader
	saImmOm_attributes5 = new SaImmAttrValuesT_2*[2];
	saImmOm_attributes5[0] = new SaImmAttrValuesT_2;
	saImmOm_attributes5[0]->attrName = "thresholdDirection";
	saImmOm_attributes5[0]->attrValueType = SA_IMM_ATTR_SAINT32T;
	saImmOm_attributes5[0]->attrValues = (void**) &decrPtr;
	saImmOm_attributes5[0]->attrValuesNumber = 1;
	saImmOm_attributes5[1] = NULL;

	iNumCalls = 1;
	com_ret = convertPmThresholdHeader4(&ntf, comNot);

	EXPECT_EQ(com_ret, MafOk);

	EXPECT_EQ( 193, comNot->majorType);
	EXPECT_EQ( 321, comNot->minorType);
	expAddText = "Observed value: ; Threshold level: ; MeasurementType: JOHNNY; Threshold Direction: DECREASING; MO instance: ";
	EXPECT_EQ( expAddText, std::string(comNot->additionalText));

	saNameDelete(measurementTypeId, true);
	free(comNot->dn);
	free(comNot->additionalText);
	delete comNot;
	delete ecimMeasTypeDn;
	delete saImmOm_objectName1;
	delete saImmOm_objectName3;
	delete saImmOm_attributes1[0];
	delete [] saImmOm_attributes1;
	delete saImmOm_attributes2[0];
	delete saImmOm_attributes2[1];
	delete saImmOm_attributes2[2];
	delete [] saImmOm_attributes2;
	delete saImmOm_attributes3[0];
	delete [] saImmOm_attributes3;
	delete saImmOm_attributes4[0];
	delete [] saImmOm_attributes4;
	delete saImmOm_attributes5[0];
	delete [] saImmOm_attributes5;
	saImmOm_attributes1 = NULL;
	saImmOm_attributes2 = NULL;
	saImmOm_attributes3 = NULL;
	delete []filterMaf;
	delete hd->notificationClassId;
	saNameDelete(hd->notificationObject, true);
	saNameDelete(hd->notifyingObject, true);
}
TEST (NtfTest, convertTo3GPPDN1)
{
	SaNameT* name = makeSaNameT("3GPPresult");
	//char *theResult = new char[saNameMaxLen()];
	char *theResult = NULL;
	char expected[] = "result";

	bool bResult = convertTo3GPPDN(name, &theResult);

	EXPECT_EQ(true, bResult);
	EXPECT_EQ(0, strcmp( expected, theResult));
	saNameDelete(name, true);
	free(theResult);
}

TEST (NtfTest, setValueInCmEvent1)
{
	bool bResult = true;
	MafMoAttributeValue_3T attribValue;
	MafOamSpiMoAttributeType_3T type;
	SaNtfValueT value;
	SaNtfValueTypeT ntfType;

	value.int16Val = 1;
	bResult = setValueInCmEvent(&attribValue, MafOamSpiMoAttributeType_3_INT16, (void *) &value, ntfType);
	EXPECT_EQ(bResult, true);
	EXPECT_EQ(attribValue.value.i16, 1);

	value.int32Val = 2;
	bResult = setValueInCmEvent(&attribValue, MafOamSpiMoAttributeType_3_INT32, (void *) &value, ntfType);
	EXPECT_EQ(bResult, true);
	EXPECT_EQ(attribValue.value.i32, 2);

	value.int64Val = 3;
	bResult = setValueInCmEvent(&attribValue, MafOamSpiMoAttributeType_3_INT64, (void *) &value, ntfType);
	EXPECT_EQ(bResult, true);
	EXPECT_EQ(attribValue.value.i64, 3);

	value.uint8Val = 4;
	bResult = setValueInCmEvent(&attribValue, MafOamSpiMoAttributeType_3_UINT8, (void *) &value, ntfType);
	EXPECT_EQ(bResult, true);
	EXPECT_EQ(attribValue.value.u8, 4);

	value.uint16Val = 5;
	bResult = setValueInCmEvent(&attribValue, MafOamSpiMoAttributeType_3_UINT16, (void *) &value, ntfType);
	EXPECT_EQ(bResult, true);
	EXPECT_EQ(attribValue.value.u16, 5);

	value.uint32Val = 6;
	bResult = setValueInCmEvent(&attribValue, MafOamSpiMoAttributeType_3_UINT32, (void *) &value, ntfType);
	EXPECT_EQ(bResult, true);
	EXPECT_EQ(attribValue.value.u32, 6);

	value.uint64Val = 7;
	bResult = setValueInCmEvent(&attribValue, MafOamSpiMoAttributeType_3_UINT64, (void *) &value, ntfType);
	EXPECT_EQ(bResult, true);
	EXPECT_EQ(attribValue.value.u64, 7);

	bResult = setValueInCmEvent(&attribValue, MafOamSpiMoAttributeType_3_STRING, NULL, ntfType);
	EXPECT_EQ(bResult, true);
	free((void *)attribValue.value.theString);

	char *string="test string";
	bResult = setValueInCmEvent(&attribValue, MafOamSpiMoAttributeType_3_STRING, (void *) string, ntfType);
	EXPECT_EQ(bResult, true);
	EXPECT_EQ(0, strcmp( attribValue.value.theString, "test string"));
	free((void *)attribValue.value.theString);

	bResult = setValueInCmEvent(&attribValue, MafOamSpiMoAttributeType_3_REFERENCE, NULL, ntfType);
	EXPECT_EQ(bResult, true);
	free((void *)attribValue.value.moRef);

	SaNameT *name = (SaNameT *) calloc(sizeof(SaNameT), 1);
	saNameSet("reference", name);
	bResult = setValueInCmEvent(&attribValue, MafOamSpiMoAttributeType_3_REFERENCE, (void *) name, ntfType);
	EXPECT_EQ(bResult, true);
	EXPECT_EQ(0, strcmp( attribValue.value.moRef, "JOHNNY"));
	free((void *)attribValue.value.moRef);

	value.uint64Val = 8;
	bResult = setValueInCmEvent(&attribValue, MafOamSpiMoAttributeType_3_DECIMAL64, (void *) &value, SA_NTF_VALUE_IPADDRESS);
	EXPECT_EQ(bResult, false);

}

TEST (NtfTest, comsaNotify_positiveTest1)
{
	MafReturnT res;
	MafOamSpiEventConsumerHandleT handle = 1;

	char *eventType = "MafOamSpiNotificationFmEventComponent_4";

	MafNameValuePairT VALUES;
	memset(&VALUES, 0, sizeof(MafNameValuePairT));
	MafNameValuePairT *filter=&VALUES;

	MafOamSpiNotificationFmStruct_4T VALUES4;
	memset(&VALUES4, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	VALUES4.additionalText = "Alarm_Raised_by_TestComponent1120";
	VALUES4.dn = "objImpTestClassId=1";
	VALUES4.eventTime = 133856000;
	VALUES4.majorType = 18568;
	VALUES4.minorType = 14;
	VALUES4.severity = MafOamSpiNotificationFmSeverityCleared;
	VALUES4.additionalInfo.size = 0;

	res = comsaNotify(handle, eventType, &filter, (void *)&VALUES4);

	int outSeverity = int(*Globalnotification->perceivedSeverity);
	int inSeverity = (int) VALUES4.severity;


	char * inAddText = VALUES4.additionalText;
	char * outAddText = (char *) Globalnotification->notificationHeader.additionalText;
	int compResAddTxt = strcmp(inAddText, outAddText);


	long int outVendId = (long int)Globalnotification->notificationHeader.notificationClassId->vendorId;
	long int inVendId = (long int) VALUES4.majorType;


	int outMajorId = (int)Globalnotification->notificationHeader.notificationClassId->majorId;
	int inMajorId = VALUES4.minorType/65536;


	int outMinorId = (int)Globalnotification->notificationHeader.notificationClassId->minorId;
	int inMinorId = VALUES4.minorType%65536;


	unsigned long long outEventTime = *((unsigned long long *)Globalnotification->notificationHeader.eventTime);
	unsigned long long inEventTime = (unsigned long long )VALUES4.eventTime;


	EXPECT_EQ (inSeverity, outSeverity);
	EXPECT_EQ ( res, 0 );
	EXPECT_EQ (compResAddTxt, 0);
	EXPECT_EQ (inVendId, outVendId);
	EXPECT_EQ (outMajorId, inMajorId);
	EXPECT_EQ (outMinorId, inMinorId);
	EXPECT_EQ (outEventTime, inEventTime);
	Globalnotification.reset();
}


TEST (NtfTest, comsaNotify_positiveTest2)
{
	// In this TC we use other eventTime, majorType, minorType and severity
	MafReturnT res;
	MafOamSpiEventConsumerHandleT handle = 1;

	char *eventType = "MafOamSpiNotificationFmEventComponent_4";

	MafNameValuePairT VALUES;
	memset(&VALUES, 0, sizeof(MafNameValuePairT));
	MafNameValuePairT *filter=&VALUES;

	MafOamSpiNotificationFmStruct_4T VALUES4;
	memset(&VALUES4, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	VALUES4.additionalText = "Alarm_Raised_by_TestComponent1120 String for test case 2";
	VALUES4.dn = "ManagedElement=1,objImpTestClassId=1";
	VALUES4.eventTime = 13334345;
	VALUES4.majorType = 193;
	VALUES4.minorType = 14232121;
	VALUES4.severity = MafOamSpiNotificationFmSeverityWarning;

	res = comsaNotify(handle, eventType, &filter, (void *)&VALUES4);

	int outSeverity = int(*Globalnotification->perceivedSeverity);
	int inSeverity = (int) VALUES4.severity;


	char * inAddText = VALUES4.additionalText;
	char * outAddText = (char *) Globalnotification->notificationHeader.additionalText;
	int compResAddTxt = strcmp(inAddText, outAddText);


	long int outVendId = (long int)Globalnotification->notificationHeader.notificationClassId->vendorId;
	long int inVendId = (long int) VALUES4.majorType;


	int outMajorId = (int)Globalnotification->notificationHeader.notificationClassId->majorId;
	int inMajorId = VALUES4.minorType/65536;


	int outMinorId = (int)Globalnotification->notificationHeader.notificationClassId->minorId;
	int inMinorId = VALUES4.minorType%65536;


	unsigned long long outEventTime = *((unsigned long long *)Globalnotification->notificationHeader.eventTime);
	unsigned long long inEventTime = (unsigned long long )VALUES4.eventTime;


	EXPECT_EQ (inSeverity, outSeverity);
	EXPECT_EQ ( res, 0 );
	EXPECT_EQ (compResAddTxt, 0);
	EXPECT_EQ (inVendId, outVendId);
	EXPECT_EQ (outMajorId, inMajorId);
	EXPECT_EQ (outMinorId, inMinorId);
	EXPECT_EQ (outEventTime, inEventTime);
	Globalnotification.reset();
}

TEST (NtfTest, comsaNotify_positiveTest3)
{
	//boundary value testing for length of additional text
	MafReturnT res;
	MafOamSpiEventConsumerHandleT handle = 1;

	char *eventType = "MafOamSpiNotificationFmEventComponent_4";

	MafNameValuePairT VALUES;
	memset(&VALUES, 0, sizeof(MafNameValuePairT));
	MafNameValuePairT *filter=&VALUES;

	MafOamSpiNotificationFmStruct_4T VALUES4;
	memset(&VALUES4, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	VALUES4.additionalText = "this an exactly 256 char long textthis an exactly 256 char long textthis an exactly 256 char long textthis an exactly 256 char long textthis an exactly 256 char long textthis an exactly 256 char long textthis an exactly 256 char long textthis an exactly 25";
	VALUES4.dn = "objImpTestClassId=1";
	VALUES4.eventTime = 98234345;
	VALUES4.majorType = 32993;
	VALUES4.minorType = 45223221;
	VALUES4.severity = MafOamSpiNotificationFmSeverityMajor;

	res = comsaNotify(handle, eventType, &filter, (void *)&VALUES4);

	int outSeverity = int(*Globalnotification->perceivedSeverity);
	int inSeverity = (int) VALUES4.severity;


	char * inAddText = VALUES4.additionalText;
	char * outAddText = (char *) Globalnotification->notificationHeader.additionalText;
	int compResAddTxt = strcmp(inAddText, outAddText);


	long int outVendId = (long int)Globalnotification->notificationHeader.notificationClassId->vendorId;
	long int inVendId = (long int) VALUES4.majorType;


	int outMajorId = (int)Globalnotification->notificationHeader.notificationClassId->majorId;
	int inMajorId = VALUES4.minorType/65536;


	int outMinorId = (int)Globalnotification->notificationHeader.notificationClassId->minorId;
	int inMinorId = VALUES4.minorType%65536;


	unsigned long long outEventTime = *((unsigned long long *)Globalnotification->notificationHeader.eventTime);
	unsigned long long inEventTime = (unsigned long long )VALUES4.eventTime;


	EXPECT_EQ (inSeverity, outSeverity);
	EXPECT_EQ ( res, 0 );
	EXPECT_EQ (compResAddTxt, 0);
	EXPECT_EQ (inVendId, outVendId);
	EXPECT_EQ (outMajorId, inMajorId);
	EXPECT_EQ (outMinorId, inMinorId);
	EXPECT_EQ (outEventTime, inEventTime);
	Globalnotification.reset();
}

TEST (NtfTest, comsaNotify_positiveTest4)
{
	//boundary value testing for additional text with length 0
	MafReturnT res;
	MafOamSpiEventConsumerHandleT handle = 1;

	char *eventType = "MafOamSpiNotificationFmEventComponent_4";

	MafNameValuePairT VALUES;
	memset(&VALUES, 0, sizeof(MafNameValuePairT));
	MafNameValuePairT *filter=&VALUES;

	MafOamSpiNotificationFmStruct_4T VALUES4;
	memset(&VALUES4, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	VALUES4.additionalText = "";
	VALUES4.dn = "objImpTestClassId=1";
	VALUES4.eventTime = 1347485960;
	VALUES4.majorType = 123293;
	VALUES4.minorType = 144858692;
	VALUES4.severity = MafOamSpiNotificationFmSeverityCritical;

	res = comsaNotify(handle, eventType, &filter, (void *)&VALUES4);

	int outSeverity = int(*Globalnotification->perceivedSeverity);
	int inSeverity = (int) VALUES4.severity;


	char * inAddText = VALUES4.additionalText;
	char * outAddText = (char *) Globalnotification->notificationHeader.additionalText;
	int compResAddTxt = strcmp(inAddText, outAddText);


	long int outVendId = (long int)Globalnotification->notificationHeader.notificationClassId->vendorId;
	long int inVendId = (long int) VALUES4.majorType;


	int outMajorId = (int)Globalnotification->notificationHeader.notificationClassId->majorId;
	int inMajorId = VALUES4.minorType/65536;


	int outMinorId = (int)Globalnotification->notificationHeader.notificationClassId->minorId;
	int inMinorId = VALUES4.minorType%65536;


	unsigned long long outEventTime = *((unsigned long long *)Globalnotification->notificationHeader.eventTime);
	unsigned long long inEventTime = (unsigned long long )VALUES4.eventTime;


	EXPECT_EQ (inSeverity, outSeverity);
	EXPECT_EQ ( res, 0 );
	EXPECT_EQ (compResAddTxt, 0);
	EXPECT_EQ (inVendId, outVendId);
	EXPECT_EQ (outMajorId, inMajorId);
	EXPECT_EQ (outMinorId, inMinorId);
	EXPECT_EQ (outEventTime, inEventTime);
	Globalnotification.reset();
}



TEST (NtfTest, comsaNotify_positiveTest5)
{
	// in this test case we send in an additional text that is longer than 256 char. However, it seems to be automatically truncated to a length of 256
	MafReturnT res;
	MafOamSpiEventConsumerHandleT handle = 1;

	char *eventType = "MafOamSpiNotificationFmEventComponent_4";

	MafNameValuePairT VALUES;
	memset(&VALUES, 0, sizeof(MafNameValuePairT));
	MafNameValuePairT *filter=&VALUES;

	MafOamSpiNotificationFmStruct_4T VALUES4;
	memset(&VALUES4, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	VALUES4.additionalText = "this is gpoing to be a very long text, longer than 256 char, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long.";
	VALUES4.dn = "objImpTestClassId=1";
	VALUES4.eventTime = 1;
	VALUES4.majorType = 193;
	VALUES4.minorType = 144858692;
	VALUES4.severity = MafOamSpiNotificationFmSeverityMinor;

	res = comsaNotify(handle, eventType, &filter, (void *)&VALUES4);

	int outSeverity = int(*Globalnotification->perceivedSeverity);
	int inSeverity = (int) VALUES4.severity;


	char * inAddText = VALUES4.additionalText;
	char * outAddText = (char *) Globalnotification->notificationHeader.additionalText;
	int compResAddTxt = strcmp(inAddText, outAddText);


	long int outVendId = (long int)Globalnotification->notificationHeader.notificationClassId->vendorId;
	long int inVendId = (long int) VALUES4.majorType;


	int outMajorId = (int)Globalnotification->notificationHeader.notificationClassId->majorId;
	int inMajorId = VALUES4.minorType/65536;


	int outMinorId = (int)Globalnotification->notificationHeader.notificationClassId->minorId;
	int inMinorId = VALUES4.minorType%65536;


	unsigned long long outEventTime = *((unsigned long long *)Globalnotification->notificationHeader.eventTime);
	unsigned long long inEventTime = (unsigned long long )VALUES4.eventTime;


	EXPECT_EQ (inSeverity, outSeverity);
	EXPECT_EQ ( res, 0 );
	EXPECT_EQ (compResAddTxt, 0);
	EXPECT_EQ (inVendId, outVendId);
	EXPECT_EQ (outMajorId, inMajorId);
	EXPECT_EQ (outMinorId, inMinorId);
	EXPECT_EQ (outEventTime, inEventTime);
	Globalnotification.reset();
}

TEST (NtfTest, comsaNotify4_positiveTest1)
{
	// add additionalInfo with 2 pair of (name, value)
	MafReturnT res = MafOk;
	MafOamSpiEventConsumerHandleT handle = 1;

	char *eventType = "MafOamSpiNotificationFmEventComponent_4";

	MafNameValuePairT VALUES = {0};
	MafNameValuePairT *filter=&VALUES;

	MafOamSpiNotificationFmStruct_4T VALUES4 = {0};
	VALUES4.additionalText = "Alarm_Raised_by_TestComponent1120";
	VALUES4.dn = "objImpTestClassId=1";
	VALUES4.eventTime = 133856000;
	VALUES4.majorType = 18568;
	VALUES4.minorType = 14;
	VALUES4.severity = MafOamSpiNotificationFmSeverityCleared;
	VALUES4.additionalInfo.size = 2;
	VALUES4.additionalInfo.additionalInfoArr = new MafOamSpiNotificationFmAdditionalInfoT[2];
	VALUES4.additionalInfo.additionalInfoArr[0].name = "age";
	VALUES4.additionalInfo.additionalInfoArr[0].value = "1.6";
	VALUES4.additionalInfo.additionalInfoArr[1].name = "name";
	VALUES4.additionalInfo.additionalInfoArr[1].value = "Job";

	res = comsaNotify4(handle, eventType, &filter, (void *)&VALUES4);

	int outSeverity = int(*Globalnotification->perceivedSeverity);
	int inSeverity = VALUES4.severity;


	char * inAddText = VALUES4.additionalText;
	char * outAddText = Globalnotification->notificationHeader.additionalText;
	int compResAddTxt = strcmp(inAddText, outAddText);


	long int outVendId = Globalnotification->notificationHeader.notificationClassId->vendorId;
	long int inVendId = VALUES4.majorType;


	int outMajorId = Globalnotification->notificationHeader.notificationClassId->majorId;
	int inMajorId = VALUES4.minorType/65536;


	int outMinorId = Globalnotification->notificationHeader.notificationClassId->minorId;
	int inMinorId = VALUES4.minorType%65536;

	unsigned long long outEventTime = *Globalnotification->notificationHeader.eventTime;
	unsigned long long inEventTime = VALUES4.eventTime;

	char * outvalue1;
	saNtfPtrValGet(20,&Globalnotification->notificationHeader.additionalInfo->infoValue,(void **)&outvalue1,NULL);
	char * invalue1  = VALUES4.additionalInfo.additionalInfoArr[0].value;
	int compValue1= strcmp(outvalue1, invalue1);

	char * outvalue2;
	saNtfPtrValGet(20,&Globalnotification->notificationHeader.additionalInfo->infoValue,(void **)&outvalue2,NULL);
	char * invalue2  = VALUES4.additionalInfo.additionalInfoArr[0].value;
	int compValue2= strcmp(outvalue2, invalue2);

	delete [] VALUES4.additionalInfo.additionalInfoArr;
	EXPECT_EQ (inSeverity, outSeverity);
	EXPECT_EQ ( res, 0 );
	EXPECT_EQ (compResAddTxt, 0);
	EXPECT_EQ (inVendId, outVendId);
	EXPECT_EQ (outMajorId, inMajorId);
	EXPECT_EQ (outMinorId, inMinorId);
	EXPECT_EQ (outEventTime, inEventTime);
	EXPECT_EQ (compValue1, 0);
	EXPECT_EQ (compValue2, 0);
	freeAllAdditionInfo();
	Globalnotification.reset();
}

TEST (NtfTest, comsaNotify4_positiveTest2)
{
	// added code to test additionalInfo with length 0
	//boundary value testing for additional text with length 0
	MafReturnT res = MafOk;
	MafOamSpiEventConsumerHandleT handle = 1;

	char *eventType = "MafOamSpiNotificationFmEventComponent_4";

	MafNameValuePairT VALUES;
	memset(&VALUES, 0, sizeof(MafNameValuePairT));
	MafNameValuePairT *filter=&VALUES;

	MafOamSpiNotificationFmStruct_4T VALUES4;
	memset(&VALUES4, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	VALUES4.additionalText = "";
	VALUES4.dn = "objImpTestClassId=1";
	VALUES4.eventTime = 1347485960;
	VALUES4.majorType = 123293;
	VALUES4.minorType = 144858692;
	VALUES4.severity = MafOamSpiNotificationFmSeverityCritical;
	VALUES4.additionalInfo.size = 2;
	VALUES4.additionalInfo.additionalInfoArr = new MafOamSpiNotificationFmAdditionalInfoT[2];
	VALUES4.additionalInfo.additionalInfoArr[0].name = "";
	VALUES4.additionalInfo.additionalInfoArr[0].value = "";
	VALUES4.additionalInfo.additionalInfoArr[1].name = "";
	VALUES4.additionalInfo.additionalInfoArr[1].value = "";

	res = comsaNotify4(handle, eventType, &filter, (void *)&VALUES4);

	int outSeverity = int(*Globalnotification->perceivedSeverity);
	int inSeverity = VALUES4.severity;


	char * inAddText = VALUES4.additionalText;
	char * outAddText = Globalnotification->notificationHeader.additionalText;
	int compResAddTxt = strcmp(inAddText, outAddText);


	long int outVendId = Globalnotification->notificationHeader.notificationClassId->vendorId;
	long int inVendId = VALUES4.majorType;


	int outMajorId = Globalnotification->notificationHeader.notificationClassId->majorId;
	int inMajorId = VALUES4.minorType/65536;


	int outMinorId = Globalnotification->notificationHeader.notificationClassId->minorId;
	int inMinorId = VALUES4.minorType%65536;


	unsigned long long outEventTime = *Globalnotification->notificationHeader.eventTime;
	unsigned long long inEventTime = VALUES4.eventTime;

	char * outvalue1;
	saNtfPtrValGet(20,&Globalnotification->notificationHeader.additionalInfo->infoValue,(void **)&outvalue1,NULL);
	char * invalue1  = VALUES4.additionalInfo.additionalInfoArr[0].value;
	int compValue1= strcmp(outvalue1, invalue1);

	char * outvalue2;
	saNtfPtrValGet(20,&Globalnotification->notificationHeader.additionalInfo->infoValue,(void **)&outvalue2,NULL);
	char * invalue2  = VALUES4.additionalInfo.additionalInfoArr[0].value;
	int compValue2= strcmp(outvalue2, invalue2);

	delete [] VALUES4.additionalInfo.additionalInfoArr;
	EXPECT_EQ (inSeverity, outSeverity);
	EXPECT_EQ ( res, 0 );
	EXPECT_EQ (compResAddTxt, 0);
	EXPECT_EQ (inVendId, outVendId);
	EXPECT_EQ (outMajorId, inMajorId);
	EXPECT_EQ (outMinorId, inMinorId);
	EXPECT_EQ (outEventTime, inEventTime);
	EXPECT_EQ (compValue1, 0);
	EXPECT_EQ (compValue2, 0);
	freeAllAdditionInfo();
	Globalnotification.reset();
}

TEST (NtfTest, comsaNotify4_positiveTest3)
{
	// added code to test additionalInfo with length >256
	// in this test case we send in an additional text that is longer than 256 char. However, it seems to be automatically truncated to a length of 256
	MafReturnT res = MafOk;
	MafOamSpiEventConsumerHandleT handle = 1;

	char *eventType = "MafOamSpiNotificationFmEventComponent_4";

	MafNameValuePairT VALUES;
	memset(&VALUES, 0, sizeof(MafNameValuePairT));
	MafNameValuePairT *filter=&VALUES;

	MafOamSpiNotificationFmStruct_4T VALUES4;
	memset(&VALUES4, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	VALUES4.additionalText = "this is gpoing to be a very long text, longer than 256 char, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long.";
	VALUES4.dn = "objImpTestClassId=1";
	VALUES4.eventTime = 1;
	VALUES4.majorType = 193;
	VALUES4.minorType = 144858692;
	VALUES4.severity = MafOamSpiNotificationFmSeverityMinor;
	VALUES4.additionalInfo.size = 2;
	VALUES4.additionalInfo.additionalInfoArr = new MafOamSpiNotificationFmAdditionalInfoT[2];
	VALUES4.additionalInfo.additionalInfoArr[0].name = "this is gpoing to be a very long text, longer than 256 char, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long.";
	VALUES4.additionalInfo.additionalInfoArr[0].value = "this is gpoing to be a very long text, longer than 256 char, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long.";
	VALUES4.additionalInfo.additionalInfoArr[1].name = "this is gpoing to be a very long text, longer than 256 char, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long.";
	VALUES4.additionalInfo.additionalInfoArr[1].value = "this is gpoing to be a very long text, longer than 256 char, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long. this is gpoing to be a very long text, just too long.";

	res = comsaNotify4(handle, eventType, &filter, (void *)&VALUES4);

	int outSeverity = int(*Globalnotification->perceivedSeverity);
	int inSeverity = VALUES4.severity;


	char * inAddText = VALUES4.additionalText;
	char * outAddText = Globalnotification->notificationHeader.additionalText;
	int compResAddTxt = strcmp(inAddText, outAddText);


	long int outVendId = Globalnotification->notificationHeader.notificationClassId->vendorId;
	long int inVendId = VALUES4.majorType;


	int outMajorId = Globalnotification->notificationHeader.notificationClassId->majorId;
	int inMajorId = VALUES4.minorType/65536;


	int outMinorId = Globalnotification->notificationHeader.notificationClassId->minorId;
	int inMinorId = VALUES4.minorType%65536;


	unsigned long long outEventTime = *Globalnotification->notificationHeader.eventTime;
	unsigned long long inEventTime = VALUES4.eventTime;


	char * outvalue1;
	saNtfPtrValGet(20,&Globalnotification->notificationHeader.additionalInfo->infoValue,(void **)&outvalue1,NULL);
	char * invalue1  = VALUES4.additionalInfo.additionalInfoArr[0].value;
	int compValue1= strcmp(outvalue1, invalue1);

	char * outvalue2;
	saNtfPtrValGet(20,&Globalnotification->notificationHeader.additionalInfo->infoValue,(void **)&outvalue2,NULL);
	char * invalue2  = VALUES4.additionalInfo.additionalInfoArr[0].value;
	int compValue2= strcmp(outvalue2, invalue2);

	delete [] VALUES4.additionalInfo.additionalInfoArr;

	EXPECT_EQ (inSeverity, outSeverity);
	EXPECT_EQ ( res, 0 );
	EXPECT_EQ (compResAddTxt, 0);
	EXPECT_EQ (inVendId, outVendId);
	EXPECT_EQ (outMajorId, inMajorId);
	EXPECT_EQ (outMinorId, inMinorId);
	EXPECT_EQ (outEventTime, inEventTime);
	EXPECT_EQ (compValue1, 0);
	EXPECT_EQ (compValue2, 0);
	freeAllAdditionInfo();
	Globalnotification.reset();
}

TEST (NtfTest, comsaNotify_negativeTest2)
{
	// In this test case we send in an invalid event type and expect comsaNotify to not return MafOk
	MafReturnT res;
	MafOamSpiEventConsumerHandleT handle = 1;

	char *eventType = "NotmafOamSpiNotificationFmEventComponent_3";

	MafNameValuePairT VALUES;
	memset(&VALUES, 0, sizeof(MafNameValuePairT));
	MafNameValuePairT *filter=&VALUES;

	MafOamSpiNotificationFmStruct_4T VALUES4;
	memset(&VALUES4, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	VALUES4.additionalText = "This is an additional text here";
	VALUES4.dn = "objImpTestClassId=1";
	VALUES4.eventTime = 0;
	VALUES4.majorType = 193;
	VALUES4.minorType = 144858692;
	VALUES4.severity = MafOamSpiNotificationFmSeverityMinor;
	VALUES4.additionalInfo.size = 0;

	res = comsaNotify(handle, eventType, &filter, (void *)&VALUES4);

	EXPECT_NE ( res, 0 );
}

TEST (NtfTest, comsaNotify_negativeTest3)
{
	// In this test case we send in an invalid filter (NULL) and expect comsaNotify to not return MafOk
	MafReturnT res;
	MafOamSpiEventConsumerHandleT handle = 1;

	char *eventType = "MafOamSpiNotificationFmEventComponent_4";

	MafNameValuePairT VALUES;
	memset(&VALUES, 0, sizeof(MafNameValuePairT));
	MafNameValuePairT **filter=NULL;

	MafOamSpiNotificationFmStruct_4T VALUES4;
	memset(&VALUES4, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	VALUES4.additionalText = "This is an additional text here";
	VALUES4.dn = "objImpTestClassId=1";
	VALUES4.eventTime = 0;
	VALUES4.majorType = 193;
	VALUES4.minorType = 144858692;
	VALUES4.severity = MafOamSpiNotificationFmSeverityMinor;
	VALUES4.additionalInfo.size = 0;

	res = comsaNotify(handle, eventType, filter, (void *)&VALUES4);

	EXPECT_NE ( res, 0 );
}

TEST (NtfTest, comsaNotify_negativeTest4)
{
	// In this test case we send in an invalid handle and expect comsaNotify to not return MafOk
	MafReturnT res;
	MafOamSpiEventConsumerHandleT handle = 0;

	char *eventType = "MafOamSpiNotificationFmEventComponent_4";

	MafNameValuePairT VALUES;
	memset(&VALUES, 0, sizeof(MafNameValuePairT));
	MafNameValuePairT *filter=&VALUES;

	MafOamSpiNotificationFmStruct_4T VALUES4;
	memset(&VALUES4, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	VALUES4.additionalText = "This is an additional text here";
	VALUES4.dn = "objImpTestClassId=1";
	VALUES4.eventTime = 0;
	VALUES4.majorType = 193;
	VALUES4.minorType = 144858692;
	VALUES4.severity = MafOamSpiNotificationFmSeverityMinor;
	VALUES4.additionalInfo.size = 0;

	res = comsaNotify(handle, eventType, &filter, (void *)&VALUES4);

	EXPECT_NE ( res, 0 );
}

TEST (NtfTest, comsaNotify_negativeTest5)
{
	// In this test case we send in an invalid value (last parameter in call of comsaNotify()) and expect comsaNotify to not return MafOk
	MafReturnT res;
	MafOamSpiEventConsumerHandleT handle = 1;

	char *eventType = "MafOamSpiNotificationFmEventComponent_4";

	MafNameValuePairT VALUES;
	memset(&VALUES, 0, sizeof(MafNameValuePairT));
	MafNameValuePairT *filter=&VALUES;

	res = comsaNotify(handle, eventType, &filter, NULL);

	EXPECT_NE ( res, 0 );
}

TEST (NtfTest, comsaNotify4_negativeTest1)
{
	// In this test case we send in an invalid event type and expect comsaNotify to not return MafOk
	MafReturnT res = MafOk;
	MafOamSpiEventConsumerHandleT handle = 1;

	char *eventType = "NotmafOamSpiNotificationFmEventComponent_4";

	MafNameValuePairT VALUES;
	memset(&VALUES, 0, sizeof(MafNameValuePairT));
	MafNameValuePairT *filter=&VALUES;

	MafOamSpiNotificationFmStruct_4T VALUES4;
	memset(&VALUES4, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	VALUES4.additionalText = "This is an additional text here";
	VALUES4.dn = "objImpTestClassId=1";
	VALUES4.eventTime = 0;
	VALUES4.majorType = 193;
	VALUES4.minorType = 144858692;
	VALUES4.severity = MafOamSpiNotificationFmSeverityMinor;
	VALUES4.additionalInfo.size = 0;

	res = comsaNotify4(handle, eventType, &filter, (void *)&VALUES4);

	EXPECT_NE ( res, 0 );
}

TEST (NtfTest, comsaNotify4_negativeTest2)
{
	// In this test case we send in an invalid filter (NULL) and expect comsaNotify to not return MafOk
	MafReturnT res = MafOk;
	MafOamSpiEventConsumerHandleT handle = 1;

	char *eventType = "MafOamSpiNotificationFmEventComponent_4";

	MafNameValuePairT VALUES;
	memset(&VALUES, 0, sizeof(MafNameValuePairT));
	MafNameValuePairT **filter=NULL;

	MafOamSpiNotificationFmStruct_4T VALUES4;
	memset(&VALUES4, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	VALUES4.additionalText = "This is an additional text here";
	VALUES4.dn = "objImpTestClassId=1";
	VALUES4.eventTime = 0;
	VALUES4.majorType = 193;
	VALUES4.minorType = 144858692;
	VALUES4.severity = MafOamSpiNotificationFmSeverityMinor;
	VALUES4.additionalInfo.size = 0;

	res = comsaNotify4(handle, eventType, filter, (void *)&VALUES4);

	EXPECT_NE ( res, 0 );
}

TEST (NtfTest, comsaNotify4_negativeTest3)
{
	// In this test case we send in an invalid handle and expect comsaNotify to not return MafOk
	MafReturnT res = MafOk;
	MafOamSpiEventConsumerHandleT handle = 0;

	char *eventType = "MafOamSpiNotificationFmEventComponent_4";

	MafNameValuePairT VALUES;
	memset(&VALUES, 0, sizeof(MafNameValuePairT));
	MafNameValuePairT *filter=&VALUES;

	MafOamSpiNotificationFmStruct_4T VALUES4;
	memset(&VALUES4, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	VALUES4.additionalText = "This is an additional text here";
	VALUES4.dn = "objImpTestClassId=1";
	VALUES4.eventTime = 0;
	VALUES4.majorType = 193;
	VALUES4.minorType = 144858692;
	VALUES4.severity = MafOamSpiNotificationFmSeverityMinor;
	VALUES4.additionalInfo.size = 0;

	res = comsaNotify4(handle, eventType, &filter, (void *)&VALUES4);

	EXPECT_NE ( res, 0 );
}

TEST (NtfTest, comsaNotify_negativeTest9)
{
	// In this test case we send in an invalid value (last parameter in call of comsaNotify()) and expect comsaNotify to not return MafOk
	MafReturnT res = MafOk;
	MafOamSpiEventConsumerHandleT handle = 1;

	char *eventType = "MafOamSpiNotificationFmEventComponent_4";

	MafNameValuePairT VALUES;
	memset(&VALUES, 0, sizeof(MafNameValuePairT));
	MafNameValuePairT *filter=&VALUES;

	res = comsaNotify(handle, eventType, &filter, NULL);

	EXPECT_NE ( res, 0 );
}

TEST (NtfTest, convertHeader4a)
{
	// In this test case we send in an correct convertHeader to not return MafOk
	// test for string
	MafReturnT maf_rc = MafOk;
	MafOamSpiNotificationFmStruct_4T *comNot = new MafOamSpiNotificationFmStruct_4T;
	memset(comNot, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	SaNtfValueT infoValue;
	SaNtfAdditionalInfoT additionalInfo[2];

	SaNtfNotificationsT ntf;
	memset(&ntf, 0, sizeof(SaNtfNotificationsT));
	ntf.notificationType = SA_NTF_TYPE_ALARM;
	SaNtfNotificationHeaderT *hd = &ntf.notification.alarmNotification.notificationHeader;

	SaInt64T  tmplli = 1346475867467378548;
	hd->eventTime = &tmplli;
	hd->notificationObject = new SaNameInitT;
	hd->additionalText = "example test";
	hd->notificationClassId = new SaNtfClassIdT;
	hd->notificationClassId->vendorId = 10;
	hd->notificationClassId->majorId = 11;
	hd->notificationClassId->minorId = 12;
	hd->numAdditionalInfo = 2;
	hd->additionalInfo = &additionalInfo[0];
	void * destPtr = NULL;
	char * tmpStr1 = "test string 1";
	char * tmpStr2 = "test string 22";
	infoValue.int8Val = 1;
	setSaNtfAdditionalInfoT(0, SA_NTF_VALUE_STRING, &(additionalInfo[0]),  infoValue);
	infoValue.int8Val = 2;
	setSaNtfAdditionalInfoT(1, SA_NTF_VALUE_STRING, &(additionalInfo[1]), infoValue);
	myTestDataAttributesNames[0] = tmpStr1;
	myTestDataAttributesNames[1] = tmpStr2;

	SaAisErrorT ret = saNtfPtrValAllocate(
											20,
											strlen(tmpStr1) + 1,
											(void**) &destPtr,
											&(hd->additionalInfo[0].infoValue));
	EXPECT_EQ ( ret, SA_AIS_OK );
	((char *)destPtr)[strlen(tmpStr1)] = '\0';
	strcpy((char *)destPtr,tmpStr1);

	hd->additionalInfo[1].infoType = SA_NTF_VALUE_STRING;
	ret = saNtfPtrValAllocate(
								20,
								strlen(tmpStr2) + 1,
								(void**) &destPtr,
								&(hd->additionalInfo[1].infoValue));
	EXPECT_EQ ( SA_AIS_OK, ret);
	((char *)destPtr)[strlen(tmpStr2)] = '\0';
	strcpy((char *)destPtr,tmpStr2);

	char value[] = "1234567";
	MafNameValuePairT *filter = new MafNameValuePairT[1];
	nFilter_V4.filter = &filter;
	nFilter_V4.filter[0]->name = NULL;
	nFilter_V4.filter[0]->value = (const char *) &value;

	maf_rc = convertHeader4(&ntf, comNot);
	EXPECT_EQ ( MafOk, maf_rc );

	//Checking the result
	EXPECT_EQ(1346475867467378548, comNot->eventTime);
	EXPECT_EQ(0, strcmp(hd->additionalText, comNot->additionalText));
	EXPECT_EQ(hd->notificationClassId->vendorId, comNot->majorType);
	EXPECT_EQ((((uint32_t)hd->notificationClassId->majorId << 16)|(uint32_t)hd->notificationClassId->minorId), comNot->minorType);
	EXPECT_EQ(2, comNot->additionalInfo.size);
	EXPECT_EQ(0, strcmp( tmpStr1, comNot->additionalInfo.additionalInfoArr[0].value));
	EXPECT_EQ(0, strcmp( tmpStr2, comNot->additionalInfo.additionalInfoArr[1].value));

	saNameDelete(hd->notificationObject, true);
	delete hd->notificationClassId;
	freeAllAdditionInfo();
	free(comNot->dn);
	free(comNot->additionalText);
	for (int i = 0; i < comNot->additionalInfo.size; i++)
	{
		free(comNot->additionalInfo.additionalInfoArr[i].name);
		free(comNot->additionalInfo.additionalInfoArr[i].value);
	}
	free(comNot->additionalInfo.additionalInfoArr);
	delete comNot;
	delete []filter;
}

TEST (NtfTest, convertHeader4b)
{
	// In this test case we send in an correct convertHeader to not return MafOk
	// test for string
	MafReturnT maf_rc = MafOk;
	MafOamSpiNotificationFmStruct_4T *comNot = new MafOamSpiNotificationFmStruct_4T;
	memset(comNot, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	SaNtfNotificationHandleT notificationHandle;
	SaNtfValueT infoValue;
	SaNtfAdditionalInfoT additionalInfo[2];

	SaNtfNotificationsT ntf;
	memset(&ntf, 0, sizeof(SaNtfNotificationsT));
	ntf.notificationType = SA_NTF_TYPE_ALARM;
	SaNtfNotificationHeaderT *hd = &ntf.notification.alarmNotification.notificationHeader;
	//SaNtfNotificationHeaderT * hd = new SaNtfNotificationHeaderT;
	SaInt64T  tmplli = 1346475867467378548;
	hd->eventTime = &tmplli;
	hd->notificationObject = new SaNameInitT;
	hd->additionalText = "example test";
	hd->notificationClassId = new SaNtfClassIdT;
	hd->notificationClassId->vendorId = 10;
	hd->notificationClassId->majorId = 20;
	hd->notificationClassId->minorId = 1;
	hd->numAdditionalInfo = 2;
	hd->additionalInfo = &additionalInfo[0];
	void * destPtr = NULL;
	char * tmpStr1 = "test string 1";
	char * tmpStr2 = "test string 22";
	infoValue.int8Val = 1;
	setSaNtfAdditionalInfoT(0, SA_NTF_VALUE_STRING, &(additionalInfo[0]),  infoValue);
	infoValue.int8Val = 2;
	setSaNtfAdditionalInfoT(1, SA_NTF_VALUE_LDAP_NAME, &(additionalInfo[1]), infoValue);
	myTestDataAttributesNames[0] = tmpStr1;
	myTestDataAttributesNames[1] = tmpStr2;

	SaAisErrorT ret = saNtfPtrValAllocate(
											20,
											strlen(tmpStr1) + 1,
											(void**) &destPtr,
											&(hd->additionalInfo[0].infoValue));
	EXPECT_EQ ( ret, SA_AIS_OK );
	((char *)destPtr)[strlen(tmpStr1)] = '\0';
	strcpy((char *)destPtr,tmpStr1);

	hd->additionalInfo[1].infoType = SA_NTF_VALUE_LDAP_NAME;
	ret = saNtfPtrValAllocate(
								20,
								strlen(tmpStr2) + 1,
								(void**) &destPtr,
								&(hd->additionalInfo[1].infoValue));
	EXPECT_EQ ( SA_AIS_OK, ret);
	((char *)destPtr)[strlen(tmpStr2)] = '\0';
	strcpy((char *)destPtr,tmpStr2);

	char value[] = "1234567";
	MafNameValuePairT *filter = new MafNameValuePairT[1];
	nFilter_V4.filter = &filter;
	nFilter_V4.filter[0]->name = NULL;
	nFilter_V4.filter[0]->value = (const char *) &value;

	maf_rc = convertHeader4(&ntf, comNot);
	EXPECT_EQ ( MafOk, maf_rc );

	//Checking the result
	EXPECT_EQ(1346475867467378548, comNot->eventTime);
	EXPECT_EQ(0, strcmp(hd->additionalText, comNot->additionalText));
	EXPECT_EQ(hd->notificationClassId->vendorId, comNot->majorType);
	EXPECT_EQ((((uint32_t)hd->notificationClassId->majorId << 16)|(uint32_t)hd->notificationClassId->minorId), comNot->minorType);
	EXPECT_EQ(2, comNot->additionalInfo.size);
	EXPECT_EQ(0, strcmp( tmpStr1, comNot->additionalInfo.additionalInfoArr[0].value));
	EXPECT_EQ(0, strcmp( "JOHNNY", comNot->additionalInfo.additionalInfoArr[1].value));

	saNameDelete(hd->notificationObject, true);
	delete hd->notificationClassId;
	freeAllAdditionInfo();
	free(comNot->dn);
	free(comNot->additionalText);
	for (int i = 0; i < comNot->additionalInfo.size; i++)
	{
		free(comNot->additionalInfo.additionalInfoArr[i].name);
		free(comNot->additionalInfo.additionalInfoArr[i].value);
	}
	free(comNot->additionalInfo.additionalInfoArr);
	delete comNot;
	delete []filter;
}

TEST (NtfTest, convertHeader4c)
{
	// In this test case we send Threshold-alarm/notification (PM-services)
	// test for string
	MafReturnT maf_rc = MafOk;
	MafOamSpiNotificationFmStruct_4T *comNot = new MafOamSpiNotificationFmStruct_4T;
	memset(comNot, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	SaNtfNotificationHandleT notificationHandle;
	SaNtfValueT infoValue;
	SaNtfAdditionalInfoT additionalInfo[2];

	SaNtfNotificationsT ntf;
	memset(&ntf, 0, sizeof(SaNtfNotificationsT));
	ntf.notificationType = SA_NTF_TYPE_ALARM;
	SaNtfNotificationHeaderT *hd = &ntf.notification.alarmNotification.notificationHeader;
	SaInt64T  tmplli = 1346475867467378548;
	hd->eventTime = &tmplli;
	hd->notificationObject = new SaNameInitT;
	hd->notifyingObject = makeSaNameT("safThreshold=TM1-Warning,SaPmfJobMT=safMT=CcMT-1\\,safMeasObjClass=CcGroup1\\,safPm=1,safJob=ThresJob1,safPm=1");
	hd->additionalText = "example test";
	hd->notificationClassId = new SaNtfClassIdT;
	hd->notificationClassId->vendorId = SA_NTF_VENDOR_ID_SAF;
	hd->notificationClassId->majorId = 20;
	hd->notificationClassId->minorId = 1;
	hd->numAdditionalInfo = 2;
	hd->additionalInfo = &additionalInfo[0];
	void * destPtr = NULL;
	char * tmpStr1 = "test string 1";
	char * tmpStr2 = "test string 22";
	infoValue.int8Val = 1;
	setSaNtfAdditionalInfoT(0, SA_NTF_VALUE_STRING, &(additionalInfo[0]),  infoValue);
	infoValue.int8Val = 2;
	setSaNtfAdditionalInfoT(1, SA_NTF_VALUE_STRING, &(additionalInfo[1]), infoValue);
	myTestDataAttributesNames[0] = tmpStr1;
	myTestDataAttributesNames[1] = tmpStr2;

	SaAisErrorT ret = saNtfPtrValAllocate(
											20,
											strlen(tmpStr1) + 1,
											(void**) &destPtr,
											&(hd->additionalInfo[0].infoValue));
	EXPECT_EQ ( ret, SA_AIS_OK );
	((char *)destPtr)[strlen(tmpStr1)] = '\0';
	strcpy((char *)destPtr,tmpStr1);

	hd->additionalInfo[1].infoType = SA_NTF_VALUE_STRING;
	ret = saNtfPtrValAllocate(
								20,
								strlen(tmpStr2) + 1,
								(void**) &destPtr,
								&(hd->additionalInfo[1].infoValue));
	EXPECT_EQ ( SA_AIS_OK, ret);
	((char *)destPtr)[strlen(tmpStr2)] = '\0';
	strcpy((char *)destPtr,tmpStr2);

	char value[] = "1234567";
	MafNameValuePairT *filter = new MafNameValuePairT[1];
	nFilter_V4.filter = &filter;
	nFilter_V4.filter[0]->name = NULL;
	nFilter_V4.filter[0]->value = (const char *) &value;

	maf_rc = convertHeader4(&ntf, comNot);
	EXPECT_EQ ( MafOk, maf_rc );

	//Checking the result
	EXPECT_EQ(1346475867467378548, comNot->eventTime);
	EXPECT_EQ(0, strcmp(hd->additionalText, comNot->additionalText));
	EXPECT_EQ(hd->notificationClassId->vendorId, comNot->majorType);
	EXPECT_EQ((((uint32_t)hd->notificationClassId->majorId << 16)|(uint32_t)hd->notificationClassId->minorId), comNot->minorType);
	EXPECT_EQ(2, comNot->additionalInfo.size);
	EXPECT_EQ(0, strcmp( tmpStr1, comNot->additionalInfo.additionalInfoArr[0].value));
	EXPECT_EQ(0, strcmp( tmpStr2, comNot->additionalInfo.additionalInfoArr[1].value));

	saNameDelete(hd->notificationObject, true);
	saNameDelete(hd->notifyingObject, true);
	delete hd->notificationClassId;
	freeAllAdditionInfo();
	free(comNot->dn);
	free(comNot->additionalText);
	for (int i = 0; i < comNot->additionalInfo.size; i++)
	{
		free(comNot->additionalInfo.additionalInfoArr[i].name);
		free(comNot->additionalInfo.additionalInfoArr[i].value);
	}
	free(comNot->additionalInfo.additionalInfoArr);
	delete comNot;
	delete []filter;
}

TEST (NtfTest, convertHeader4_additionalText_NULL)
{
	// In this test case we verify whether the "additionalText" attribute value
	// is NULL while sending notification
	MafReturnT maf_rc = MafOk;
	MafOamSpiNotificationFmStruct_4T *comNot = new MafOamSpiNotificationFmStruct_4T;
	memset(comNot, 0, sizeof(MafOamSpiNotificationFmStruct_4T));
	SaNtfNotificationHandleT notificationHandle;
	SaNtfValueT infoValue;
	SaNtfAdditionalInfoT additionalInfo[2];

	SaNtfNotificationsT ntf;
	memset(&ntf, 0, sizeof(SaNtfNotificationsT));
	ntf.notificationType = SA_NTF_TYPE_ALARM;
	SaNtfNotificationHeaderT *hd = &ntf.notification.alarmNotification.notificationHeader;
	SaInt64T  tmplli = 1346475867467378548;
	hd->eventTime = &tmplli;
	hd->notificationObject = new SaNameInitT;
	hd->additionalText = NULL;
	hd->notificationClassId = new SaNtfClassIdT;
	hd->notificationClassId->vendorId = 10;
	hd->notificationClassId->majorId = 11;
	hd->notificationClassId->minorId = 12;
	hd->numAdditionalInfo = 2;
	hd->additionalInfo = &additionalInfo[0];
	void * destPtr = NULL;
	char * tmpStr1 = "test string 1";
	char * tmpStr2 = "test string 22";
	infoValue.int8Val = 1;
	setSaNtfAdditionalInfoT(0, SA_NTF_VALUE_STRING, &(additionalInfo[0]),  infoValue);
	infoValue.int8Val = 2;
	setSaNtfAdditionalInfoT(1, SA_NTF_VALUE_STRING, &(additionalInfo[1]), infoValue);
	myTestDataAttributesNames[0] = tmpStr1;
	myTestDataAttributesNames[1] = tmpStr2;

	SaAisErrorT ret = saNtfPtrValAllocate(
											20,
											strlen(tmpStr1) + 1,
											(void**) &destPtr,
											&(hd->additionalInfo[0].infoValue));
	EXPECT_EQ ( ret, SA_AIS_OK );
	((char *)destPtr)[strlen(tmpStr1)] = '\0';
	strcpy((char *)destPtr,tmpStr1);

	hd->additionalInfo[1].infoType = SA_NTF_VALUE_STRING;
	ret = saNtfPtrValAllocate(
								20,
								strlen(tmpStr2) + 1,
								(void**) &destPtr,
								&(hd->additionalInfo[1].infoValue));
	EXPECT_EQ ( SA_AIS_OK, ret);
	((char *)destPtr)[strlen(tmpStr2)] = '\0';
	strcpy((char *)destPtr,tmpStr2);

	char value[] = "1234567";
	MafNameValuePairT *filter = new MafNameValuePairT[1];
	nFilter_V4.filter = &filter;
	nFilter_V4.filter[0]->name = NULL;
	nFilter_V4.filter[0]->value = (const char *) &value;

	maf_rc = convertHeader4(&ntf, comNot);
	EXPECT_EQ ( MafOk, maf_rc );

	//Checking the result
	EXPECT_EQ(1346475867467378548, comNot->eventTime);
	EXPECT_EQ(NULL, comNot->additionalText);
	EXPECT_EQ(hd->notificationClassId->vendorId, comNot->majorType);
	EXPECT_EQ((((uint32_t)hd->notificationClassId->majorId << 16)|(uint32_t)hd->notificationClassId->minorId), comNot->minorType);
	EXPECT_EQ(2, comNot->additionalInfo.size);
	EXPECT_EQ(0, strcmp( tmpStr1, comNot->additionalInfo.additionalInfoArr[0].value));
	EXPECT_EQ(0, strcmp( tmpStr2, comNot->additionalInfo.additionalInfoArr[1].value));

	saNameDelete(hd->notificationObject, true);
	delete hd->notificationClassId;
	freeAllAdditionInfo();
	free(comNot->dn);
	for (int i = 0; i < comNot->additionalInfo.size; i++)
	{
		free(comNot->additionalInfo.additionalInfoArr[i].name);
		free(comNot->additionalInfo.additionalInfoArr[i].value);
	}
	free(comNot->additionalInfo.additionalInfoArr);
	delete comNot;
	delete []filter;
}


TEST (NtfTest, freeAdditionalInfo1)
{
	MafOamSpiNotificationFmStruct_4T *comNot = new MafOamSpiNotificationFmStruct_4T;
	memset(comNot, 0, sizeof(MafOamSpiNotificationFmStruct_4T));

	void * destPtr = NULL;
	char * tmpStr1 = "test string 1";
	char * tmpStr2 = "test string 22";

	comNot->additionalInfo.size = 1;
	comNot->additionalInfo.additionalInfoArr = (MafOamSpiNotificationFmAdditionalInfoT *) malloc(comNot->additionalInfo.size * sizeof(MafOamSpiNotificationFmAdditionalInfoT));
	comNot->additionalInfo.additionalInfoArr[0].name = strdup("");
	comNot->additionalInfo.additionalInfoArr[0].value = strdup("");

	freeAdditionalInfo(comNot);

	//Checking the result
	EXPECT_EQ(0, comNot->additionalInfo.additionalInfoArr = NULL);
	EXPECT_EQ(0, comNot->additionalInfo.size = 0);
	free(comNot->dn);
	free(comNot->additionalText);
	delete comNot;
}


TEST (NtfTest, cleanVlist4a)
{
	nVlist4 = NULL;

	cleanVlist4();

	//Checking the result
	EXPECT_EQ(0, nVlist4);
	EXPECT_EQ(0, nVlLast4);
}

TEST (NtfTest, addNotValue4a)
{
	nVlist4 = NULL;
	MafOamSpiNotificationFmStruct_4T *mafNot1 = (MafOamSpiNotificationFmStruct_4*) calloc(1, sizeof(MafOamSpiNotificationFmStruct_4));
	MafOamSpiNotificationFmStruct_4T *mafNot2 = (MafOamSpiNotificationFmStruct_4*) calloc(1, sizeof(MafOamSpiNotificationFmStruct_4));
	addNotValue4(mafNot1);
	addNotValue4(mafNot2);
	cleanVlist4();
}

TEST (NtfTest, clearAll_V4a)
{
	MafReturnT res = MafOk;
	nFilter_V4.consumerHandle = 2;

	res = clearAll_V4();

	EXPECT_EQ ( res, MafOk );
}

TEST (NtfTest, saNtfNotificationCallback4a)
{
	MafReturnT res = MafOk;
	SaNtfSubscriptionIdT subscriptionId = 1;
	SaNtfNotificationsT ntf;
	memset(&ntf, 0, sizeof(SaNtfNotificationsT));
	ntf.notificationType = SA_NTF_TYPE_SECURITY_ALARM;
	SaNtfNotificationHeaderT *hd = &ntf.notification.securityAlarmNotification.notificationHeader;
	hd->eventTime = new SaTimeT;
	hd->additionalText = "addText";
	hd->numAdditionalInfo = 0;
	hd->notificationId = new SaNtfIdentifierT;
	memset(hd->notificationId, 0, sizeof(SaNtfIdentifierT));
	hd->notificationClassId = new SaNtfClassIdT;
	memset(hd->notificationClassId, 0, sizeof(SaNtfClassIdT));
	hd->notificationObject = new SaNameInitT;
	ntf.notification.securityAlarmNotification.severity = new SaNtfSeverityT;
	memset(ntf.notification.securityAlarmNotification.severity, 0, sizeof(SaNtfSeverityT));

	char value[] = "1234567";
	MafNameValuePairT *filter = new MafNameValuePairT[1];
	nFilter_V4.filter = &filter;
	nFilter_V4.filter[0]->name = NULL;
	nFilter_V4.filter[0]->value = (const char *) &value;

	saNtfNotificationCallback4(subscriptionId, &ntf);
	EXPECT_EQ ( res, MafOk );

	delete ntf.notification.alarmNotification.notificationHeader.eventTime;
	delete []filter;
	delete hd->notificationId;
	delete hd->notificationClassId;
	saNameDelete(hd->notificationObject, true);
	delete ntf.notification.securityAlarmNotification.severity;
}

TEST (NtfTest, saNtfNotificationCallback4b)
{
	MafReturnT res = MafOk;
	SaNtfSubscriptionIdT subscriptionId = 1;
	SaNtfNotificationsT nh;
	nh.notificationType = SA_NTF_TYPE_STATE_CHANGE;

	saNtfNotificationCallback4(subscriptionId, &nh);

	EXPECT_EQ ( res, MafOk );
}

TEST (NtfTest, saNtfNotificationCallback4c)
{
	MafReturnT res = MafOk;
	SaNtfSubscriptionIdT subscriptionId = 1;
	SaNtfNotificationsT nh;
	nh.notificationType = SA_NTF_TYPE_OBJECT_CREATE_DELETE;

	saNtfNotificationCallback4(subscriptionId, &nh);

	EXPECT_EQ ( res, MafOk );
}

TEST (NtfTest, saNtfNotificationCallback4d)
{
	MafReturnT res = MafOk;
	SaNtfSubscriptionIdT subscriptionId = 1;
	SaNtfNotificationsT nh;
	nh.notificationType = SA_NTF_TYPE_ATTRIBUTE_CHANGE;

	saNtfNotificationCallback4(subscriptionId, &nh);

	EXPECT_EQ ( res, MafOk );
}


// Test Of CMEVENT function
/*
 * Mocking CMEVENT functions
 */
std::string TestValue = "COMSA19";
std::string TeastToValue;
std::string Last_classNameReceived;
bool bNotify = true;
bool  isNotified(char* attributename, char *classname, char* immRdn)
{
	if (bNotify)
	{
		Last_classNameReceived = classname;
		std::string isNotifiedString;
		if(attributename != NULL)
			isNotifiedString = attributename;
		if (	isNotifiedString == "ccbLast" ||
				isNotifiedString == "SaImmOiCcbIdT" ||
				isNotifiedString == "saImmAttrClassName" ||
				isNotifiedString == "saImmAttrAdminOwnerName" )
			return false;
		else
			return true;
	}
	return false;
}

extern void freeAVC(MafMoAttributeValueContainer_3T *avc);
// Mock to be able to compile ComSANtf.c
// Not used by the current tests
typedef struct _testDataReceived{
	SaImmOiCcbIdT ccbId;
	std::string attrName;
	std::string membername;
	MafMoAttributeValueContainer_3T* memberValue;
} testDataReceived;
int DataReceived = 0;
typedef struct _mapDataReceived : std::map<int, testDataReceived> {
	void clear()
	{
		std::map<int, testDataReceived>::iterator ite = this->begin();
		for(; ite != this->end(); ++ite)
		{
			freeAVC((*ite).second.memberValue);
			free((*ite).second.memberValue);
		}
		std::map<int, testDataReceived>::clear();
	}
//	~_mapDataReceived()
//	{
//		std::map<int, testDataReceived>::iterator ite = this->begin();
//		for(; ite != this->end(); ++ite)
//		{
//			freeAVC((*ite).second.memberValue);
//			free((*ite).second.memberValue);
//		}
//	}
} mapDataReceived;

mapDataReceived testEntry;

MafReturnT addToCmCache(SaImmOiCcbIdT ccbId, const char *attrName, const char *immRdn, const char *memberName, MafMoAttributeValueContainer_3T *memberValue)
{
	// Store the  data so we can check it later
	if (memberName != NULL)
	{
		testDataReceived receivedData = {ccbId, attrName, memberName, memberValue};
		testEntry[DataReceived] = receivedData;
	}
	else
	{
		testDataReceived receivedData = {ccbId, attrName, "", memberValue};
		testEntry[DataReceived] = receivedData;
	}
	++DataReceived;
	return MafOk;
}

uint64_t getTime(void)
{
	return 13347892375982LL;
}

typedef struct {
	SaImmOiCcbIdT ccbId;
	MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator;
	std::string dn;
	MafOamSpiCmEvent_EventType_1T eventType;
	bool ccbLast;
} testSetData;
int DataNumber = 0;
std::map<int, testSetData> testSetDataReceived;
MafReturnT setDnInCmCache(SaImmOiCcbIdT ccbId, MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator, const char *dn, MafOamSpiCmEvent_EventType_1T eventType, bool ccbLast)
{
	testSetData receivedData = {ccbId, sourceIndicator, dn, eventType, ccbLast};
	testSetDataReceived[DataNumber] = receivedData;
	++DataNumber;
	return MafOk;
}

bool removeStructPartOfDn(char **out, const char **dn)
{
	return false;
}

// This array contains the types to be return for different attributes Names
std::map<std::string, MafOamSpiMoAttributeType_3T> ReturnType;

MafOamSpiMoAttributeType_3T  getTypeForAttribute(char* attributename, char* immRdn)
{
	std::map<std::string, MafOamSpiMoAttributeType_3T>::iterator typePointer = ReturnType.find(attributename);
	if (typePointer!=ReturnType.end())
		return typePointer->second;
	// Return crap :-(
	return MafOamSpiMoAttributeType_3_INT8;
}

extern "C" SaAisErrorT saNtfPtrValGetTest(
	SaNtfNotificationHandleT notificationHandle,
	const SaNtfValueT *value,
	void **dataPtr,
	SaUint16T *dataSize,
	bool IsDataValues = false)
{
	if (!saNtfPtrValGetTestReturnValue)
	{
		// Return failure here
		*dataPtr = NULL;
		return SA_AIS_ERR_NOT_READY;
	}
	// Use Value as InfoId
	if (IsDataValues)
	{
		std::map<int, std::string>::iterator valuePointer = myTestDataValues.find(value->int8Val);
		if (valuePointer!=myTestDataValues.end())
		{

			*dataPtr = (void*)((valuePointer->second).c_str());
			return SA_AIS_OK;
		}
	}
	else
	{
		std::map<int, std::string>::iterator valuePointer = myTestDataAttributesNames.find(value->int8Val);
		if (valuePointer!=myTestDataAttributesNames.end())
		{

			*dataPtr = (void*)((valuePointer->second).c_str());
			return SA_AIS_OK;
		}
	}
	*dataPtr = NULL;
	return SA_AIS_ERR_BAD_HANDLE;
}

/*
 * END MOCKING CMEVENT Function
 */
extern "C" MafReturnT stop_CmEventHandler(void)
{
	return MafOk;
}
extern "C" MafReturnT start_CmEventHandler(MafMgmtSpiInterfacePortal_3T *portal_MAF, SaAisErrorT (*subscriberFunction)(void), SaAisErrorT (*unsubscriberFunction)(void))
{
	return MafOk;
}
extern "C" MafReturnT push_CmEventHandler(MafOamSpiCmEvent_Notification_1T *mafCmNot)
{
	return MafOk;
}
extern "C" bool isDiscardedCcb(SaImmOiCcbIdT ccbId)
{
	return false;
}

extern "C" void removeDiscardedCcb(SaImmOiCcbIdT ccbId)
{
	return;
}

char* FakeDnString = "NOTACORRECT DN HERE";
char* convertTo3GPPDNCOMCASE(char *immRdn)
{
	// Make a fake translation for now
	return FakeDnString;
}
char* ClassNameString = "SILLYCLASSNAME";
bool  getClassName(char* immRdn, char** className)
{
	// No Class name yet here
	*className = ClassNameString;
	return true;
}
bool ReturnIsStruct = false;
bool  isStructAttribute(char *immRdn, char *saAttributeName)
{
	// No struct support in unittest yet
	return ReturnIsStruct;
}
// Extern declaration of test object:
extern "C" bool transformAttributeList(SaNtfNotificationHandleT notificationHandle,
		SaUint16T numAdditionalInfo,
		SaNtfAdditionalInfoT *additionalInfo,
		SaUint16T numAttributes,
		// One of this but not both!
		SaNtfAttributeChangeT *changedAttributes,
		SaNtfAttributeT *objectAttributes,
		char *immRdn,
		SaUint64T SaImmOiCcbIdTValue,
		bool isCreate);

// Helper method to set a SaNtfAdditionalInfoT struct
void setSaNtfAdditionalInfoT(int infoId, SaNtfValueTypeT infoType, SaNtfAdditionalInfoT *additionalInfo, SaNtfValueT infoValue)
{
	additionalInfo->infoId = infoId;
	additionalInfo->infoType = infoType;
	// This is only to fool the system.
	additionalInfo->infoValue.uint8Val = infoValue.uint8Val;
}

void setSaNtfobjectAttributes(SaNtfElementIdT attributeId,
		SaNtfValueTypeT attributeType,
		SaNtfAttributeT *AttributeChange,
		SaNtfValueT infoValue)
{
	AttributeChange->attributeId = attributeId;
	AttributeChange->attributeType = attributeType;
	// This is only to fool the system.
	if( SA_NTF_VALUE_DOUBLE == attributeType ) AttributeChange->attributeValue.doubleVal = infoValue.doubleVal;
	else if(SA_NTF_VALUE_FLOAT == attributeType) AttributeChange->attributeValue.floatVal = infoValue.floatVal;
	else if(SA_NTF_VALUE_INT32 == attributeType) AttributeChange->attributeValue.int32Val = infoValue.int32Val;
	else AttributeChange->attributeValue.uint8Val = infoValue.uint8Val;
}
void setSaNtfAttributeChangeT(SaNtfElementIdT attributeId,
		SaNtfValueTypeT attributeType,
		SaNtfAttributeChangeT *AttributeChange,
		SaNtfValueT newValue,
		SaNtfValueT oldValue)
{
	AttributeChange->attributeId = attributeId;
	AttributeChange->attributeType = attributeType;
	// This is only to fool the system.
	// This is only to fool the system.
	if( SA_NTF_VALUE_DOUBLE == attributeType ) {
		AttributeChange->newAttributeValue.doubleVal = newValue.doubleVal;
		AttributeChange->oldAttributeValue.doubleVal = oldValue.doubleVal;
	}
	else if(SA_NTF_VALUE_FLOAT == attributeType) {
		AttributeChange->newAttributeValue.floatVal = newValue.floatVal;
		AttributeChange->oldAttributeValue.floatVal = oldValue.floatVal;
	}
	else if(SA_NTF_VALUE_INT32 == attributeType) {
		AttributeChange->newAttributeValue.int32Val = newValue.int32Val;
		AttributeChange->oldAttributeValue.int32Val = oldValue.int32Val;
	}
	else {
		AttributeChange->newAttributeValue.uint8Val = newValue.uint8Val;
		AttributeChange->oldAttributeValue.uint8Val = oldValue.uint8Val;
	}


}


MafOamSpiMoAttributeType_3T  getTypeForStructMemberAttribute(char *attributename, char* membername, char *immRdn)
{
	return ReturnType[membername];
	//return MafOamSpiMoAttributeType_3_STRING;
}

//TEST (NtfCMevent1, CMEVENTTEST1)
//{
///*
// * Test to translate a SA_NTF_TYPE_ATTRIBUTE_CHANGE event containing two attribute with a value One string and one int8
// * Test to receive notification or no notification
// */
//	saNtfPtrValGetTestReturnValue = true;
//	SaNtfNotificationHandleT notificationHandle = 1;
//	SaUint16T numAdditionalInfo = 5;
//	SaNtfAdditionalInfoT additionalInfo[5];
//	// Entry One
//	SaNtfValueT infoValue;
//	infoValue.int8Val = 1;
//
//	setSaNtfAdditionalInfoT(1, SA_NTF_VALUE_STRING, &(additionalInfo[0]),  infoValue);
//	// Entry two
//	infoValue.int8Val = 2;
//
//	setSaNtfAdditionalInfoT(2, SA_NTF_VALUE_STRING, &(additionalInfo[1]), infoValue);
//	// Entry three
//	infoValue.int8Val = 3;
//
//	setSaNtfAdditionalInfoT(3, SA_NTF_VALUE_STRING, &(additionalInfo[2]), infoValue);
//	infoValue.int8Val = 4;
//
//	setSaNtfAdditionalInfoT(4, SA_NTF_VALUE_STRING, &(additionalInfo[3]), infoValue);
//	infoValue.int8Val = 5;
//
//	setSaNtfAdditionalInfoT(5, SA_NTF_VALUE_STRING, &(additionalInfo[4]), infoValue);
//
//	SaUint16T numAttributes = 5;
//	SaNtfAttributeChangeT AttributeChange[5];
//	infoValue.int8Val = 1;
//	setSaNtfAttributeChangeT(1,	SA_NTF_VALUE_STRING, &(AttributeChange[0]), infoValue);
//	infoValue.int8Val = 2;
//	setSaNtfAttributeChangeT(2,	SA_NTF_VALUE_STRING, &(AttributeChange[1]), infoValue);
//	infoValue.int8Val = 3;
//	setSaNtfAttributeChangeT(3,	SA_NTF_VALUE_STRING, &(AttributeChange[2]), infoValue);
//	infoValue.int8Val = 4;
//	setSaNtfAttributeChangeT(4,	SA_NTF_VALUE_STRING, &(AttributeChange[3]), infoValue);
//	// Test this VALUE
//	infoValue.int8Val = 125;
//	setSaNtfAttributeChangeT(5,	SA_NTF_VALUE_INT8, &(AttributeChange[4]), infoValue);
//
//	// Create the names
//	myTestDataAttributesNames[1] = "CRAP";
//	myTestDataAttributesNames[2] = "saImmAttrClassName";
//	myTestDataAttributesNames[3] = "COMSA19";
//	myTestDataAttributesNames[4] = "TESTATTRIBUTE";
//	myTestDataAttributesNames[5] = "TESTATTRIBUTENUMBERTWO";
//	// Create The values
//	myTestDataValues[1] = "CRAP";
//	myTestDataValues[2] = "TESTCLASS1";
//	myTestDataValues[3] = "COMSA19";
//	myTestDataValues[4] = "AVALUEOFSTRING";
//	myTestDataValues[5] = "INVALIDNOTTOBESEEN";
//	// Set the Types
//	ReturnType["CRAP"] = MafOamSpiMoAttributeType_3_STRING;
//	ReturnType["COMSA19"] = MafOamSpiMoAttributeType_3_STRING;
//	ReturnType["saImmAttrClassName"] = MafOamSpiMoAttributeType_3_STRING;
//	ReturnType["TESTATTRIBUTE"] = MafOamSpiMoAttributeType_3_STRING;
//	ReturnType["TESTATTRIBUTENUMBERTWO"] = MafOamSpiMoAttributeType_3_INT8;
//	// Not used by the current tests
////	typedef struct {
////		SaImmOiCcbIdT ccbId;
////		std::string attrName;
////		std::string membername;
////		MafMoAttributeValueContainer_3T* memberValue;
////	} testDataReceived;
////	int DataReceived = 0;
////	std::map<int, testDataReceived> testEntry;
//
//	testEntry.clear();
//	DataReceived = 0;
//	char *immRdn = "HELLO WORLD";
//	bool returnValue = transformAttributeList(notificationHandle,
//			numAdditionalInfo, additionalInfo,
//			numAttributes, AttributeChange, NULL, // One of this but not both!
//			immRdn, 12, false);
//	// Check that we have the right attribute name available
//	EXPECT_NE( returnValue, false );
//	EXPECT_EQ(DataReceived, 2);
//	EXPECT_EQ(testEntry[0].attrName, myTestDataAttributesNames[4]);
//	EXPECT_EQ(testEntry[0].membername, "");
//	EXPECT_EQ((testEntry[0].memberValue)->nrOfValues, 1);
//	EXPECT_EQ((testEntry[0].memberValue)->type, MafOamSpiMoAttributeType_3_STRING);
//	EXPECT_EQ(((testEntry[0].memberValue)->values)->value.theString, std::string("AVALUEOFSTRING"));
//
//	EXPECT_EQ(testEntry[1].attrName, myTestDataAttributesNames[5]);
//	EXPECT_EQ(testEntry[1].membername, "");
//	EXPECT_EQ((testEntry[1].memberValue)->nrOfValues, 1);
//	EXPECT_EQ((testEntry[1].memberValue)->type, MafOamSpiMoAttributeType_3_INT8);
//	EXPECT_EQ(((testEntry[1].memberValue)->values)->value.i8, 125);
//
//	// Now enable the notify flag for COMSA19 and test again.
//	TestValue = "CRAP";
//	// Clear received data
//	testEntry.clear();
//	DataReceived = 0;
//	returnValue = transformAttributeList(notificationHandle,
//			numAdditionalInfo, additionalInfo,
//			numAttributes, AttributeChange, NULL, // One of this but not both!
//			immRdn, 12, false);
//
//
//	EXPECT_NE ( returnValue, false );
//
//	EXPECT_EQ(DataReceived, 3);
//	EXPECT_EQ(testEntry[1].attrName, myTestDataAttributesNames[4]);
//	EXPECT_EQ(testEntry[1].membername, "");
//	EXPECT_EQ((testEntry[1].memberValue)->nrOfValues, 1);
//	EXPECT_EQ((testEntry[1].memberValue)->type, MafOamSpiMoAttributeType_3_STRING);
//	EXPECT_EQ(((testEntry[1].memberValue)->values)->value.theString, std::string("AVALUEOFSTRING"));
//
//	EXPECT_EQ(testEntry[0].attrName, myTestDataAttributesNames[3]);
//	EXPECT_EQ(testEntry[0].membername, "");
//	EXPECT_EQ((testEntry[0].memberValue)->nrOfValues, 1);
//	EXPECT_EQ((testEntry[0].memberValue)->type, MafOamSpiMoAttributeType_3_STRING);
//	EXPECT_EQ(((testEntry[0].memberValue)->values)->value.theString , myTestDataValues[3]);
//
//	EXPECT_EQ(testEntry[2].attrName, myTestDataAttributesNames[5]);
//	EXPECT_EQ(testEntry[2].membername, "");
//	EXPECT_EQ((testEntry[2].memberValue)->nrOfValues, 1);
//	EXPECT_EQ((testEntry[2].memberValue)->type, MafOamSpiMoAttributeType_3_INT8);
//	EXPECT_EQ(((testEntry[2].memberValue)->values)->value.i8, 125);
//	// Ok, now try to call with nothing notified
//	testEntry.clear();
//	DataReceived = 0;
//
//	bNotify = false;
//	returnValue = transformAttributeList(notificationHandle,
//			numAdditionalInfo, additionalInfo,
//			numAttributes, AttributeChange, NULL, // One of this but not both!
//			immRdn, 12, false);
//
//	bNotify = true;
//	EXPECT_EQ(DataReceived, 0);
//
//	// Try to test to return ERROR CODE from Ntf interface
//	saNtfPtrValGetTestReturnValue = false;
//	// Now enable the notify flag for COMSA19 and test again.
//	TestValue = "CRAP";
//	returnValue = transformAttributeList(notificationHandle,
//			numAdditionalInfo, additionalInfo,
//			numAttributes, AttributeChange, NULL, // One of this but not both!
//			immRdn, 12, false);
//
//	EXPECT_NE ( returnValue, true );
//
//	// free resources
//	int i = 0;
//}
// Now test this
extern "C" bool convertCMNotificationHeader(const SaNtfNotificationTypeT type,
		const SaNtfNotificationsT *nh);

TEST (NtfCMevent1, CMEVENTTEST2)
{
/*
 * TEST ONLY SIMPLE TYPES
 * Test to translate a SA_NTF_TYPE_OBJECT_CREATE_DELETE event containing two attribute with a value One string and one int8
 * Test to receive notification or no notification
 */
	saNtfPtrValGetTestReturnValue = true;
	SaNtfNotificationsT nh;
	nh.notification.objectCreateDeleteNotification.notificationHandle = 1;
	nh.notification.objectCreateDeleteNotification.notificationHeader.numAdditionalInfo = 8;
	SaNtfSourceIndicatorT sourceIndicator = SA_NTF_MANAGEMENT_OPERATION;
	nh.notification.objectCreateDeleteNotification.sourceIndicator = &sourceIndicator;

	SaNtfAdditionalInfoT additionalInfo[8];
	nh.notification.objectCreateDeleteNotification.notificationHeader.additionalInfo = additionalInfo;
	// Entry One
	SaNtfValueT infoValue;
	infoValue.int8Val = 1;

	setSaNtfAdditionalInfoT(0, SA_NTF_VALUE_STRING, &(additionalInfo[0]),  infoValue);
	// Entry two
	infoValue.int8Val = 2;

	setSaNtfAdditionalInfoT(1, SA_NTF_VALUE_STRING, &(additionalInfo[1]), infoValue);
	// Entry three
	infoValue.int8Val = 3;

	setSaNtfAdditionalInfoT(2, SA_NTF_VALUE_STRING, &(additionalInfo[2]), infoValue);
	// ccblast
	infoValue.int8Val = 4;

	setSaNtfAdditionalInfoT(3, SA_NTF_VALUE_STRING, &(additionalInfo[3]), infoValue);
	//TESTATTRIBUTE
	infoValue.int8Val = 5;
	setSaNtfAdditionalInfoT(4, SA_NTF_VALUE_STRING, &(additionalInfo[4]), infoValue);
	//
	infoValue.int8Val = 6;
	//TESTATTRIBUTENUMBERTWO
	setSaNtfAdditionalInfoT(5, SA_NTF_VALUE_STRING, &(additionalInfo[5]), infoValue);

	infoValue.int8Val = 7;
	//TESTATTRIBUTEFLOATINGPOINT -- DOUBLE
	setSaNtfAdditionalInfoT(6, SA_NTF_VALUE_STRING, &(additionalInfo[6]), infoValue);

	infoValue.int8Val = 8;
	//TESTATTRIBUTEFLOATINGPOINT -- FLOAT
	setSaNtfAdditionalInfoT(7, SA_NTF_VALUE_STRING, &(additionalInfo[7]), infoValue);


	nh.notification.objectCreateDeleteNotification.numAttributes = 15;
	SaNtfAttributeT AttributeChange[15] = {0};
	nh.notification.objectCreateDeleteNotification.objectAttributes = AttributeChange;
	infoValue.int8Val = 1;
	setSaNtfobjectAttributes(0,	SA_NTF_VALUE_STRING, &(AttributeChange[0]), infoValue);
	infoValue.int8Val = 2;
	setSaNtfobjectAttributes(1,	SA_NTF_VALUE_STRING, &(AttributeChange[1]), infoValue);
	infoValue.int8Val = 3;
	setSaNtfobjectAttributes(2,	SA_NTF_VALUE_UINT64, &(AttributeChange[2]), infoValue);

	// MULTIVALUE STRING BELOW
	infoValue.int8Val = 5;
	setSaNtfobjectAttributes(4,	SA_NTF_VALUE_STRING, &(AttributeChange[3]), infoValue);
	infoValue.int8Val = 6;
	setSaNtfobjectAttributes(4,	SA_NTF_VALUE_STRING, &(AttributeChange[5]), infoValue);
	infoValue.int8Val = 7;
	setSaNtfobjectAttributes(4,	SA_NTF_VALUE_STRING, &(AttributeChange[6]), infoValue);
	// Test this VALUE
	infoValue.int8Val = 125;
	setSaNtfobjectAttributes(5,	SA_NTF_VALUE_INT8, &(AttributeChange[4]), infoValue);
	// Test this VALUE
	infoValue.int8Val = 100;
	setSaNtfobjectAttributes(5,	SA_NTF_VALUE_INT8, &(AttributeChange[7]), infoValue);
	// Test this VALUE
	infoValue.int8Val = 90;
	setSaNtfobjectAttributes(5,	SA_NTF_VALUE_INT8, &(AttributeChange[8]), infoValue);
	// Test this VALUE
	infoValue.int8Val = 1;
	setSaNtfobjectAttributes(5,	SA_NTF_VALUE_INT8, &(AttributeChange[9]), infoValue);

	// Test this Floating Point Double VALUE
	infoValue.doubleVal = (double)1.2341;
	setSaNtfobjectAttributes(6,	SA_NTF_VALUE_DOUBLE, &(AttributeChange[10]), infoValue);

	// Test this Floating Point Double VALUE
	infoValue.doubleVal = (double)-1.3334;
	setSaNtfobjectAttributes(6,	SA_NTF_VALUE_DOUBLE, &(AttributeChange[11]), infoValue);

	// Test this Floating Point Float VALUE
	infoValue.floatVal = (float)1.2341;
	setSaNtfobjectAttributes(7,	SA_NTF_VALUE_FLOAT, &(AttributeChange[12]), infoValue);


	// Test this Floating Point Float VALUE
	infoValue.floatVal = (float)-1.3334;
	setSaNtfobjectAttributes(7,	SA_NTF_VALUE_FLOAT, &(AttributeChange[13]), infoValue);

	// Test this BOOL value equal to false
	// note: cannot use "setSaNtfobjectAttributes" method as before, since in COM SA Ntf the "ccbLast" is hardcoded as "uint32Val".
	AttributeChange[14].attributeId = 3;
	AttributeChange[14].attributeType = SA_NTF_VALUE_UINT32;
	AttributeChange[14].attributeValue.uint32Val = 1;

	// Create the names
	myTestDataAttributesNames[1] = "saImmAttrAdminOwnerName";
	myTestDataAttributesNames[2] = "saImmAttrClassName";
	myTestDataAttributesNames[3] = "SaImmOiCcbIdT";
	myTestDataAttributesNames[4] = "ccbLast";
	myTestDataAttributesNames[5] = "TESTATTRIBUTE";
	myTestDataAttributesNames[6] = "TESTATTRIBUTENUMBERTWO";
	myTestDataAttributesNames[7] = "TESTATTRIBUTEDOUBLE";
	myTestDataAttributesNames[8] = "TESTATTRIBUTEFLOAT";

	// Create The values
	myTestDataValues[1] = "NOTOAMSA98";
	myTestDataValues[2] = "TESTCLASS1";
	myTestDataValues[3] = "COMSA19";
	myTestDataValues[4] = "CCBLASTVALUE"; // Not returned as a string value, so this is only a name
	myTestDataValues[5] = "AVALUEOFSTRINGFIRST";
	myTestDataValues[6] = "AVALUEOFSTRINGSECOND";
	myTestDataValues[7] = "AVALUEOFSTRINGTHIRD";
	myTestDataValues[8] = "TESTATTRIBUTENUMBERTWO";
	myTestDataValues[9] = "TESTATTRIBUTEDOUBLE";
	myTestDataValues[10] = "TESTATTRIBUTEFLOAT";
	myTestDataValues[11] = "INVALID NOT TO BE SEEN";

	// Set the Types
	ReturnType["saImmAttrAdminOwnerName"] = MafOamSpiMoAttributeType_3_STRING;
	ReturnType["saImmAttrClassName"] = MafOamSpiMoAttributeType_3_STRING;
	ReturnType["SaImmOiCcbIdT"] = MafOamSpiMoAttributeType_3_UINT64;
	ReturnType["ccbLast"] = MafOamSpiMoAttributeType_3_BOOL;
	ReturnType["TESTATTRIBUTE"] = MafOamSpiMoAttributeType_3_STRING;
	ReturnType["TESTATTRIBUTENUMBERTWO"] = MafOamSpiMoAttributeType_3_INT8;
	ReturnType["TESTATTRIBUTEDOUBLE"] = MafOamSpiMoAttributeType_3_DECIMAL64;
	ReturnType["TESTATTRIBUTEFLOAT"] = MafOamSpiMoAttributeType_3_DECIMAL64;

	// RDN for IMM object
	SaNameT no;
	nh.notification.objectCreateDeleteNotification.notificationHeader.notificationObject = &no;
	saNameSet("STRANGE", nh.notification.objectCreateDeleteNotification.notificationHeader.notificationObject);

	SaNtfEventTypeT eventType = SA_NTF_OBJECT_CREATION;
	nh.notification.objectCreateDeleteNotification.notificationHeader.eventType = &eventType;
	uint64_t eventTime;
	// Ok, now try to call with nothing notified
	testEntry.clear();
	DataReceived = 0;

//	int DataNumber = 0;
//	std::map<int, testSetData> testSetDataReceived;
	DataNumber = 0;
	testSetDataReceived.clear();
	bNotify = true;
	bool returnVal = convertCMNotificationHeader(SA_NTF_TYPE_OBJECT_CREATE_DELETE,
			&nh);

	EXPECT_NE ( returnVal, false );
	EXPECT_EQ(DataReceived, 4);

//	EXPECT_EQ(testEntry[0].attrName, myTestDataAttributesNames[5]);
//	EXPECT_EQ(testEntry[0].membername, "");
//	EXPECT_EQ((testEntry[0].memberValue)->nrOfValues, 1);
//	EXPECT_EQ((testEntry[0].memberValue)->type, MafOamSpiMoAttributeType_3_STRING);
//	EXPECT_EQ(((testEntry[0].memberValue)->values)->value.theString , myTestDataValues[5]);
	testDataReceived test0 = testEntry[0];
	testDataReceived test1 = testEntry[1];
	testDataReceived test2 = testEntry[2];
	testDataReceived test3 = testEntry[3];

	// Test multivalue string
	EXPECT_EQ(testEntry[0].attrName, myTestDataAttributesNames[5]);
	EXPECT_EQ(testEntry[0].membername, "");
	EXPECT_EQ((testEntry[0].memberValue)->nrOfValues, 3);
	EXPECT_EQ((testEntry[0].memberValue)->type, MafOamSpiMoAttributeType_3_STRING);
	EXPECT_EQ(((testEntry[0].memberValue)->values[0]).value.theString , myTestDataValues[5]);
	EXPECT_EQ(((testEntry[0].memberValue)->values[1]).value.theString , myTestDataValues[6]);
	EXPECT_EQ(((testEntry[0].memberValue)->values[2]).value.theString , myTestDataValues[7]);

	// Test multivalue int8
	EXPECT_EQ(testEntry[1].attrName, myTestDataAttributesNames[6]);
	EXPECT_EQ(testEntry[1].membername, "");
	EXPECT_EQ((testEntry[1].memberValue)->nrOfValues, 4);
	EXPECT_EQ((testEntry[1].memberValue)->type, MafOamSpiMoAttributeType_3_INT8);
	EXPECT_EQ(((testEntry[1].memberValue)->values[0]).value.i8 , 125);
	EXPECT_EQ(((testEntry[1].memberValue)->values[1]).value.i8 , 100);
	EXPECT_EQ(((testEntry[1].memberValue)->values[2]).value.i8 , 90);
	EXPECT_EQ(((testEntry[1].memberValue)->values[3]).value.i8 , 1);

	// Test multivalue double
	EXPECT_EQ(testEntry[2].attrName, myTestDataAttributesNames[7]);
	EXPECT_EQ(testEntry[2].membername, "");
	EXPECT_EQ((testEntry[2].memberValue)->nrOfValues, 2);
	EXPECT_EQ((testEntry[2].memberValue)->type, MafOamSpiMoAttributeType_3_DECIMAL64);
	EXPECT_EQ(((testEntry[2].memberValue)->values[0]).value.decimal64 , 1.2341);
	EXPECT_EQ(((testEntry[2].memberValue)->values[1]).value.decimal64 , -1.3334);

	// Test multivalue float
	EXPECT_EQ(testEntry[3].attrName, myTestDataAttributesNames[8]);
	EXPECT_EQ(testEntry[3].membername, "");
	EXPECT_EQ((testEntry[3].memberValue)->nrOfValues, 2);
	EXPECT_EQ((testEntry[3].memberValue)->type, MafOamSpiMoAttributeType_3_DECIMAL64);
	EXPECT_EQ(round(((testEntry[3].memberValue)->values[0]).value.decimal64 * 10000.0f)/10000.0f , 1.2341);
	EXPECT_EQ(round(((testEntry[3].memberValue)->values[1]).value.decimal64 *10000.0f)/10000.0f , -1.3334);

	ASSERT_TRUE(Last_classNameReceived == std::string("TESTCLASS1"));
	//	typedef struct {
	//		SaImmOiCcbIdT ccbId;
	//		MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator;
	//		std::string dn;
	//		MafOamSpiCmEvent_EventType_1T eventType;
	//		bool ccbLast;
	//	} testSetData;

	ASSERT_TRUE(testSetDataReceived[0].sourceIndicator == MafOamSpiCmEvent_Unknown_1);
	ASSERT_TRUE(testSetDataReceived[0].ccbLast == true);
	ASSERT_TRUE(testSetDataReceived[0].dn == "JOHNNY");
	ASSERT_TRUE(testSetDataReceived[0].eventType == MafOamSpiCmEvent_MoCreated_1);
}

TEST (NtfCMevent1, CMEVENTTEST3)
{
/*
 * TEST ONLY STRUCT TYPES
 * Test to translate a SA_NTF_TYPE_OBJECT_CREATE_DELETE event containing two attribute with a value One string and one int8
 * Test to receive notification or no notification
 */
	saNtfPtrValGetTestReturnValue = true;
	SaNtfNotificationsT nh;
	nh.notification.objectCreateDeleteNotification.notificationHandle = 1;
	nh.notification.objectCreateDeleteNotification.notificationHeader.numAdditionalInfo = 6;
	SaNtfSourceIndicatorT sourceIndicator = SA_NTF_MANAGEMENT_OPERATION;
	nh.notification.objectCreateDeleteNotification.sourceIndicator = &sourceIndicator;

	SaNtfAdditionalInfoT additionalInfo[6];
	nh.notification.objectCreateDeleteNotification.notificationHeader.additionalInfo = additionalInfo;
	// Entry One
	SaNtfValueT infoValue;
	infoValue.int8Val = 1;

	setSaNtfAdditionalInfoT(0, SA_NTF_VALUE_STRING, &(additionalInfo[0]),  infoValue);
	// Entry two
	infoValue.int8Val = 2;

	setSaNtfAdditionalInfoT(1, SA_NTF_VALUE_STRING, &(additionalInfo[1]), infoValue);
	// Entry three
	infoValue.int8Val = 3;

	setSaNtfAdditionalInfoT(2, SA_NTF_VALUE_STRING, &(additionalInfo[2]), infoValue);
	// ccblast
	infoValue.int8Val = 4;

	setSaNtfAdditionalInfoT(3, SA_NTF_VALUE_STRING, &(additionalInfo[3]), infoValue);
	//TESTATTRIBUTE
	infoValue.int8Val = 5;
	setSaNtfAdditionalInfoT(4, SA_NTF_VALUE_STRING, &(additionalInfo[4]), infoValue);
	//
	infoValue.int8Val = 6;
	//TESTATTRIBUTENUMBERTWO
	setSaNtfAdditionalInfoT(5, SA_NTF_VALUE_STRING, &(additionalInfo[5]), infoValue);


	nh.notification.objectCreateDeleteNotification.numAttributes = 11;
	SaNtfAttributeT AttributeChange[11] = {0};
	nh.notification.objectCreateDeleteNotification.objectAttributes = AttributeChange;
	infoValue.int8Val = 1;
	setSaNtfobjectAttributes(0,	SA_NTF_VALUE_STRING, &(AttributeChange[0]), infoValue);
	infoValue.int8Val = 2;
	setSaNtfobjectAttributes(1,	SA_NTF_VALUE_STRING, &(AttributeChange[1]), infoValue);
	infoValue.int8Val = 3;
	setSaNtfobjectAttributes(2,	SA_NTF_VALUE_UINT64, &(AttributeChange[2]), infoValue);
	// Test this BOOL value equal to true
	// note: cannot use "setSaNtfobjectAttributes" method as before, since in COM SA Ntf the "ccbLast" is hardcoded as "uint32Val".
	AttributeChange[10].attributeId = 3;
	AttributeChange[10].attributeType = SA_NTF_VALUE_UINT32;
	AttributeChange[10].attributeValue.uint32Val = 1;

	// MULTIVALUE STRING BELOW
	infoValue.int8Val = 5;
	setSaNtfobjectAttributes(4,	SA_NTF_VALUE_STRING, &(AttributeChange[3]), infoValue);
	infoValue.int8Val = 6;
	setSaNtfobjectAttributes(4,	SA_NTF_VALUE_STRING, &(AttributeChange[5]), infoValue);
	infoValue.int8Val = 7;
	setSaNtfobjectAttributes(4,	SA_NTF_VALUE_STRING, &(AttributeChange[6]), infoValue);
	// Test this VALUE
	infoValue.int8Val = 125;
	setSaNtfobjectAttributes(5,	SA_NTF_VALUE_INT8, &(AttributeChange[4]), infoValue);
	// Test this VALUE
	infoValue.int8Val = 100;
	setSaNtfobjectAttributes(5,	SA_NTF_VALUE_INT8, &(AttributeChange[7]), infoValue);
	// Test this VALUE
	infoValue.int8Val = 90;
	setSaNtfobjectAttributes(5,	SA_NTF_VALUE_INT8, &(AttributeChange[8]), infoValue);
	// Test this VALUE
	infoValue.int8Val = 1;
	setSaNtfobjectAttributes(5,	SA_NTF_VALUE_INT8, &(AttributeChange[9]), infoValue);

	// Create the names
	myTestDataAttributesNames[1] = "saImmAttrAdminOwnerName";
	myTestDataAttributesNames[2] = "saImmAttrClassName";
	myTestDataAttributesNames[3] = "SaImmOiCcbIdT";
	myTestDataAttributesNames[4] = "ccbLast";
	myTestDataAttributesNames[5] = "TESTATTRIBUTE";
	myTestDataAttributesNames[6] = "TESTATTRIBUTENUMBERTWO";

	// Create The values
	myTestDataValues[1] = "NOTOAMSA98";
	myTestDataValues[2] = "TESTCLASS1";
	myTestDataValues[3] = "COMSA19";
	myTestDataValues[4] = "CCBLASTVALUE"; // Not returned as a string value, so this is only a name
	myTestDataValues[5] = "AVALUEOFSTRINGFIRST";
	myTestDataValues[6] = "AVALUEOFSTRINGSECOND";
	myTestDataValues[7] = "AVALUEOFSTRINGTHIRD";
	myTestDataValues[8] = "TESTATTRIBUTENUMBERTWO";
	myTestDataValues[9] = "INVALID NOT TO BE SEEN";
	// Set the Types
	ReturnType["saImmAttrAdminOwnerName"] = MafOamSpiMoAttributeType_3_STRING;
	ReturnType["saImmAttrClassName"] = MafOamSpiMoAttributeType_3_STRING;
	ReturnType["SaImmOiCcbIdT"] = MafOamSpiMoAttributeType_3_UINT64;
	ReturnType["ccbLast"] = MafOamSpiMoAttributeType_3_BOOL;
	ReturnType["TESTATTRIBUTE"] = MafOamSpiMoAttributeType_3_STRING;
	ReturnType["TESTATTRIBUTENUMBERTWO"] = MafOamSpiMoAttributeType_3_INT8;

	// RDN for IMM object
	SaNameT no;
	nh.notification.objectCreateDeleteNotification.notificationHeader.notificationObject = &no;
	saNameSet("id=s_JOHNNY,A=1", nh.notification.objectCreateDeleteNotification.notificationHeader.notificationObject);

	SaNtfEventTypeT eventType = SA_NTF_OBJECT_CREATION;
	nh.notification.objectCreateDeleteNotification.notificationHeader.eventType = &eventType;
	uint64_t eventTime;
	// Ok, now try to call with nothing notified
	testEntry.clear();
	DataReceived = 0;

//	int DataNumber = 0;
//	std::map<int, testSetData> testSetDataReceived;
	DataNumber = 0;
	testSetDataReceived.clear();
	bNotify = true;
	ReturnIsStruct = true; // Return that this is a struct type
	bool returnVal = convertCMNotificationHeader(SA_NTF_TYPE_OBJECT_CREATE_DELETE,
			&nh);

	EXPECT_NE ( returnVal, false );
	EXPECT_EQ(DataReceived, 2);

//	EXPECT_EQ(testEntry[0].attrName, myTestDataAttributesNames[5]);
//	EXPECT_EQ(testEntry[0].membername, "");
//	EXPECT_EQ((testEntry[0].memberValue)->nrOfValues, 1);
//	EXPECT_EQ((testEntry[0].memberValue)->type, MafOamSpiMoAttributeType_3_STRING);
//	EXPECT_EQ(((testEntry[0].memberValue)->values)->value.theString , myTestDataValues[5]);
	testDataReceived test0 = testEntry[0];
	testDataReceived test1 = testEntry[1];

	// Test multivalue string
	EXPECT_EQ(testEntry[0].attrName, "JOHNNYTESTATTRIBUTE");
	EXPECT_EQ(testEntry[0].membername, myTestDataAttributesNames[5]);
	EXPECT_EQ((testEntry[0].memberValue)->nrOfValues, 3);
	EXPECT_EQ((testEntry[0].memberValue)->type, MafOamSpiMoAttributeType_3_STRING);
	EXPECT_EQ(((testEntry[0].memberValue)->values[0]).value.theString , myTestDataValues[5]);
	EXPECT_EQ(((testEntry[0].memberValue)->values[1]).value.theString , myTestDataValues[6]);
	EXPECT_EQ(((testEntry[0].memberValue)->values[2]).value.theString , myTestDataValues[7]);

	// Test multivalue int8
	EXPECT_EQ(testEntry[1].attrName, "JOHNNYTESTATTRIBUTE");
	EXPECT_EQ(testEntry[1].membername, myTestDataAttributesNames[6]);
	EXPECT_EQ((testEntry[1].memberValue)->nrOfValues, 4);
	EXPECT_EQ((testEntry[1].memberValue)->type, MafOamSpiMoAttributeType_3_INT8);
	EXPECT_EQ(((testEntry[1].memberValue)->values[0]).value.i8 , 125);
	EXPECT_EQ(((testEntry[1].memberValue)->values[1]).value.i8 , 100);
	EXPECT_EQ(((testEntry[1].memberValue)->values[2]).value.i8 , 90);
	EXPECT_EQ(((testEntry[1].memberValue)->values[3]).value.i8 , 1);

	ASSERT_TRUE(Last_classNameReceived == std::string("TESTCLASS1"));
	//	typedef struct {
	//		SaImmOiCcbIdT ccbId;
	//		MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator;
	//		std::string dn;
	//		MafOamSpiCmEvent_EventType_1T eventType;
	//		bool ccbLast;
	//	} testSetData;

	ASSERT_TRUE(testSetDataReceived[0].sourceIndicator == MafOamSpiCmEvent_Unknown_1);
	ASSERT_TRUE(testSetDataReceived[0].ccbLast == true);
	ASSERT_TRUE(testSetDataReceived[0].dn == "JOHNNY");
	ASSERT_TRUE(testSetDataReceived[0].eventType == MafOamSpiCmEvent_MoCreated_1);

	testEntry.clear();
	DataReceived = 0;
}

// AND NOW THE COMBINED TEST OF THE SIMPLE + STRUCT IN ONE SESSION HERE :-)
TEST (NtfCMevent1, CMEVENTTEST4)
{
/*
 * TEST A OBJECT WITH BOTH SIMPLE AND STRUCT TYPES OF ATTRIBUTES
 * This means that
 * 1. We have a callback for the main object + a set call with ccbLast = false
 * 2. We have a callback for the struct object + a set call with ccbLast = true
 * Test to translate a SA_NTF_TYPE_OBJECT_CREATE_DELETE event containing 4 attribute with a value One string and one int8, float, double
 * Test to receive notification or no notification
 */
	// Clear all data since previously
	myTestDataValues.clear();
	myTestDataAttributesNames.clear();

	saNtfPtrValGetTestReturnValue = true;
	SaNtfNotificationsT nh;
	nh.notification.objectCreateDeleteNotification.notificationHandle = 1;
	nh.notification.objectCreateDeleteNotification.notificationHeader.numAdditionalInfo = 8;
	SaNtfSourceIndicatorT sourceIndicator = SA_NTF_MANAGEMENT_OPERATION;
	nh.notification.objectCreateDeleteNotification.sourceIndicator = &sourceIndicator;

	SaNtfAdditionalInfoT additionalInfo[8];
	nh.notification.objectCreateDeleteNotification.notificationHeader.additionalInfo = additionalInfo;
	// Entry One
	SaNtfValueT infoValue;
	infoValue.int8Val = 1;

	setSaNtfAdditionalInfoT(0, SA_NTF_VALUE_STRING, &(additionalInfo[0]),  infoValue);
	// Entry two
	infoValue.int8Val = 2;

	setSaNtfAdditionalInfoT(1, SA_NTF_VALUE_STRING, &(additionalInfo[1]), infoValue);
	// Entry three
	infoValue.int8Val = 3;

	setSaNtfAdditionalInfoT(2, SA_NTF_VALUE_STRING, &(additionalInfo[2]), infoValue);
	// ccblast
	infoValue.int8Val = 4;

	setSaNtfAdditionalInfoT(3, SA_NTF_VALUE_STRING, &(additionalInfo[3]), infoValue);
	//TESTATTRIBUTE
	infoValue.int8Val = 5;
	setSaNtfAdditionalInfoT(4, SA_NTF_VALUE_STRING, &(additionalInfo[4]), infoValue);
	//
	infoValue.int8Val = 6;
	//TESTATTRIBUTENUMBERTWO
	setSaNtfAdditionalInfoT(5, SA_NTF_VALUE_STRING, &(additionalInfo[5]), infoValue);

	infoValue.int8Val = 7;
	//TESTATTRIBUTEFLOATINGPOINT -- DOUBLE
	setSaNtfAdditionalInfoT(6, SA_NTF_VALUE_STRING, &(additionalInfo[6]), infoValue);

	infoValue.int8Val = 8;
	//TESTATTRIBUTEFLOATINGPOINT -- FLOAT
	setSaNtfAdditionalInfoT(7, SA_NTF_VALUE_STRING, &(additionalInfo[7]), infoValue);


	nh.notification.objectCreateDeleteNotification.numAttributes = 15;
	SaNtfAttributeT AttributeChange[15] = {0};
	nh.notification.objectCreateDeleteNotification.objectAttributes = AttributeChange;
	infoValue.int8Val = 1;
	setSaNtfobjectAttributes(0,	SA_NTF_VALUE_STRING, &(AttributeChange[0]), infoValue);
	infoValue.int8Val = 2;
	setSaNtfobjectAttributes(1,	SA_NTF_VALUE_STRING, &(AttributeChange[1]), infoValue);
	infoValue.int8Val = 3;
	setSaNtfobjectAttributes(2,	SA_NTF_VALUE_UINT64, &(AttributeChange[2]), infoValue);


	// MULTIVALUE STRING BELOW
	infoValue.int8Val = 5;
	setSaNtfobjectAttributes(4,	SA_NTF_VALUE_STRING, &(AttributeChange[3]), infoValue);
	infoValue.int8Val = 6;
	setSaNtfobjectAttributes(4,	SA_NTF_VALUE_STRING, &(AttributeChange[5]), infoValue);
	infoValue.int8Val = 7;
	setSaNtfobjectAttributes(4,	SA_NTF_VALUE_STRING, &(AttributeChange[6]), infoValue);
	// Test this VALUE
	infoValue.int8Val = 125;
	setSaNtfobjectAttributes(5,	SA_NTF_VALUE_INT8, &(AttributeChange[4]), infoValue);
	// Test this VALUE
	infoValue.int8Val = 100;
	setSaNtfobjectAttributes(5,	SA_NTF_VALUE_INT8, &(AttributeChange[7]), infoValue);
	// Test this VALUE
	infoValue.int8Val = 90;
	setSaNtfobjectAttributes(5,	SA_NTF_VALUE_INT8, &(AttributeChange[8]), infoValue);
	// Test this VALUE
	infoValue.int8Val = 1;
	setSaNtfobjectAttributes(5,	SA_NTF_VALUE_INT8, &(AttributeChange[9]), infoValue);

	// Test this Floating Point Double VALUE
	infoValue.doubleVal = (double)1.2341;
	setSaNtfobjectAttributes(6,	SA_NTF_VALUE_DOUBLE, &(AttributeChange[10]), infoValue);

	// Test this Floating Point Double VALUE
	infoValue.doubleVal = (double)-1.3334;
	setSaNtfobjectAttributes(6,	SA_NTF_VALUE_DOUBLE, &(AttributeChange[11]), infoValue);

	// Test this Floating Point Float VALUE
	infoValue.floatVal = (float)1.2341;
	setSaNtfobjectAttributes(7,	SA_NTF_VALUE_FLOAT, &(AttributeChange[12]), infoValue);


	// Test this Floating Point Float VALUE
	infoValue.floatVal = (float)-1.3334;
	setSaNtfobjectAttributes(7,	SA_NTF_VALUE_FLOAT, &(AttributeChange[13]), infoValue);
	// Test this BOOL value equal to false
	// note: cannot use "setSaNtfobjectAttributes" method as before, since in COM SA Ntf the "ccbLast" is hardcoded as "uint32Val".
	AttributeChange[14].attributeId = 3;
	AttributeChange[14].attributeType = SA_NTF_VALUE_UINT32;
	AttributeChange[14].attributeValue.uint32Val = 1;

	// Create the names
	myTestDataAttributesNames[1] = "saImmAttrAdminOwnerName";
	myTestDataAttributesNames[2] = "saImmAttrClassName";
	myTestDataAttributesNames[3] = "SaImmOiCcbIdT";
	myTestDataAttributesNames[4] = "ccbLast";
	myTestDataAttributesNames[5] = "TESTATTRIBUTE";
	myTestDataAttributesNames[6] = "TESTATTRIBUTENUMBERTWO";
	myTestDataAttributesNames[7] = "TESTATTRIBUTEDOUBLE";
	myTestDataAttributesNames[8] = "TESTATTRIBUTEFLOAT";

	// Create The values
	myTestDataValues[1] = "NOTOAMSA98";
	myTestDataValues[2] = "TESTCLASS1";
	myTestDataValues[3] = "COMSA19";
	myTestDataValues[4] = "CCBLASTVALUE"; // Not returned as a string value, so this is only a name
	myTestDataValues[5] = "AVALUEOFSTRINGFIRST";
	myTestDataValues[6] = "AVALUEOFSTRINGSECOND";
	myTestDataValues[7] = "AVALUEOFSTRINGTHIRD";
	myTestDataValues[8] = "TESTATTRIBUTENUMBERTWO";
	myTestDataValues[9] = "TESTATTRIBUTEDOUBLE";
	myTestDataValues[10] = "TESTATTRIBUTEFLOAT";
	myTestDataValues[11] = "INVALID NOT TO BE SEEN";
	// Set the Types
	ReturnType["saImmAttrAdminOwnerName"] = MafOamSpiMoAttributeType_3_STRING;
	ReturnType["saImmAttrClassName"] = MafOamSpiMoAttributeType_3_STRING;
	ReturnType["SaImmOiCcbIdT"] = MafOamSpiMoAttributeType_3_UINT64;
	ReturnType["ccbLast"] = MafOamSpiMoAttributeType_3_BOOL;
	ReturnType["TESTATTRIBUTE"] = MafOamSpiMoAttributeType_3_STRING;
	ReturnType["TESTATTRIBUTENUMBERTWO"] = MafOamSpiMoAttributeType_3_INT8;
	ReturnType["TESTATTRIBUTEDOUBLE"] = MafOamSpiMoAttributeType_3_DECIMAL64;
	ReturnType["TESTATTRIBUTEFLOAT"] = MafOamSpiMoAttributeType_3_DECIMAL64;

	// RDN for IMM object
	SaNameT no;
	nh.notification.objectCreateDeleteNotification.notificationHeader.notificationObject = &no;
	saNameSet("id=s_JOHNNY,A=1", nh.notification.objectCreateDeleteNotification.notificationHeader.notificationObject);

	SaNtfEventTypeT eventType = SA_NTF_OBJECT_CREATION;
	nh.notification.objectCreateDeleteNotification.notificationHeader.eventType = &eventType;
	uint64_t eventTime;
	// Ok, now try to call with nothing notified
	testEntry.clear();
	DataReceived = 0;

//	int DataNumber = 0;
//	std::map<int, testSetData> testSetDataReceived;
	DataNumber = 0;
	testSetDataReceived.clear();
	bNotify = true;
	ReturnIsStruct = true; // Return that this is a struct type
	bool returnVal = convertCMNotificationHeader(SA_NTF_TYPE_OBJECT_CREATE_DELETE,
			&nh);

	EXPECT_NE ( returnVal, false );
	EXPECT_EQ(DataReceived, 4);

	testDataReceived test0 = testEntry[0];
	testDataReceived test1 = testEntry[1];
	testDataReceived test2 = testEntry[2];
	testDataReceived test3 = testEntry[3];

	// Test multivalue string
	EXPECT_EQ(testEntry[0].attrName, "JOHNNYTESTATTRIBUTE");
	EXPECT_EQ(testEntry[0].membername, myTestDataAttributesNames[5]);
	EXPECT_EQ((testEntry[0].memberValue)->nrOfValues, 3);
	EXPECT_EQ((testEntry[0].memberValue)->type, MafOamSpiMoAttributeType_3_STRING);
	EXPECT_EQ(((testEntry[0].memberValue)->values[0]).value.theString , myTestDataValues[5]);
	EXPECT_EQ(((testEntry[0].memberValue)->values[1]).value.theString , myTestDataValues[6]);
	EXPECT_EQ(((testEntry[0].memberValue)->values[2]).value.theString , myTestDataValues[7]);

	// Test multivalue int8
	EXPECT_EQ(testEntry[1].attrName, "JOHNNYTESTATTRIBUTE");
	EXPECT_EQ(testEntry[1].membername, myTestDataAttributesNames[6]);
	EXPECT_EQ((testEntry[1].memberValue)->nrOfValues, 4);
	EXPECT_EQ((testEntry[1].memberValue)->type, MafOamSpiMoAttributeType_3_INT8);
	EXPECT_EQ(((testEntry[1].memberValue)->values[0]).value.i8 , 125);
	EXPECT_EQ(((testEntry[1].memberValue)->values[1]).value.i8 , 100);
	EXPECT_EQ(((testEntry[1].memberValue)->values[2]).value.i8 , 90);
	EXPECT_EQ(((testEntry[1].memberValue)->values[3]).value.i8 , 1);

	// Test multivalue double
	EXPECT_EQ(testEntry[2].attrName, "JOHNNYTESTATTRIBUTE");
	EXPECT_EQ(testEntry[2].membername, myTestDataAttributesNames[7]);
	EXPECT_EQ((testEntry[2].memberValue)->nrOfValues, 2);
	EXPECT_EQ((testEntry[2].memberValue)->type, MafOamSpiMoAttributeType_3_DECIMAL64);
	EXPECT_EQ(((testEntry[2].memberValue)->values[0]).value.decimal64 , 1.2341);
	EXPECT_EQ(((testEntry[2].memberValue)->values[1]).value.decimal64 , -1.3334);

	// Test multivalue float
	EXPECT_EQ(testEntry[3].attrName, "JOHNNYTESTATTRIBUTE");
	EXPECT_EQ(testEntry[3].membername, myTestDataAttributesNames[8]);
	EXPECT_EQ((testEntry[3].memberValue)->nrOfValues, 2);
	EXPECT_EQ((testEntry[3].memberValue)->type, MafOamSpiMoAttributeType_3_DECIMAL64);
	EXPECT_EQ(round(((testEntry[3].memberValue)->values[0]).value.decimal64 * 10000.0f)/10000.0f , 1.2341);
	EXPECT_EQ(round(((testEntry[3].memberValue)->values[1]).value.decimal64 *10000.0f)/10000.0f , -1.3334);



	ASSERT_TRUE(Last_classNameReceived == std::string("TESTCLASS1"));
	//	typedef struct {
	//		SaImmOiCcbIdT ccbId;
	//		MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator;
	//		std::string dn;
	//		MafOamSpiCmEvent_EventType_1T eventType;
	//		bool ccbLast;
	//	} testSetData;

	ASSERT_TRUE(testSetDataReceived[0].sourceIndicator == MafOamSpiCmEvent_Unknown_1);
	ASSERT_TRUE(testSetDataReceived[0].ccbLast == true);
	ASSERT_TRUE(testSetDataReceived[0].dn == "JOHNNY");
	ASSERT_TRUE(testSetDataReceived[0].eventType == MafOamSpiCmEvent_MoCreated_1);

	// Test this BOOL value equal to true
	// note: cannot use "setSaNtfobjectAttributes" method as before, since in COM SA Ntf the "ccbLast" is hardcoded as "uint32Val".
	AttributeChange[14].attributeId = 3;
	AttributeChange[14].attributeType = SA_NTF_VALUE_UINT32;
	AttributeChange[14].attributeValue.uint32Val = 1;
	ReturnIsStruct = false; // Return that this is not struct type
	// RDN for IMM object

	nh.notification.objectCreateDeleteNotification.notificationHeader.notificationObject = &no;
	saNameSet("A=1", nh.notification.objectCreateDeleteNotification.notificationHeader.notificationObject);
	testEntry.clear();
	testSetDataReceived.clear();
	DataReceived = 0;
	returnVal = convertCMNotificationHeader(SA_NTF_TYPE_OBJECT_CREATE_DELETE,
			&nh);

	EXPECT_NE ( returnVal, false );
	EXPECT_EQ(DataReceived, 4);

	test0 = testEntry[0];
	test1 = testEntry[1];
	test2 = testEntry[2];
	test3 = testEntry[3];
	// Test multivalue string
	EXPECT_EQ(testEntry[0].attrName, myTestDataAttributesNames[5]);
	EXPECT_EQ(testEntry[0].membername, "");
	EXPECT_EQ((testEntry[0].memberValue)->nrOfValues, 3);
	EXPECT_EQ((testEntry[0].memberValue)->type, MafOamSpiMoAttributeType_3_STRING);
	EXPECT_EQ(((testEntry[0].memberValue)->values[0]).value.theString , myTestDataValues[5]);
	EXPECT_EQ(((testEntry[0].memberValue)->values[1]).value.theString , myTestDataValues[6]);
	EXPECT_EQ(((testEntry[0].memberValue)->values[2]).value.theString , myTestDataValues[7]);

	// Test multivalue int8
	EXPECT_EQ(testEntry[1].attrName, myTestDataAttributesNames[6]);
	EXPECT_EQ(testEntry[1].membername, "");
	EXPECT_EQ((testEntry[1].memberValue)->nrOfValues, 4);
	EXPECT_EQ((testEntry[1].memberValue)->type, MafOamSpiMoAttributeType_3_INT8);
	EXPECT_EQ(((testEntry[1].memberValue)->values[0]).value.i8 , 125);
	EXPECT_EQ(((testEntry[1].memberValue)->values[1]).value.i8 , 100);
	EXPECT_EQ(((testEntry[1].memberValue)->values[2]).value.i8 , 90);
	EXPECT_EQ(((testEntry[1].memberValue)->values[3]).value.i8 , 1);

	// Test multivalue double
	EXPECT_EQ(testEntry[2].attrName, myTestDataAttributesNames[7]);
	EXPECT_EQ(testEntry[2].membername, "");
	EXPECT_EQ((testEntry[2].memberValue)->nrOfValues, 2);
	EXPECT_EQ((testEntry[2].memberValue)->type, MafOamSpiMoAttributeType_3_DECIMAL64);
	EXPECT_EQ(((testEntry[2].memberValue)->values[0]).value.decimal64 , 1.2341);
	EXPECT_EQ(((testEntry[2].memberValue)->values[1]).value.decimal64 , -1.3334);

	// Test multivalue float
	EXPECT_EQ(testEntry[3].attrName, myTestDataAttributesNames[8]);
	EXPECT_EQ(testEntry[3].membername, "");
	EXPECT_EQ((testEntry[3].memberValue)->nrOfValues, 2);
	EXPECT_EQ((testEntry[3].memberValue)->type, MafOamSpiMoAttributeType_3_DECIMAL64);
	EXPECT_EQ(round(((testEntry[3].memberValue)->values[0]).value.decimal64 * 10000.0f)/10000.0f , 1.2341);
	EXPECT_EQ(round(((testEntry[3].memberValue)->values[1]).value.decimal64 *10000.0f)/10000.0f , -1.3334);

	ASSERT_TRUE(Last_classNameReceived == std::string("TESTCLASS1"));
	//	typedef struct {
	//		SaImmOiCcbIdT ccbId;
	//		MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator;
	//		std::string dn;
	//		MafOamSpiCmEvent_EventType_1T eventType;
	//		bool ccbLast;
	//	} testSetData;
	// This is number two of the set calls we receive
	ASSERT_TRUE(testSetDataReceived[1].sourceIndicator == MafOamSpiCmEvent_Unknown_1);
	ASSERT_TRUE(testSetDataReceived[1].ccbLast == true);
	ASSERT_TRUE(testSetDataReceived[1].dn == "JOHNNY");
	ASSERT_TRUE(testSetDataReceived[1].eventType == MafOamSpiCmEvent_MoCreated_1);

}
TEST (NtfCMevent1, CMEVENTTEST5)
{
/*
 * TEST ONLY SIMPLE TYPES
 * Test to translate a SA_NTF_TYPE_OBJECT_CREATE_DELETE event containing two attribute with a value One string and one int8
 * Test to receive notification or no notification
 */
	// Clear all data since previously
	myTestDataValues.clear();
	myTestDataAttributesNames.clear();

	saNtfPtrValGetTestReturnValue = true;
	SaNtfNotificationsT nh;
	nh.notification.attributeChangeNotification.notificationHandle = 1;
	nh.notification.attributeChangeNotification.notificationHeader.numAdditionalInfo = 8;
	SaNtfSourceIndicatorT sourceIndicator = SA_NTF_MANAGEMENT_OPERATION;
	nh.notification.attributeChangeNotification.sourceIndicator = &sourceIndicator;

	SaNtfAdditionalInfoT additionalInfo[8];
	nh.notification.attributeChangeNotification.notificationHeader.additionalInfo = additionalInfo;
	// Entry One
	SaNtfValueT newValue;
	SaNtfValueT oldValue;

	newValue.int8Val = 1;

	setSaNtfAdditionalInfoT(0, SA_NTF_VALUE_STRING, &(additionalInfo[0]),  newValue);
	// Entry two
	newValue.int8Val = 2;

	setSaNtfAdditionalInfoT(1, SA_NTF_VALUE_STRING, &(additionalInfo[1]), newValue);
	// Entry three
	newValue.int8Val = 3;

	setSaNtfAdditionalInfoT(2, SA_NTF_VALUE_STRING, &(additionalInfo[2]), newValue);
	// ccblast
	newValue.int8Val = 4;

	setSaNtfAdditionalInfoT(3, SA_NTF_VALUE_STRING, &(additionalInfo[3]), newValue);
	//TESTATTRIBUTE
	newValue.int8Val = 5;
	setSaNtfAdditionalInfoT(4, SA_NTF_VALUE_STRING, &(additionalInfo[4]), newValue);
	//
	newValue.int8Val = 6;
	//TESTATTRIBUTENUMBERTWO
	setSaNtfAdditionalInfoT(5, SA_NTF_VALUE_STRING, &(additionalInfo[5]), newValue);

	newValue.int8Val = 7;
	//TESTATTRIBUTEFLOATINGPOINT -- DOUBLE
	setSaNtfAdditionalInfoT(6, SA_NTF_VALUE_STRING, &(additionalInfo[6]), newValue);

	newValue.int8Val = 8;
	//TESTATTRIBUTEFLOATINGPOINT -- FLOAT
	setSaNtfAdditionalInfoT(7, SA_NTF_VALUE_STRING, &(additionalInfo[7]), newValue);


	nh.notification.attributeChangeNotification.numAttributes = 15;
	SaNtfAttributeChangeT AttributeChange[15] = {0};
	nh.notification.attributeChangeNotification.changedAttributes = AttributeChange;
	newValue.int8Val = 1;
	oldValue.int8Val = 2;
	setSaNtfAttributeChangeT(0,	SA_NTF_VALUE_STRING, &(AttributeChange[0]), newValue,  oldValue);

	newValue.int8Val = 2;
	oldValue.int8Val = 3;
	setSaNtfAttributeChangeT(1,	SA_NTF_VALUE_STRING, &(AttributeChange[1]), newValue,  oldValue);

	newValue.int8Val = 3;
	oldValue.int8Val = 4;
	setSaNtfAttributeChangeT(2,	SA_NTF_VALUE_UINT64, &(AttributeChange[2]), newValue,  oldValue);


	// MULTIVALUE STRING BELOW
	newValue.int8Val = 5;
	oldValue.int8Val = 6;
	setSaNtfAttributeChangeT(4,	SA_NTF_VALUE_STRING, &(AttributeChange[3]), newValue,  oldValue);

	newValue.int8Val = 6;
	oldValue.int8Val = 7;
	setSaNtfAttributeChangeT(4,	SA_NTF_VALUE_STRING, &(AttributeChange[5]), newValue,  oldValue);

	newValue.int8Val = 7;
	oldValue.int8Val = 5;
	setSaNtfAttributeChangeT(4,	SA_NTF_VALUE_STRING, &(AttributeChange[6]), newValue,  oldValue);

	// Test this VALUE
	newValue.int8Val = 125;
	oldValue.int8Val = 126;
	setSaNtfAttributeChangeT(5,	SA_NTF_VALUE_INT8, &(AttributeChange[4]), newValue,  oldValue);
	// Test this VALUE
	newValue.int8Val = 100;
	oldValue.int8Val = 101;
	setSaNtfAttributeChangeT(5,	SA_NTF_VALUE_INT8, &(AttributeChange[7]), newValue,  oldValue);
	// Test this VALUE
	newValue.int8Val = 90;
	oldValue.int8Val = 91;
	setSaNtfAttributeChangeT(5,	SA_NTF_VALUE_INT8, &(AttributeChange[8]), newValue,  oldValue);
	// Test this VALUE
	newValue.int8Val = 1;
	oldValue.int8Val = 2;
	setSaNtfAttributeChangeT(5,	SA_NTF_VALUE_INT8, &(AttributeChange[9]), newValue,  oldValue);

	// Test this Floating Point Double VALUE
	newValue.doubleVal = (double)1.2341;
	oldValue.doubleVal = (double)-1.2341;
	setSaNtfAttributeChangeT(6,	SA_NTF_VALUE_DOUBLE, &(AttributeChange[10]), newValue,  oldValue);

	// Test this Floating Point Double VALUE
	newValue.doubleVal = (double)-1.3334;
	oldValue.doubleVal = (double)1.3334;
	setSaNtfAttributeChangeT(6,	SA_NTF_VALUE_DOUBLE, &(AttributeChange[11]), newValue,  oldValue);

	// Test this Floating Point Float VALUE
	newValue.floatVal = (float)1.2341;
	oldValue.floatVal = (float)-1.2341;
	setSaNtfAttributeChangeT(7,	SA_NTF_VALUE_FLOAT, &(AttributeChange[12]), newValue,  oldValue);


	// Test this Floating Point Float VALUE
	newValue.floatVal = (float)-1.3334;
	oldValue.floatVal = (float)1.3334;
	setSaNtfAttributeChangeT(7,	SA_NTF_VALUE_FLOAT, &(AttributeChange[13]), newValue,  oldValue);
	// Test this BOOL value equal to true
	newValue.int32Val = 1;
	oldValue.int32Val = 0;
	setSaNtfAttributeChangeT(3, SA_NTF_VALUE_INT32, &(AttributeChange[14]), newValue,  oldValue);


	// Create the names
	myTestDataAttributesNames[1] = "saImmAttrAdminOwnerName";
	myTestDataAttributesNames[2] = "saImmAttrClassName";
	myTestDataAttributesNames[3] = "SaImmOiCcbIdT";
	myTestDataAttributesNames[4] = "ccbLast";
	myTestDataAttributesNames[5] = "TESTATTRIBUTE";
	myTestDataAttributesNames[6] = "TESTATTRIBUTENUMBERTWO";
	myTestDataAttributesNames[7] = "TESTATTRIBUTEDOUBLE";
	myTestDataAttributesNames[8] = "TESTATTRIBUTEFLOAT";

	// Create The values
	myTestDataValues[1] = "NOTOAMSA98";
	myTestDataValues[2] = "TESTCLASS1";
	myTestDataValues[3] = "COMSA19";
	myTestDataValues[4] = "CCBLASTVALUE"; // Not returned as a string value, so this is only a name
	myTestDataValues[5] = "AVALUEOFSTRINGFIRST";
	myTestDataValues[6] = "AVALUEOFSTRINGSECOND";
	myTestDataValues[7] = "AVALUEOFSTRINGTHIRD";
	myTestDataValues[8] = "TESTATTRIBUTENUMBERTWO";
	myTestDataValues[9] = "TESTATTRIBUTEDOUBLE";
	myTestDataValues[10] = "TESTATTRIBUTEFLOAT";
	myTestDataValues[11] = "INVALID NOT TO BE SEEN";
	// Set the Types
	ReturnType["saImmAttrAdminOwnerName"] = MafOamSpiMoAttributeType_3_STRING;
	ReturnType["saImmAttrClassName"] = MafOamSpiMoAttributeType_3_STRING;
	ReturnType["SaImmOiCcbIdT"] = MafOamSpiMoAttributeType_3_UINT64;
	ReturnType["ccbLast"] = MafOamSpiMoAttributeType_3_BOOL;
	ReturnType["TESTATTRIBUTE"] = MafOamSpiMoAttributeType_3_STRING;
	ReturnType["TESTATTRIBUTENUMBERTWO"] = MafOamSpiMoAttributeType_3_INT8;
	ReturnType["TESTATTRIBUTEDOUBLE"] = MafOamSpiMoAttributeType_3_DECIMAL64;
	ReturnType["TESTATTRIBUTEFLOAT"] = MafOamSpiMoAttributeType_3_DECIMAL64;

	// RDN for IMM object
	SaNameT no;
	// nh.notification.attributeChangeNotification.notificationHeader.notificationObject = &no;
	// strncpy((char*)nh.notification.attributeChangeNotification.notificationHeader.notificationObject->value, "id=s_JOHNNY,A=1", strlen("id=s_JOHNNY,A=1") );
	// nh.notification.attributeChangeNotification.notificationHeader.notificationObject->length = strlen("id=s_JOHNNY,A=1");

	nh.notification.attributeChangeNotification.notificationHeader.notificationObject = &no;
	saNameSet("STRANGE", nh.notification.attributeChangeNotification.notificationHeader.notificationObject);

	SaNtfEventTypeT eventType = SA_NTF_ATTRIBUTE_CHANGED;
	nh.notification.attributeChangeNotification.notificationHeader.eventType = &eventType;
	uint64_t eventTime;
	// Ok, now try to call with nothing notified
	testEntry.clear();
	DataReceived = 0;

//	int DataNumber = 0;
//	std::map<int, testSetData> testSetDataReceived;
	DataNumber = 0;
	testSetDataReceived.clear();
	bNotify = true;
	ReturnIsStruct = true; // Return that this is a struct type
	bool returnVal = convertCMNotificationHeader(SA_NTF_TYPE_ATTRIBUTE_CHANGE,
			&nh);

	EXPECT_NE ( returnVal, false );
	EXPECT_EQ(DataReceived, 4);

	testDataReceived test0 = testEntry[0];
	testDataReceived test1 = testEntry[1];
	testDataReceived test2 = testEntry[2];
	testDataReceived test3 = testEntry[3];

	EXPECT_NE ( returnVal, false );
	EXPECT_EQ(DataReceived, 4);

	// Test multivalue string
	EXPECT_EQ(testEntry[0].attrName, myTestDataAttributesNames[5]);
	EXPECT_EQ(testEntry[0].membername, "");
	EXPECT_EQ((testEntry[0].memberValue)->nrOfValues, 3);
	EXPECT_EQ((testEntry[0].memberValue)->type, MafOamSpiMoAttributeType_3_STRING);
	EXPECT_EQ(((testEntry[0].memberValue)->values[0]).value.theString , myTestDataValues[5]);
	EXPECT_EQ(((testEntry[0].memberValue)->values[1]).value.theString , myTestDataValues[6]);
	EXPECT_EQ(((testEntry[0].memberValue)->values[2]).value.theString , myTestDataValues[7]);

	// Test multivalue int8
	EXPECT_EQ(testEntry[1].attrName, myTestDataAttributesNames[6]);
	EXPECT_EQ(testEntry[1].membername, "");
	EXPECT_EQ((testEntry[1].memberValue)->nrOfValues, 4);
	EXPECT_EQ((testEntry[1].memberValue)->type, MafOamSpiMoAttributeType_3_INT8);
	EXPECT_EQ(((testEntry[1].memberValue)->values[0]).value.i8 , 125);
	EXPECT_EQ(((testEntry[1].memberValue)->values[1]).value.i8 , 100);
	EXPECT_EQ(((testEntry[1].memberValue)->values[2]).value.i8 , 90);
	EXPECT_EQ(((testEntry[1].memberValue)->values[3]).value.i8 , 1);

	// Test multivalue double
	EXPECT_EQ(testEntry[2].attrName, myTestDataAttributesNames[7]);
	EXPECT_EQ(testEntry[2].membername, "");
	EXPECT_EQ((testEntry[2].memberValue)->nrOfValues, 2);
	EXPECT_EQ((testEntry[2].memberValue)->type, MafOamSpiMoAttributeType_3_DECIMAL64);
	EXPECT_EQ(((testEntry[2].memberValue)->values[0]).value.decimal64 , 1.2341);
	EXPECT_EQ(((testEntry[2].memberValue)->values[1]).value.decimal64 , -1.3334);

	// Test multivalue float
	EXPECT_EQ(testEntry[3].attrName, myTestDataAttributesNames[8]);
	EXPECT_EQ(testEntry[3].membername, "");
	EXPECT_EQ((testEntry[3].memberValue)->nrOfValues, 2);
	EXPECT_EQ((testEntry[3].memberValue)->type, MafOamSpiMoAttributeType_3_DECIMAL64);
	EXPECT_EQ(round(((testEntry[3].memberValue)->values[0]).value.decimal64 * 10000.0f)/10000.0f , 1.2341);
	EXPECT_EQ(round(((testEntry[3].memberValue)->values[1]).value.decimal64 *10000.0f)/10000.0f , -1.3334);

	ASSERT_TRUE(Last_classNameReceived == std::string("TESTCLASS1"));
	//	typedef struct {
	//		SaImmOiCcbIdT ccbId;
	//		MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator;
	//		std::string dn;
	//		MafOamSpiCmEvent_EventType_1T eventType;
	//		bool ccbLast;
	//	} testSetData;
	// This is number two of the set calls we receive
	ASSERT_TRUE(testSetDataReceived[0].sourceIndicator == MafOamSpiCmEvent_Unknown_1);
	ASSERT_TRUE(testSetDataReceived[0].ccbLast == true);
	ASSERT_TRUE(testSetDataReceived[0].dn == "JOHNNY");
	ASSERT_TRUE(testSetDataReceived[0].eventType == MafOamSpiCmEvent_AttributeValueChange_1);

}


TEST (NtfCMevent1, CMEVENTTEST6)
{
/*
 * TEST A CHANGE ATTRIBUTE WITH BOTH SIMPLE AND STRUCT TYPES OF ATTRIBUTES
 * This means that
 * 1. We have a callback for the main object + a set call with ccbLast = false
 * 2. We have a callback for the struct object + a set call with ccbLast = true
 * Test to translate a SA_NTF_TYPE_ATTRIBUTE_CHANGE event containing 4 attribute with a value One string and one int8, float, double
 * Test to receive notification or no notification
 */
	// Clear all data since previously
	myTestDataValues.clear();
	myTestDataAttributesNames.clear();

	saNtfPtrValGetTestReturnValue = true;
	SaNtfNotificationsT nh;
	nh.notification.attributeChangeNotification.notificationHandle = 1;
	nh.notification.attributeChangeNotification.notificationHeader.numAdditionalInfo = 8;
	SaNtfSourceIndicatorT sourceIndicator = SA_NTF_MANAGEMENT_OPERATION;
	nh.notification.attributeChangeNotification.sourceIndicator = &sourceIndicator;

	SaNtfAdditionalInfoT additionalInfo[8];
	nh.notification.attributeChangeNotification.notificationHeader.additionalInfo = additionalInfo;
	// Entry One
	SaNtfValueT newValue;
	SaNtfValueT oldValue;

	newValue.int8Val = 1;

	setSaNtfAdditionalInfoT(0, SA_NTF_VALUE_STRING, &(additionalInfo[0]),  newValue);
	// Entry two
	newValue.int8Val = 2;

	setSaNtfAdditionalInfoT(1, SA_NTF_VALUE_STRING, &(additionalInfo[1]), newValue);
	// Entry three
	newValue.int8Val = 3;

	setSaNtfAdditionalInfoT(2, SA_NTF_VALUE_STRING, &(additionalInfo[2]), newValue);
	// ccblast
	newValue.int8Val = 4;

	setSaNtfAdditionalInfoT(3, SA_NTF_VALUE_STRING, &(additionalInfo[3]), newValue);
	//TESTATTRIBUTE
	newValue.int8Val = 5;
	setSaNtfAdditionalInfoT(4, SA_NTF_VALUE_STRING, &(additionalInfo[4]), newValue);
	//
	newValue.int8Val = 6;
	//TESTATTRIBUTENUMBERTWO
	setSaNtfAdditionalInfoT(5, SA_NTF_VALUE_STRING, &(additionalInfo[5]), newValue);

	newValue.int8Val = 7;
	//TESTATTRIBUTEFLOATINGPOINT -- DOUBLE
	setSaNtfAdditionalInfoT(6, SA_NTF_VALUE_STRING, &(additionalInfo[6]), newValue);

	newValue.int8Val = 8;
	//TESTATTRIBUTEFLOATINGPOINT -- FLOAT
	setSaNtfAdditionalInfoT(7, SA_NTF_VALUE_STRING, &(additionalInfo[7]), newValue);


	nh.notification.attributeChangeNotification.numAttributes = 15;
	SaNtfAttributeChangeT AttributeChange[15] = {0};
	nh.notification.attributeChangeNotification.changedAttributes = AttributeChange;
	newValue.int8Val = 1;
	oldValue.int8Val = 2;
	setSaNtfAttributeChangeT(0,	SA_NTF_VALUE_STRING, &(AttributeChange[0]), newValue,  oldValue);
	newValue.int8Val = 2;
	oldValue.int8Val = 3;
	setSaNtfAttributeChangeT(1,	SA_NTF_VALUE_STRING, &(AttributeChange[1]), newValue,  oldValue);
	newValue.int8Val = 3;
	oldValue.int8Val = 4;
	setSaNtfAttributeChangeT(2,	SA_NTF_VALUE_UINT64, &(AttributeChange[2]), newValue,  oldValue);


	// MULTIVALUE STRING BELOW
	newValue.int8Val = 5;
	oldValue.int8Val = 6;
	setSaNtfAttributeChangeT(4,	SA_NTF_VALUE_STRING, &(AttributeChange[3]), newValue,  oldValue);
	newValue.int8Val = 6;
	oldValue.int8Val = 7;
	setSaNtfAttributeChangeT(4,	SA_NTF_VALUE_STRING, &(AttributeChange[5]), newValue,  oldValue);
	newValue.int8Val = 7;
	oldValue.int8Val = 5;
	setSaNtfAttributeChangeT(4,	SA_NTF_VALUE_STRING, &(AttributeChange[6]), newValue,  oldValue);
	// Test this VALUE
	newValue.int8Val = 125;
	oldValue.int8Val = 126;
	setSaNtfAttributeChangeT(5,	SA_NTF_VALUE_INT8, &(AttributeChange[4]), newValue,  oldValue);
	// Test this VALUE
	newValue.int8Val = 100;
	oldValue.int8Val = 101;
	setSaNtfAttributeChangeT(5,	SA_NTF_VALUE_INT8, &(AttributeChange[7]), newValue,  oldValue);
	// Test this VALUE
	newValue.int8Val = 90;
	oldValue.int8Val = 91;
	setSaNtfAttributeChangeT(5,	SA_NTF_VALUE_INT8, &(AttributeChange[8]), newValue,  oldValue);
	// Test this VALUE
	newValue.int8Val = 1;
	oldValue.int8Val = 2;
	setSaNtfAttributeChangeT(5,	SA_NTF_VALUE_INT8, &(AttributeChange[9]), newValue,  oldValue);

	// Test this Floating Point Double VALUE
	newValue.doubleVal = (double)1.2341;
	oldValue.doubleVal = (double)-1.2341;
	setSaNtfAttributeChangeT(6,	SA_NTF_VALUE_DOUBLE, &(AttributeChange[10]), newValue,  oldValue);

	// Test this Floating Point Double VALUE
	newValue.doubleVal = (double)-1.3334;
	oldValue.doubleVal = (double)1.3334;
	setSaNtfAttributeChangeT(6,	SA_NTF_VALUE_DOUBLE, &(AttributeChange[11]), newValue,  oldValue);

	// Test this Floating Point Float VALUE
	newValue.floatVal = (float)1.2341;
	oldValue.floatVal = (float)-1.2341;
	setSaNtfAttributeChangeT(7,	SA_NTF_VALUE_FLOAT, &(AttributeChange[12]), newValue,  oldValue);


	// Test this Floating Point Float VALUE
	newValue.floatVal = (float)-1.3334;
	oldValue.floatVal = (float)1.3334;
	setSaNtfAttributeChangeT(7,	SA_NTF_VALUE_FLOAT, &(AttributeChange[13]), newValue,  oldValue);
	// Test this BOOL value equal to false
	newValue.int32Val = 0;
	oldValue.int32Val = 0;
	setSaNtfAttributeChangeT(3, SA_NTF_VALUE_INT32, &(AttributeChange[14]), newValue,  oldValue);


	// Create the names
	myTestDataAttributesNames[1] = "saImmAttrAdminOwnerName";
	myTestDataAttributesNames[2] = "saImmAttrClassName";
	myTestDataAttributesNames[3] = "SaImmOiCcbIdT";
	myTestDataAttributesNames[4] = "ccbLast";
	myTestDataAttributesNames[5] = "TESTATTRIBUTE";
	myTestDataAttributesNames[6] = "TESTATTRIBUTENUMBERTWO";
	myTestDataAttributesNames[7] = "TESTATTRIBUTEDOUBLE";
	myTestDataAttributesNames[8] = "TESTATTRIBUTEFLOAT";

	// Create The values
	myTestDataValues[1] = "NOTOAMSA98";
	myTestDataValues[2] = "TESTCLASS1";
	myTestDataValues[3] = "COMSA19";
	myTestDataValues[4] = "CCBLASTVALUE"; // Not returned as a string value, so this is only a name
	myTestDataValues[5] = "AVALUEOFSTRINGFIRST";
	myTestDataValues[6] = "AVALUEOFSTRINGSECOND";
	myTestDataValues[7] = "AVALUEOFSTRINGTHIRD";
	myTestDataValues[8] = "TESTATTRIBUTENUMBERTWO";
	myTestDataValues[9] = "TESTATTRIBUTEDOUBLE";
	myTestDataValues[10] = "TESTATTRIBUTEFLOAT";
	myTestDataValues[11] = "INVALID NOT TO BE SEEN";
	// Set the Types
	ReturnType["saImmAttrAdminOwnerName"] = MafOamSpiMoAttributeType_3_STRING;
	ReturnType["saImmAttrClassName"] = MafOamSpiMoAttributeType_3_STRING;
	ReturnType["SaImmOiCcbIdT"] = MafOamSpiMoAttributeType_3_UINT64;
	ReturnType["ccbLast"] = MafOamSpiMoAttributeType_3_BOOL;
	ReturnType["TESTATTRIBUTE"] = MafOamSpiMoAttributeType_3_STRING;
	ReturnType["TESTATTRIBUTENUMBERTWO"] = MafOamSpiMoAttributeType_3_INT8;
	ReturnType["TESTATTRIBUTEDOUBLE"] = MafOamSpiMoAttributeType_3_DECIMAL64;
	ReturnType["TESTATTRIBUTEFLOAT"] = MafOamSpiMoAttributeType_3_DECIMAL64;

	// RDN for IMM object
	SaNameT no;
	nh.notification.attributeChangeNotification.notificationHeader.notificationObject = &no;
	saNameSet("id=s_JOHNNY,A=1", nh.notification.attributeChangeNotification.notificationHeader.notificationObject);

	SaNtfEventTypeT eventType = SA_NTF_ATTRIBUTE_CHANGED;
	nh.notification.attributeChangeNotification.notificationHeader.eventType = &eventType;
	uint64_t eventTime;
	// Ok, now try to call with nothing notified
	testEntry.clear();
	DataReceived = 0;

//	int DataNumber = 0;
//	std::map<int, testSetData> testSetDataReceived;
	DataNumber = 0;
	testSetDataReceived.clear();
	bNotify = true;
	ReturnIsStruct = true; // Return that this is a struct type
	bool returnVal = convertCMNotificationHeader(SA_NTF_TYPE_ATTRIBUTE_CHANGE,
			&nh);

	EXPECT_NE ( returnVal, false );
	EXPECT_EQ(DataReceived, 4);

	testDataReceived test0 = testEntry[0];
	testDataReceived test1 = testEntry[1];
	testDataReceived test2 = testEntry[2];
	testDataReceived test3 = testEntry[3];

	// Test multivalue string
	EXPECT_EQ(testEntry[0].attrName, "JOHNNYTESTATTRIBUTE");
	EXPECT_EQ(testEntry[0].membername, myTestDataAttributesNames[5]);
	EXPECT_EQ((testEntry[0].memberValue)->nrOfValues, 3);
	EXPECT_EQ((testEntry[0].memberValue)->type, MafOamSpiMoAttributeType_3_STRING);
	EXPECT_EQ(((testEntry[0].memberValue)->values[0]).value.theString , myTestDataValues[5]);
	EXPECT_EQ(((testEntry[0].memberValue)->values[1]).value.theString , myTestDataValues[6]);
	EXPECT_EQ(((testEntry[0].memberValue)->values[2]).value.theString , myTestDataValues[7]);

	// Test multivalue int8
	EXPECT_EQ(testEntry[1].attrName, "JOHNNYTESTATTRIBUTE");
	EXPECT_EQ(testEntry[1].membername, myTestDataAttributesNames[6]);
	EXPECT_EQ((testEntry[1].memberValue)->nrOfValues, 4);
	EXPECT_EQ((testEntry[1].memberValue)->type, MafOamSpiMoAttributeType_3_INT8);
	EXPECT_EQ(((testEntry[1].memberValue)->values[0]).value.i8 , 125);
	EXPECT_EQ(((testEntry[1].memberValue)->values[1]).value.i8 , 100);
	EXPECT_EQ(((testEntry[1].memberValue)->values[2]).value.i8 , 90);
	EXPECT_EQ(((testEntry[1].memberValue)->values[3]).value.i8 , 1);

	// Test multivalue double
	EXPECT_EQ(testEntry[2].attrName, "JOHNNYTESTATTRIBUTE");
	EXPECT_EQ(testEntry[2].membername, myTestDataAttributesNames[7]);
	EXPECT_EQ((testEntry[2].memberValue)->nrOfValues, 2);
	EXPECT_EQ((testEntry[2].memberValue)->type, MafOamSpiMoAttributeType_3_DECIMAL64);
	EXPECT_EQ(((testEntry[2].memberValue)->values[0]).value.decimal64 , 1.2341);
	EXPECT_EQ(((testEntry[2].memberValue)->values[1]).value.decimal64 , -1.3334);

	// Test multivalue float
	EXPECT_EQ(testEntry[3].attrName, "JOHNNYTESTATTRIBUTE");
	EXPECT_EQ(testEntry[3].membername, myTestDataAttributesNames[8]);
	EXPECT_EQ((testEntry[3].memberValue)->nrOfValues, 2);
	EXPECT_EQ((testEntry[3].memberValue)->type, MafOamSpiMoAttributeType_3_DECIMAL64);
	EXPECT_EQ(round(((testEntry[3].memberValue)->values[0]).value.decimal64 * 10000.0f)/10000.0f , 1.2341);
	EXPECT_EQ(round(((testEntry[3].memberValue)->values[1]).value.decimal64 *10000.0f)/10000.0f , -1.3334);



	ASSERT_TRUE(Last_classNameReceived == std::string("TESTCLASS1"));
	//	typedef struct {
	//		SaImmOiCcbIdT ccbId;
	//		MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator;
	//		std::string dn;
	//		MafOamSpiCmEvent_EventType_1T eventType;
	//		bool ccbLast;
	//	} testSetData;

	ASSERT_TRUE(testSetDataReceived[0].sourceIndicator == MafOamSpiCmEvent_Unknown_1);
	ASSERT_TRUE(testSetDataReceived[0].ccbLast == false);
	ASSERT_TRUE(testSetDataReceived[0].dn == "JOHNNY");
	ASSERT_TRUE(testSetDataReceived[0].eventType == MafOamSpiCmEvent_AttributeValueChange_1);

	// Test this BOOL value equal to true
	newValue.int32Val = 1;
	oldValue.int32Val = 1;
	setSaNtfAttributeChangeT(3, SA_NTF_VALUE_INT32, &(AttributeChange[14]), newValue, oldValue);
	ReturnIsStruct = false; // Return that this is not struct type
	// RDN for IMM object

	nh.notification.attributeChangeNotification.notificationHeader.notificationObject = &no;
	saNameSet("A=1", nh.notification.attributeChangeNotification.notificationHeader.notificationObject);
	testEntry.clear();
	testSetDataReceived.clear();
	DataReceived = 0;
	returnVal = convertCMNotificationHeader(SA_NTF_TYPE_ATTRIBUTE_CHANGE,
			&nh);

	EXPECT_NE ( returnVal, false );
	EXPECT_EQ(DataReceived, 4);

	test0 = testEntry[0];
	test1 = testEntry[1];
	test2 = testEntry[2];
	test3 = testEntry[3];
	// Test multivalue string
	EXPECT_EQ(testEntry[0].attrName, myTestDataAttributesNames[5]);
	EXPECT_EQ(testEntry[0].membername, "");
	EXPECT_EQ((testEntry[0].memberValue)->nrOfValues, 3);
	EXPECT_EQ((testEntry[0].memberValue)->type, MafOamSpiMoAttributeType_3_STRING);
	EXPECT_EQ(((testEntry[0].memberValue)->values[0]).value.theString , myTestDataValues[5]);
	EXPECT_EQ(((testEntry[0].memberValue)->values[1]).value.theString , myTestDataValues[6]);
	EXPECT_EQ(((testEntry[0].memberValue)->values[2]).value.theString , myTestDataValues[7]);

	// Test multivalue int8
	EXPECT_EQ(testEntry[1].attrName, myTestDataAttributesNames[6]);
	EXPECT_EQ(testEntry[1].membername, "");
	EXPECT_EQ((testEntry[1].memberValue)->nrOfValues, 4);
	EXPECT_EQ((testEntry[1].memberValue)->type, MafOamSpiMoAttributeType_3_INT8);
	EXPECT_EQ(((testEntry[1].memberValue)->values[0]).value.i8 , 125);
	EXPECT_EQ(((testEntry[1].memberValue)->values[1]).value.i8 , 100);
	EXPECT_EQ(((testEntry[1].memberValue)->values[2]).value.i8 , 90);
	EXPECT_EQ(((testEntry[1].memberValue)->values[3]).value.i8 , 1);

	// Test multivalue double
	EXPECT_EQ(testEntry[2].attrName, myTestDataAttributesNames[7]);
	EXPECT_EQ(testEntry[2].membername, "");
	EXPECT_EQ((testEntry[2].memberValue)->nrOfValues, 2);
	EXPECT_EQ((testEntry[2].memberValue)->type, MafOamSpiMoAttributeType_3_DECIMAL64);
	EXPECT_EQ(((testEntry[2].memberValue)->values[0]).value.decimal64 , 1.2341);
	EXPECT_EQ(((testEntry[2].memberValue)->values[1]).value.decimal64 , -1.3334);

	// Test multivalue float
	EXPECT_EQ(testEntry[3].attrName, myTestDataAttributesNames[8]);
	EXPECT_EQ(testEntry[3].membername, "");
	EXPECT_EQ((testEntry[3].memberValue)->nrOfValues, 2);
	EXPECT_EQ((testEntry[3].memberValue)->type, MafOamSpiMoAttributeType_3_DECIMAL64);
	EXPECT_EQ(round(((testEntry[3].memberValue)->values[0]).value.decimal64 * 10000.0f)/10000.0f , 1.2341);
	EXPECT_EQ(round(((testEntry[3].memberValue)->values[1]).value.decimal64 *10000.0f)/10000.0f , -1.3334);

	ASSERT_TRUE(Last_classNameReceived == std::string("TESTCLASS1"));
	//	typedef struct {
	//		SaImmOiCcbIdT ccbId;
	//		MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator;
	//		std::string dn;
	//		MafOamSpiCmEvent_EventType_1T eventType;
	//		bool ccbLast;
	//	} testSetData;
	// This is number two of the set calls we receive
	ASSERT_TRUE(testSetDataReceived[1].sourceIndicator == MafOamSpiCmEvent_Unknown_1);
	ASSERT_TRUE(testSetDataReceived[1].ccbLast == true);
	ASSERT_TRUE(testSetDataReceived[1].dn == "JOHNNY");
	ASSERT_TRUE(testSetDataReceived[1].eventType == MafOamSpiCmEvent_AttributeValueChange_1);

	testEntry.clear();
	testSetDataReceived.clear();
	DataReceived = 0;
}


//
//TEST (NtfCMevent1, CMEVENTTEST3)
//{
///*
// * Test to translate a SA_NTF_TYPE_OBJECT_CREATE_DELETE event containing a runtime struct attribute
// */
//	saNtfPtrValGetTestReturnValue = true;
//	SaNtfNotificationsT nh;
//	nh.notification.objectCreateDeleteNotification.notificationHandle = 1;
//	nh.notification.objectCreateDeleteNotification.notificationHeader.numAdditionalInfo = 3;
//	SaNtfSourceIndicatorT sourceIndicator = SA_NTF_MANAGEMENT_OPERATION;
//	nh.notification.objectCreateDeleteNotification.sourceIndicator = &sourceIndicator;
//
//	SaNtfAdditionalInfoT additionalInfo[2];
//	nh.notification.objectCreateDeleteNotification.notificationHeader.additionalInfo = additionalInfo;
//	// Entry One
//	SaNtfValueT infoValue;
//	infoValue.int8Val = 1;
//
//	setSaNtfAdditionalInfoT(1, SA_NTF_VALUE_STRING, &(additionalInfo[0]),  infoValue);
//	// Entry two
//	infoValue.int8Val = 2;
//
//	setSaNtfAdditionalInfoT(2, SA_NTF_VALUE_STRING, &(additionalInfo[1]), infoValue);
//	// Entry three
//	infoValue.int8Val = 3;
//
//	setSaNtfAdditionalInfoT(3, SA_NTF_VALUE_STRING, &(additionalInfo[2]), infoValue);
//
//	nh.notification.objectCreateDeleteNotification.numAttributes = 3;
//	SaNtfAttributeT AttributeChange[2];
//	nh.notification.objectCreateDeleteNotification.objectAttributes = AttributeChange;
//	infoValue.int8Val = 1;
//	setSaNtfobjectAttributes(1,	SA_NTF_VALUE_STRING, &(AttributeChange[0]), infoValue);
//	infoValue.int8Val = 2;
//	setSaNtfobjectAttributes(2,	SA_NTF_VALUE_STRING, &(AttributeChange[1]), infoValue);
//	infoValue.int8Val = 3;
//	setSaNtfobjectAttributes(3,	SA_NTF_VALUE_STRING, &(AttributeChange[2]), infoValue);
//
//	// Create the names
//	myTestDataAttributesNames[1] = "saImmAttrAdminOwnerName";
//	myTestDataAttributesNames[2] = "saImmAttrClassName";
//	myTestDataAttributesNames[3] = "COMSA19";
//
//	// Create The values
//	myTestDataValues[1] = "NOTOAMSA98";
//	myTestDataValues[2] = "TESTCLASS1";
//	myTestDataValues[3] = "COMSA19";
//
//	// Set the Types
//	ReturnType["CRAP"] = MafOamSpiMoAttributeType_3_STRING;
//	ReturnType["COMSA19"] = MafOamSpiMoAttributeType_3_STRING;
//	ReturnType["saImmAttrClassName"] = MafOamSpiMoAttributeType_3_STRING;
//
//	// RDN for IMM object, send the call back for the struct object named as id
//	SaNameT no;
//	nh.notification.objectCreateDeleteNotification.notificationHeader.notificationObject = &no;
//	strncpy((char*)nh.notification.objectCreateDeleteNotification.notificationHeader.notificationObject->value, "id=a_0", strlen("id=a_0") );
//	nh.notification.objectCreateDeleteNotification.notificationHeader.notificationObject->length = strlen("id=a_0");
//
//	SaNtfEventTypeT eventType = SA_NTF_OBJECT_CREATION;
//	nh.notification.objectCreateDeleteNotification.notificationHeader.eventType = &eventType;
//	MafOamSpiCmEvent_Notification_1T mafCmNot;
//
//	/*
//	 * Test ONE, send a correct DN and let fillStructMember return ok values
//	 */
//	bool returnVal = convertCMNotificationHeader(SA_NTF_TYPE_OBJECT_CREATE_DELETE,
//			&nh,
//			&mafCmNot);
//
//	bool result = true;
//	if ( mafCmNot.events[0]== NULL )
//	{
//		returnVal = false;
//	}
//	EXPECT_NE ( result, false );
//	EXPECT_NE ( returnVal, false );
//	ASSERT_TRUE( std::string(mafCmNot.events[0]->attributes[0]->name) == std::string("NOTVALID") );
//
//	EXPECT_EQ(mafCmNot.events[0]->attributes[0]->value.nrOfValues, 1);
//	EXPECT_EQ(mafCmNot.events[0]->attributes[0]->value.type, MafOamSpiMoAttributeType_3_STRING);
//	ASSERT_TRUE(std::string(mafCmNot.events[0]->attributes[0]->value.values->value.theString) == std::string("NOTVALID"));
//	ASSERT_TRUE(Last_classNameReceived == std::string("TESTCLASS1"));
//	ASSERT_TRUE(mafCmNot.sourceIndicator == MafOamSpiCmEvent_Unknown_1);
//	ASSERT_TRUE(mafCmNot.events[0]->eventType == MafOamSpiCmEvent_MoCreated_1);
//	/*
//	 * Test Two, send a correct DN and let fillStructMember return false and no values
//	 */
//
//	// Enable fillStructMember to return false
//	returnValuefillStructMembers = false;
//	MafOamSpiCmEvent_Notification_1T mafCmNotTestTwo;
//
//	returnVal = convertCMNotificationHeader(SA_NTF_TYPE_OBJECT_CREATE_DELETE,
//			&nh,
//			&mafCmNotTestTwo);
//
//
//	/*
//	 * Test three, send an incorrect DN
//	 */
//
//
//}

/***********************************************************************************************************
 * These positive and negative test cases test the regexp filter functionality inside the CM event producer
 *
 * Positive and negative tests
 *
 * Note: some negative cases can not be tested e.g.: event structure is NULL.
 *       In that case, COM SA would crash anyway when destructing the event structure.
 ***********************************************************************************************************/


// first regexp should match
TEST (CMeventFilterEvaluationTest, REGEXP1)
{
	const char *dn = "ManagedElement=1,SystemFunctions=1";
	filterListT filterList;
	filterList.push_back("^ManagedElement=.*$");
	filterList.push_back("^ManagedElement=Sys.*$");
	bool ret = testRegexpFilter(dn, filterList);
	ASSERT_TRUE(ret);
}

// second regexp should match
TEST (CMeventFilterEvaluationTest, REGEXP2)
{
	const char *dn = "ManagedElement=1,SystemFunctions=1";
	filterListT filterList;
	filterList.push_back("element");
	filterList.push_back("^ManagedElement=1,Sys.*$");
	filterList.push_back("house");
	bool ret = testRegexpFilter(dn, filterList);
	ASSERT_TRUE(ret);
}

// Should not match
TEST (CMeventFilterEvaluationTest, REGEXP3)
{
	const char *dn = "ManagedElement=1,SystemFunctions=1";
	filterListT filterList;
	filterList.push_back("ME");
	bool ret = testRegexpFilter(dn, filterList);
	ASSERT_FALSE(ret);
}

// Should not match
TEST (CMeventFilterEvaluationTest, REGEXP4)
{
	const char *dn = "ManagedElement=1,SystemFunctions=1";
	filterListT filterList;
	filterList.push_back("^ ManagedElement=.*$");
	bool ret = testRegexpFilter(dn, filterList);
	ASSERT_FALSE(ret);
}

// Should not match
TEST (CMeventFilterEvaluationTest, REGEXP5)
{
	const char *dn = "ManagedElement=1,SystemFunctions=1";
	filterListT filterList;
	filterList.push_back(" ManagedElement");
	bool ret = testRegexpFilter(dn, filterList);
	ASSERT_FALSE(ret);
}

// This is a special case where the filter array is empty, there is no filtering at all.
// It should match.
TEST (CMeventFilterEvaluationTest, REGEXP6)
{
	const char *dn = "ManagedElement=1,SystemFunctions=1";
	filterListT filterList;
	// calling with empty list
	bool ret = testRegexpFilter(dn, filterList);
	ASSERT_TRUE(ret);
}

// Should not match
TEST (CMeventFilterEvaluationTest, REGEXP7)
{
	const char *dn = "ManagedElement=1,SystemFunctions=1";
	filterListT filterList;
	filterList.push_back("^ManagedElement=1,SYS.*$");
	filterList.push_back("^ManagedElement=1,SAMBA.*$");
	bool ret = testRegexpFilter(dn, filterList);
	ASSERT_FALSE(ret);
}

// First regex is valid and could match, but:
// second given regular expression is invalid, so it returns false immediately after checking if it is valid
// Because the order is:
//            -isValid
//            -isMatch
TEST (CMeventFilterEvaluationTest, REGEXP8)
{
	const char *dn = "ManagedElement=1,SystemFunctions=1";
	filterListT filterList;
	filterList.push_back("^ManagedElement=.*$");
	filterList.push_back("^*");
	bool ret = testRegexpFilter(dn, filterList);
	ASSERT_FALSE(ret);
}

//HS62388
// struct telling if the Cache needs to be informed about the callback
typedef struct {
	SaImmOiCcbIdT ccbId;
	bool ccbLast;
}cacheInformation;

extern "C" void checkForMandatoryAttributes(cacheInformation *notifyCache,
		SaNtfElementIdT infoId, 						// Id of Attribute to convert
		SaNtfNotificationHandleT notificationHandle,	// Handle of transaction
		SaNtfAttributeChangeT *changedAttributes,		// Pointer to value array to search for the value, for modification
		SaNtfAttributeT *objectAttributes,				// Pointer to value array to search for the value, for creation
		SaUint16T numAttributes,						// Number of attributes
		char *saAttributeName );						// Name of attribute to scan for SaImmOiCcbIdT/ccblast


TEST (checkForMandatoryAttributesTest, saAttributeName_is_NULL)
{
	cacheInformation notifyCache;
	SaNtfElementIdT infoId = 0;
	SaNtfNotificationHandleT notificationHandle = 0;
	SaNtfAttributeChangeT *changedAttributes = NULL;
	SaNtfAttributeT *objectAttributes = NULL;
	SaUint16T numAttributes = 1;
	char *saAttributeName = NULL;

	checkForMandatoryAttributes(&notifyCache,
			 infoId,
			 notificationHandle,
			 changedAttributes,
			 objectAttributes,
			 numAttributes,
			 saAttributeName );
	EXPECT_TRUE(changedAttributes == NULL);
	EXPECT_TRUE(objectAttributes == NULL);
}
