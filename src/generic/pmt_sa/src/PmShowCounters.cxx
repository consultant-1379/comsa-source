/* ***************************************************************************
* Copyright (C) 2011 by Ericsson AB
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
* File: PmShowCounters.cxx
*
* Author: xnikvap 2015-08-27
*
* Reviewed:
* Modified:
*
************************************************************************** */
#include "PmShowCounters.hxx"
#include "PmEventHandler.hxx"
#include "imm_utils.h"
#include "saname_utils.h"
#include "ComMgmtSpiThreadContext_2.h"
#include "ComMgmtSpiServiceIdentities_1.h"
/**
* @file PmShowCounters.cxx
*
* @brief This is the implementation of the ComOamSpiPmMeasurements_3 and ComOamSpiPmMeasurements_4 COM SPI interface
*
*/
using namespace PmtSa;

PmShowCounters* PmShowCounters::s_instance = NULL;

const std::string COM_TOP_MOC_INSTANCE("ManagedElement=");
const std::string PM_GROUPID_NAME ("pmGroupId");
const std::string PM_ID ("CmwPmpmId=1");
const std::string ATTR_DESCRIPTION ("description");
const std::string MEASUREMENT_TYPE_NAME/*measurementTypeName*/("measurementTypeId");

PmShowCounters::PmShowCounters()
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmShowCounters constructor");
	s_instance = this;

	if (pthread_mutex_init(&_requestMapMutex, NULL) != 0)
	{
		PMTSA_DEBUG("PmShowCounters::PmShowCounters Failed to create requestMapMutex");
	}

	_portal = PerfMgmtTransferSA::instance().portal();
	if (!_portal) {
		PMTSA_DEBUG("PmShowCounters::PmShowCounters Unable to retrieve COM SPI Interface portal");
	}
	LEAVE_PMTSA();
}

PmShowCounters::~PmShowCounters()
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmShowCounters destructor");
	_requestMap.clear();
	if (pthread_mutex_destroy(&_requestMapMutex) != 0)
	{
		PMTSA_DEBUG("PmShowCounters::PmShowCounters Failed to destroy requestMapMutex");
	}
	s_instance = NULL;
	LEAVE_PMTSA();
}

/**
* @ingroup PmtSa
*
* Returns a pointer to the singleton PmShowCounters instance.
*
* @return	PmShowCounters*
*
*/
PmShowCounters* PmShowCounters::instance()
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmShowCounters::instance() called");
	if(s_instance == NULL)
	{
		s_instance = new PmShowCounters();
	}
	LEAVE_PMTSA();
	return s_instance;
}

MafReturnT PmShowCounters::getMeasurementValues(const char * dn,
						const ComOamSpiPmMeasurementFilter_3T* filter,
						ComOamSpiPmMeasuredObject_3T * measuredObject)
{
	ENTER_PMTSA();
	MafReturnT retVal = MafOk;

	if (NULL != dn)
	{
		PMTSA_DEBUG("PmShowCounters getMeasurementValues called with DN: [%s]", dn);
		if (0 == strlen(dn))
		{
			PMTSA_ERR("PmShowCounters getMeasurementValues called with empty DN");
			return MafNotExist;
		}
	}
	else
	{
		PMTSA_ERR("PmShowCounters getMeasurementValues called with NULL DN");
		return MafInvalidArgument;
	}
	/*
	* Truncate the dn of MeasInst from full Dn
	*/
	std::string tempDn(dn);
	std::string measInst;
	size_t pos = 0;

	pos = tempDn.find_first_of(",");
	measInst = tempDn.substr(pos+1, tempDn.size());

	SaAisErrorT rc = SA_AIS_OK;
	SaTimeT timeout = 10 * SA_TIME_ONE_SECOND;

#ifndef UNIT_TEST
	SaPmCHandleT tmpHandle = PmRunnable::instance()->m_osafPmHandle;
#else
	SaPmCHandleT tmpHandle = (SaPmCHandleT) 1;
#endif
	PMTSA_DEBUG("getMeasurementValues: tmpHandle [%lu]", (unsigned long*)tmpHandle);

	if(0 != tmpHandle)
	{
		SaPmCInstanceValuesT **instVal = NULL;
		SaConstStringT* JobName = NULL;
		SaConstStringT* MeasType = NULL;
		if(filter)
		{
			if(filter->jobId)
			{
				/*
				* Mismatch: The COM SPI can provide only one job name but the PM Consumer API
				* takes a null terminated array of names
				*/
				JobName = new SaConstStringT[PM_JOB_FILTER];
				JobName[0] = strdup(filter->jobId);
				JobName[1] = NULL;
				PMTSA_DEBUG("getMeasurementValues: JobName: [%s]", JobName[0]);
			}
			if(filter->measurementTypes)
			{
				MeasType = filter->measurementTypes;
				PMTSA_DEBUG("getMeasurementValues: MeasType[0]: [%s]", MeasType[0]);
			}
			/*
			* Note: We do nothing here with the 'verbose' flag.
			* CoreMW does not take verbose flag as input and always returns a verbose result.
			* The verbose flag if not set is used to filter out empty counters below.
			*/
		}
		else
		{
			PMTSA_DEBUG("getMeasurementValues: No filter provided");
		}

		PMTSA_DEBUG("getMeasurementValues: calling saPmCGetInstanceValues with LDN: [%s]", measInst.c_str());
#ifndef UNIT_TEST
		rc = PmConsumerInterface::instance()->saPmCGetInstanceValues(tmpHandle, measInst.c_str(), JobName, MeasType, timeout, &instVal);
#else
		rc = SA_AIS_ERR_NOT_EXIST;
#endif

		if(SA_AIS_OK != rc)
		{
			PMTSA_ERR("getMeasurementValues: saPmCGetInstanceValues returns error [%d]", (int)rc);
			LEAVE_PMTSA();
			if(SA_AIS_ERR_NOT_EXIST == rc)
			{
				LEAVE_PMTSA();
				releaseSaConstStringArray(JobName);
				return MafNotExist;
			}
			/*
			* COM actually does not expect MafTimeOut.
			* It is currently handled as MafNotExist but can be used for futire enhancements.
			*/
			else if (SA_AIS_ERR_TIMEOUT == rc)
			{
				LEAVE_PMTSA();
				releaseSaConstStringArray(JobName);
				return MafTimeOut;
			}
			else
			{
				LEAVE_PMTSA();
				releaseSaConstStringArray(JobName);
				return MafFailure;
			}
		}
		else
		{
			if(!instVal)
			{
				LEAVE_PMTSA();
				releaseSaConstStringArray(JobName);
				return MafNotExist;
			}
			/*
			* Mismatch: Since the COM structure has only one job per measurement
			* and the PM Consumer structure instVal may have  multiple jobs in it
			* we wiil return one ComOamSpiPmMeasurement_3T instance for each job in each instVal.
			*/
			unsigned int i = 0;
			unsigned int jobs = 0;
			/*
			* Sum the number of jobs in all returned instVal items in order
			* to determiine how many instances to allocate for the result
			*/
			while (instVal[i])
			{
				SaPmCInstanceValuesT *iVal = instVal[i];
				jobs += iVal->jobValuesNumber;
				i++;
			}

			unsigned int allJobs = jobs;
			PMTSA_DEBUG("getMeasurementValues: saPmCGetInstanceValues returned [%u] values with total [%u] jobs", i, jobs);

			if (i > 0 )
			{
				if (jobs > 0)
				{
					measuredObject->measurements = new ComOamSpiPmMeasurement_3T[jobs];
					PMTSA_DEBUG("getMeasurementValues: allocated SPI items for total of [%u] jobs", jobs);
				}
				else
				{
					/*
					* Is it possible to get measurement results with no jobs at all?
					* This seems unlikely because counters are returned only for active jobs and
					* if there are no jobs in this mearement instance nothing should be returned.
					* Should we return anything to COM in this case (e.g. if verbose is selected)?
					* We can allocate i ComOamSpiPmMeasurement_3T items here and return them all empty.
					*/
					LEAVE_PMTSA();
					PMTSA_WARN("getMeasurementValues: saPmCGetInstanceValues returned [%u] values with total [%u] jobs", i, jobs);
					releaseSaConstStringArray(JobName);
					return MafNotExist;
				}
			}
			else
			{
				LEAVE_PMTSA();
				PMTSA_WARN("getMeasurementValues: saPmCGetInstanceValues returned [%u] values with total [%u] jobs", i, jobs);
				releaseSaConstStringArray(JobName);
				return MafNotExist;
			}

			i = 0;
			jobs = 0;
			while (instVal[i])
			{
				unsigned int j = 0;
				SaPmCInstanceValuesT *instanceValue = instVal[i];
				i++;

				for (j = 0; j < instanceValue->jobValuesNumber; j++)
				{
					SaPmCInstanceJobValueT *instanceJobValue = &instanceValue->jobValues[j];
					unsigned int numValues = instanceJobValue->valueInfo.multiplicity;

					if (0 == numValues)
					{
						if (filter->isVerbose)
						{
							PMTSA_DEBUG("getMeasurementValues: assigning verbose SPI item for job [%d] jobs", jobs);
							PMTSA_DEBUG("getMeasurementValues: instanceJobValue->granularityPeriod [%llu]", (unsigned long long)instanceJobValue->granularityPeriod);
							measuredObject->measurements[jobs].name             = strdup(instanceValue->measurementType);
							measuredObject->measurements[jobs].jobId            = strdup(instanceJobValue->jobName);
							measuredObject->measurements[jobs].gpInSeconds      = instanceJobValue->granularityPeriod / SA_TIME_ONE_SECOND;
							measuredObject->measurements[jobs].nrOfValues       = instanceJobValue->valueInfo.multiplicity;
							measuredObject->measurements[jobs].errorInformation = strdup("No data returned");
							jobs++;
						}
					}
					else
					{
						unsigned int m = 0;
						PMTSA_DEBUG("getMeasurementValues: assigning SPI item for job [%d] jobs", jobs);
						PMTSA_DEBUG("getMeasurementValues: instanceJobValue->granularityPeriod [%llu]", (unsigned long long)instanceJobValue->granularityPeriod);
						measuredObject->measurements[jobs].name             = strdup(instanceValue->measurementType);
						measuredObject->measurements[jobs].jobId            = strdup(instanceJobValue->jobName);
						measuredObject->measurements[jobs].gpInSeconds      = instanceJobValue->granularityPeriod / SA_TIME_ONE_SECOND;
						measuredObject->measurements[jobs].nrOfValues       = instanceJobValue->valueInfo.multiplicity;
						measuredObject->measurements[jobs].errorInformation = NULL;
						measuredObject->measurements[jobs].value            = new ComOamSpiPmMeasurementValue_3T[numValues];

						if (instanceJobValue->valueInfo.isFloat)
						{
							PMTSA_DEBUG("getMeasurementValues: providing %d FLOAT values", numValues);
							measuredObject->measurements[jobs].valueType = ComOamSpiPmMeasurementValueType_3_FLOAT;
							for (m = 0; m < numValues; m++)
							{
								measuredObject->measurements[jobs].value[m].value.floatVal = instanceJobValue->values[m].floatVal;
								measuredObject->measurements[jobs].value[m].isSuspect = instanceJobValue->valueInfo.isSuspect;
							}
						}
						else
						{
							PMTSA_DEBUG("getMeasurementValues: providing %d INT values", numValues);
							measuredObject->measurements[jobs].valueType = ComOamSpiPmMeasurementValueType_3_INT;
							for (m = 0; m < numValues; m++)
							{
								measuredObject->measurements[jobs].value[m].value.intVal = instanceJobValue->values[m].intVal;
								measuredObject->measurements[jobs].value[m].isSuspect = instanceJobValue->valueInfo.isSuspect;
							}
						}
						jobs++;
					}
				}
			}
			measuredObject->nrOfMeasurements = jobs;
			PMTSA_DEBUG("getMeasurementValues: returning [%d] items out of total [%d] jobs", jobs, allJobs);
		}

		rc = PmConsumerInterface::instance()->saPmCInstanceValuesMemoryFree(tmpHandle, instVal);

		if(SA_AIS_OK != rc)
		{
			PMTSA_ERR("getMeasurementValues: saPmCInstanceValuesMemoryFree returns error [%d]", (int)rc);
			LEAVE_PMTSA();
			releaseSaConstStringArray(JobName);
			return MafFailure;   /* Is this a real failure or just a memory leak? We can log a warning and return MafOk */
		}
		releaseSaConstStringArray(JobName);
	}
	else
	{
		/* We can try to obtain a handle here rather than declaring a failure */
		PMTSA_ERR("getMeasurementValues: ERROR: No PM Consumer handle!");
		LEAVE_PMTSA();
		return MafFailure;
	}

	measuredObject->release = &releaseMeasuredObject;

	LEAVE_PMTSA();
	return retVal;
}

MafReturnT PmShowCounters::getMeasurementValues_2 (const ComOamSpiPmMeasurementFilter_4T* filter,
		const bool isVerbose,
		ComOamSpiPmMeasuredObjectMeasurements_4T* measuredObjectMeasurements)
{
	ENTER_PMTSA();

	if ((NULL == measuredObjectMeasurements))
	{
		PMTSA_DEBUG("PmShowCounters::getMeasurementValues_2 invalid output parameter.");
		LEAVE_PMTSA();
		return MafFailure;
	}

#ifndef UNIT_TEST
	SaPmCHandleT pmHandle = PmRunnable::instance()->m_osafPmHandle;
#else
	SaPmCHandleT pmHandle = SaPmCHandleT(1);
#endif

	if(0 == pmHandle)
	{
		//TODO: check if it's possible to re-acquire handle.
		PMTSA_ERR("PmShowCounters::getMeasurementValues_2 No PM Consumer handle!");
		LEAVE_PMTSA();
		return MafFailure;
	}

	MafReturnT retVal                       = MafOk;
	SaConstStringT instance                 = NULL;
	SaConstStringT* measTypes               = NULL;
	SaConstStringT* pmGroups                = NULL;
	SaTimeT timeout                         = 6 * SA_TIME_ONE_SECOND;
	SaUint32T requestId                     = 0;
	SaConstStringT errorText                = NULL;
	SaPmCInstanceValuesT_2 **instanceValues = NULL;

	if (NULL != filter) {
		instance  = trimMeasuredObject(filter->measuredObject);
		measTypes = filter->measurementTypes;
		pmGroups  = filter->groupIds;
	}
	(void)executeRequestIdOperation(GET_REQUEST_ID, requestId);

	SaAisErrorT rc = PmConsumerInterface::instance()->saPmCGetInstanceValues_2(pmHandle, instance, measTypes, pmGroups,
			timeout, &requestId, &instanceValues, &errorText);

	PMTSA_DEBUG("PmShowCounters::getMeasurementValues_2 requestId returned by saPmCGetInstanceValues_2: %d", requestId);

	if (NULL != filter) {
		releaseSaConstString(instance);
	}

	if (SA_AIS_OK != rc)
	{
		PMTSA_DEBUG("PmShowCounters::getMeasurementValues_2 saPmCGetInstanceValues_2 returned error %d", rc);
		switch(rc)
		{
		case SA_AIS_ERR_TRY_AGAIN:
		{
			if (!requestId)
			{
				PMTSA_DEBUG("PmShowCounters::getMeasurementValues_2 saPmCGetInstanceValues_2 returned no requestId with SA_AIS_ERR_TRY_AGAIN");
				retVal = MafFailure;
				break;
			}
			(void)executeRequestIdOperation(UPDATE_REQUEST_ID,requestId);
			LEAVE_PMTSA();
			return MafTryAgain;
		}
		case SA_AIS_ERR_NOT_EXIST:
		{
			retVal = MafNotExist;
			break;
		}
		case SA_AIS_ERR_INVALID_PARAM:
		{
			retVal = MafInvalidArgument;
			if (errorText) {
				std::string errorString(errorText);
				if (errorString.size() > 0) {
					if (_portal) {
						ComMgmtSpiThreadContext_2T* threadContext;
						MafReturnT errVal = (MafReturnT)(_portal->getInterface(ComMgmtSpiThreadContext_2Id, (ComMgmtSpiInterface_1T**)&threadContext));
						if ((MafOk == errVal) && (NULL != threadContext)) {
							errVal = (MafReturnT)threadContext->addMessage(ThreadContextMsgNbi_2, errorString.c_str());
							if (MafOk != errVal) {
								PMTSA_DEBUG("PmShowCounters::getMeasurementValues_2 Unable to add error message to thread context.");
							}
						} else {
							PMTSA_DEBUG("PmShowCounters::getMeasurementValues_2 Unable to get COM SPI Interface for threadContext.");
						}
					} else {
						PMTSA_DEBUG("PmShowCounters::getMeasurementValues_2 Unable to fetch SPI Interface.");
					}
				}
			}
			break;
		}
		default:
			retVal = MafFailure;
		}
	}
	else
	{
		if (NULL == instanceValues)
		{
			PMTSA_DEBUG("PmShowCounters::getMeasurementValues_2 saPmCGetInstanceValues_2 returned empty SaPmCInstanceValuesT_2 container");
			measuredObjectMeasurements->nrOfMeasurements = 0;
			measuredObjectMeasurements->release = NULL;
		}
		else
		{
			unsigned int nrOfMeasurements = 0;
			while (NULL != instanceValues[nrOfMeasurements]) { ++nrOfMeasurements; }
			measuredObjectMeasurements->nrOfMeasurements = nrOfMeasurements;

			if (nrOfMeasurements)
			{
				measuredObjectMeasurements->measurements = new ComOamSpiPmMeasurement_4T[nrOfMeasurements];

				unsigned int mIndex = 0; //like measurements Index
				while (mIndex < nrOfMeasurements)
				{
					measuredObjectMeasurements->measurements[mIndex].measurementTypeName = strdup(instanceValues[mIndex]->measurementType);
					measuredObjectMeasurements->measurements[mIndex].measuredObject      = strdup(instanceValues[mIndex]->instance);
					measuredObjectMeasurements->measurements[mIndex].jobId               = NULL;
					measuredObjectMeasurements->measurements[mIndex].groupId             = strdup(instanceValues[mIndex]->pmGroup);

					unsigned int nrOfValues = 0;
					unsigned int isFloat = 0;

					nrOfValues = measuredObjectMeasurements->measurements[mIndex].nrOfValues = instanceValues[mIndex]->valueInfo.multiplicity;

					if(nrOfValues > 0)
					{
						measuredObjectMeasurements->measurements[mIndex].value                   = new ComOamSpiPmMeasurementValue_4T[nrOfValues];
						isFloat = measuredObjectMeasurements->measurements[mIndex].valueType     = ComOamSpiPmMeasurementValueType_4T(instanceValues[mIndex]->valueInfo.isFloat);

						unsigned int vIndex= 0; //like values Index
						while(vIndex < nrOfValues)
						{
							measuredObjectMeasurements->measurements[mIndex].value[vIndex].isSuspect          = bool(instanceValues[mIndex]->valueInfo.isSuspect);
							if (isFloat) {
								measuredObjectMeasurements->measurements[mIndex].value[vIndex].value.floatVal = instanceValues[mIndex]->values[vIndex].floatVal;
							} else {
								measuredObjectMeasurements->measurements[mIndex].value[vIndex].value.intVal   = instanceValues[mIndex]->values[vIndex].intVal;
							}
							++vIndex;
						}
					}
					measuredObjectMeasurements->measurements[mIndex].gpInSeconds      = 0;
					measuredObjectMeasurements->measurements[mIndex].errorInformation = (isVerbose) ? strdup("No data returned") : NULL;
					++mIndex;
				}
			}
			measuredObjectMeasurements->release = &releaseMeasuredObjectMeasurements;
		}
	}

	//remove Request Id if exists.
	(void)executeRequestIdOperation(ERASE_REQUEST_ID, requestId);

	if (instanceValues || errorText) {
		rc = PmConsumerInterface::instance()->saPmCInstanceValuesMemoryFree_2(pmHandle, instanceValues, errorText);
		if (SA_AIS_OK != rc) {
			PMTSA_DEBUG("getMeasurementValues: saPmCInstanceValuesMemoryFree returned:%d", rc);
		}
	}
	LEAVE_PMTSA();
	return retVal;
}

MafReturnT PmShowCounters::cancelGetMeasurementValues()
{
	ENTER_PMTSA();
	SaPmCHandleT pmHandle = PmRunnable::instance()->m_osafPmHandle;
	if(0 == pmHandle)
	{
		PMTSA_ERR("PmShowCounters::cancelGetMeasurementValues No PM Consumer handle!");
		LEAVE_PMTSA();
		return MafFailure;
	}

	MafReturnT retVal = MafOk;
	SaAisErrorT rc = SA_AIS_OK;
	SaUint32T requestId;

	retVal = executeRequestIdOperation(GET_REQUEST_ID, requestId);
	if (MafOk == retVal)
	{
		retVal = executeRequestIdOperation(ERASE_REQUEST_ID, requestId);
		PMTSA_DEBUG("PmShowCounters::cancelGetMeasurementValues executeRequestIdOperation retVal:%d", retVal);

		rc = PmConsumerInterface::instance()->saPmCCancelGetInstanceValues_2(pmHandle, requestId);
		if (SA_AIS_OK != rc) {
			PMTSA_DEBUG("PmShowCounters::cancelGetMeasurementValues saPmCCancelGetInstanceValues_2 returned:%d", rc);
			retVal = MafFailure;
		}
	}
	return retVal;
}

MafReturnT PmShowCounters::getMeasurementNames_2(const ComOamSpiPmMeasurementFilter_4T* filter,
		ComOamSpiPmMeasurementOutput_4T* measurementTypes)
{
	ENTER_PMTSA();

	if(NULL == measurementTypes)
	{
		PMTSA_DEBUG("PmShowCounters::getMeasurementNames_2 invalid output parameter.");
		LEAVE_PMTSA();
		return MafFailure;

	}

#ifndef UNIT_TEST
	SaPmCHandleT tmpHandle = PmRunnable::instance()->m_osafPmHandle;
#else
	SaPmCHandleT tmpHandle = (SaPmCHandleT)1;
#endif

	PMTSA_DEBUG("PmShowCounters::getMeasurementNames_2 tmpHandle [%lu]", (unsigned long*)tmpHandle);

	if(0 == tmpHandle){
		PMTSA_ERR("PmShowCounters::getMeasurementNames_2 PMConsumer handle is unavailable!");
		LEAVE_PMTSA();
		return MafFailure;
	}

	SaAisErrorT rc = SA_AIS_OK;
	SaTimeT timeout = 500 * SA_TIME_ONE_MILLISECOND;
	SaConstStringT measObjName = NULL;
	SaConstStringT measType = NULL;
	SaConstStringT* group = NULL;
	SaConstStringT *measTypeNames = NULL;

	if(NULL != filter){
		measObjName = trimMeasuredObject(filter->measuredObject);
		if(filter->measurementTypes != NULL){
			if(filter->measurementTypes[0]){
				measType=strdup(filter->measurementTypes[0]);
			}
		}
		group = filter->groupIds;
	}

	rc = PmConsumerInterface::instance()->saPmCGetMeasurementTypeNames(tmpHandle, measObjName, measType, group, timeout, &measTypeNames);
	PMTSA_DEBUG("PmShowCounters::getMeasurementNames_2 saPmCGetMeasurementTypeNames received return code [%d]", (int)rc);

	if(NULL != filter)
	{
		releaseSaConstString(measType);
		releaseSaConstString(measObjName);
	}

	if(SA_AIS_OK != rc)
	{
		PMTSA_DEBUG("PmShowCounters::getMeasurementNames_2 saPmCGetMeasurementTypeNames returned error [%d]", (int)rc);
		LEAVE_PMTSA();
		if(SA_AIS_ERR_NOT_EXIST == rc)
		{
			return MafNotExist;
		}
		else if (SA_AIS_ERR_TIMEOUT == rc)
		{
			return MafTimeOut;
		}
		if (SA_AIS_ERR_INVALID_PARAM == rc)
		{
			return MafInvalidArgument;
		}
		return MafFailure;
	}

	if(measTypeNames == NULL){
		PMTSA_DEBUG("PmShowCounters::getMeasurementNames_2 saPmCGetMeasurementTypeNames returned empty ComOamSpiPmMeasurementOutput_4T container");
		measurementTypes->names =  NULL;
		measurementTypes->release = NULL;
	}
	else{
		measurementTypes->names =  duplicateConstCharArray(measTypeNames);
		measurementTypes->release = &releaseMeasurementOutput;
		rc = PmConsumerInterface::instance()->saPmCNamesMemoryFree(tmpHandle, measTypeNames);
		if(SA_AIS_OK != rc){
			PMTSA_DEBUG("PmShowCounters::getMeasurementNames_2 saPmCNamesMemoryFree returned [%d]", (int)rc);
		}
	}
	LEAVE_PMTSA();
	return MafOk;
}

MafReturnT PmShowCounters::getPmGroupIds(const ComOamSpiPmMeasurementFilter_4T* filter, ComOamSpiPmMeasurementOutput_4T* groupIds)
{
	ENTER_PMTSA();

	if(NULL == groupIds)
	{
		PMTSA_DEBUG("PmShowCounters::getPmGroupIds invalid output parameter.");
		LEAVE_PMTSA();
		return MafFailure;

	}
#ifndef UNIT_TEST
	SaPmCHandleT tmpHandle = PmRunnable::instance()->m_osafPmHandle;
#else
	SaPmCHandleT tmpHandle = (SaPmCHandleT) 1;
#endif

	PMTSA_DEBUG("PmShowCounters::getPmGroupIds tmpHandle [%lu]", (unsigned long*)tmpHandle);
	if(0 == tmpHandle)
	{
		PMTSA_ERR("PmShowCounters::getPmGroupIds PMConsumer handle is unavailable!");
		LEAVE_PMTSA();
		return MafFailure;
	}

	SaAisErrorT rc = SA_AIS_OK;
	SaTimeT timeout = 500 * SA_TIME_ONE_MILLISECOND;
	SaConstStringT measObjName=NULL;
	SaConstStringT *measType = NULL;
	SaConstStringT group = NULL;
	SaConstStringT *groupNames = NULL;
	if(NULL != filter)
	{
		measObjName = trimMeasuredObject(filter->measuredObject);
		if(filter->groupIds != NULL ){
			if(filter->groupIds[0] != NULL){
				group = strdup(filter->groupIds[0]);
			}
		}
		measType = filter->measurementTypes;
	}

	rc = PmConsumerInterface::instance()->saPmCGetPmGroupNames(tmpHandle, measObjName, measType, group, timeout, &groupNames);
	PMTSA_DEBUG("PmShowCounters::getPmGroupIds saPmCGetPmGroupNames received return code [%d]", (int)rc);

	if (NULL != filter)
	{
		releaseSaConstString(group);
		releaseSaConstString(measObjName);
	}

	if(SA_AIS_OK != rc)
	{
		PMTSA_DEBUG("PmShowCounters::getPmGroupIds saPmCGetPmGroupNames returned error [%d]", (int)rc);
		LEAVE_PMTSA();
		if(SA_AIS_ERR_NOT_EXIST == rc)
		{
			return MafNotExist;
		}
		if (SA_AIS_ERR_TIMEOUT == rc)
		{
			return MafTimeOut;
		}
		if(SA_AIS_ERR_INVALID_PARAM == rc)
		{
			return MafInvalidArgument;
		}
		return MafFailure;
	}

	if(groupNames == NULL){
		PMTSA_DEBUG("PmShowCounters::getPmGroupIds saPmCGetPmGroupNames returned empty ComOamSpiPmMeasurementOutput_4T container");
		groupIds->names =  NULL;
		groupIds->release =NULL;
	}
	else{
		groupIds->names =  duplicateConstCharArray(groupNames);
		groupIds->release = &releaseMeasurementOutput;
		rc = PmConsumerInterface::instance()->saPmCNamesMemoryFree(tmpHandle, groupNames);
		if(SA_AIS_OK != rc){
			PMTSA_DEBUG("PmShowCounters::getPmGroupIds saPmCNamesMemoryFree returned [%d]", (int)rc);
		}
	}
	LEAVE_PMTSA();
	return MafOk;
}

MafReturnT PmShowCounters::getMeasuredObjects(const ComOamSpiPmMeasurementFilter_4T* filter,
		ComOamSpiPmMeasurementOutput_4T* measuredObjects)
{
	ENTER_PMTSA();
	if( NULL == measuredObjects )
	{
		PMTSA_DEBUG("PmShowCounter::getMeasuredObjects invalid output parameter.");
		LEAVE_PMTSA();
		return MafFailure;
	}

#ifndef UNIT_TEST
	SaPmCHandleT tmpHandle = PmRunnable::instance()->m_osafPmHandle;
#else
	SaPmCHandleT tmpHandle = (SaPmCHandleT) 1;
#endif
	if(0 == tmpHandle)
	{
		PMTSA_ERR("PmShowCounter::getMeasuredObjects No PM Consumer handle!");
		LEAVE_PMTSA();
		return MafFailure;
	}
	PMTSA_DEBUG("PmShowCounter::getMeasuredObjects tmpHandle [%lu]", (unsigned long*)tmpHandle);

	SaAisErrorT rc = SA_AIS_OK;
	SaTimeT timeout = 6000 * SA_TIME_ONE_MILLISECOND;
	SaConstStringT measObj = NULL;
	SaConstStringT* groupIds = NULL;
	SaConstStringT* measType = NULL;
	SaConstStringT* measObjNames = NULL;
	if(NULL != filter)
	{
		measObj = trimMeasuredObject(filter->measuredObject);
		groupIds = filter->groupIds;
		measType = filter->measurementTypes;
	}

	rc = PmConsumerInterface::instance()->saPmCGetInstanceNames(tmpHandle, measObj, measType, groupIds, timeout, &measObjNames);
	PMTSA_DEBUG("PmShowCounters::getMeasuredObjects saPmCGetInstanceNames received return code [%d]", (int)rc);

	if(NULL != filter){
		releaseSaConstString(measObj);
	}

	if(SA_AIS_OK != rc)
	{
		PMTSA_DEBUG("PmShowCounter::getMeasuredObjects returned error [%d]", (int)rc);
		LEAVE_PMTSA();
		if(SA_AIS_ERR_NOT_EXIST == rc)
		{
			return MafNotExist;
		}
		if (SA_AIS_ERR_TIMEOUT == rc)
		{
			return MafTimeOut;
		}
		if (SA_AIS_ERR_INVALID_PARAM == rc)
		{
			return MafInvalidArgument;
		}
		return MafFailure;
	}

	if (measObjNames == NULL){
		PMTSA_DEBUG("PmShowCounter::getMeasuredObjects saPmCGetInstanceNames returned empty ComOamSpiPmMeasurementOutput_4T container");
		measuredObjects->names = NULL;
		measuredObjects->release = NULL;
	}
	else{
		measuredObjects->names   = duplicateConstCharArray(measObjNames);
		measuredObjects->release = &releaseMeasurementOutput;
		rc = PmConsumerInterface::instance()->saPmCNamesMemoryFree(tmpHandle, measObjNames);
		if(SA_AIS_OK != rc){
			PMTSA_DEBUG("PmShowCounter::getMeasuredObjects saPmCNamesMemoryFree returned [%d]", (int)rc);
		}
	}
	LEAVE_PMTSA();
	return MafOk;
}

MafReturnT PmShowCounters::getGroupIdDescription(const char* groupId, ComOamSpiPmGroupIdDescription_4T* description)
{
	ENTER_PMTSA();
	MafReturnT retVal = MafOk;

	if ((NULL == groupId) || (0 == strlen(groupId)) || (NULL == description)) {
		PMTSA_DEBUG("PmShowCounters::getGroupIdDescription called with invalid input parameter(s)");
		return MafFailure;
	}

	PMTSA_DEBUG("PmShowCounters::getGroupIdDescription called with GroupId: [%s]", groupId);
	SaVersionT immVersion = { immReleaseCode, immMajorVersion, immMinorVersion };
	SaImmHandleT immOmHandle;
	SaAisErrorT err = SA_AIS_OK;

	err = autoRetry_saImmOmInitialize(&immOmHandle, NULL, &immVersion);
	if(err != SA_AIS_OK) {
		PMTSA_DEBUG("PmShowCounters::getGroupIdDescription cannot initialize immOmHandle, error code : [%d]", (int)err);
		return MafFailure;
	}

	SaImmAccessorHandleT accessorHandle;
	err = autoRetry_saImmOmAccessorInitialize(immOmHandle, &accessorHandle);
	if(SA_AIS_OK != err) {
		PMTSA_DEBUG("PmShowCounters::getGroupIdDescription cannot initialize saImmOmAccessorInitialize, error code : [%d]", (int)err);
		(void)autoRetry_saImmOmFinalize(immOmHandle);
		return MafFailure;
	}
	// Preparing parameters to invoke saImmOmAccessorGet_2
	std::string groupIdName = PM_GROUPID_NAME + "=" + groupId;
	SaImmAttrNameT attrNames[2] = { (char *)ATTR_DESCRIPTION.c_str(), NULL};
	std::string groupId_rootName = groupIdName + "," + PM_ID;

	SaNameT rootName;
	saNameSet(groupId_rootName.c_str(), &rootName);
	SaImmAttrValuesT_2 **attributes = NULL;

	err = autoRetry_saImmOmAccessorGet_2(accessorHandle, (const SaNameT*)&rootName, attrNames, &attributes);
	if(SA_AIS_OK != err){
		PMTSA_DEBUG("PmShowCounters::getGroupIdDescription saImmOmAccessorGet2 returned error code : [%d]", (int)err);
		retVal = (err == SA_AIS_ERR_NOT_EXIST) ? MafNotExist:MafFailure;
	} else {
		PMTSA_DEBUG("PmShowCounters::getGroupIdDescription get description attribute value");
		std::string attrValue = getDescription(attributes);
		if(attrValue.length() !=0 ){
			description->description = strdup(attrValue.c_str());
		} else {
			description->description = "";
		}
		description->release = &releaseGroupIdDescription;
		retVal = MafOk;
	}
	PMTSA_DEBUG("PmShowCounters::getGroupIdDescription saImmOmAccessorGet_2 returned code : %d", (int)err);
	// Finalize the accessorHandle
	(void)autoRetry_saImmOmAccessorFinalize(accessorHandle);

	// Finalize the immOmHandle
	err = autoRetry_saImmOmFinalize(immOmHandle);
	LEAVE_PMTSA();
	return retVal;
}

MafReturnT PmShowCounters::getMeasurementTypeDescription(const char* measurementType,
		const char** groupIds, ComOamSpiPmMeasurementTypeDescription_4T* description)
{
	ENTER_PMTSA();
	MafReturnT retVal = MafOk;

	if ((NULL == measurementType) || (0 == strlen(measurementType) || ( NULL == description ) )) {
		PMTSA_DEBUG("PmShowCounters::getMeasurementTypeDescription called with invalid parameter(s).");
		return MafFailure;
	}

	PMTSA_DEBUG("PmShowCounters::getMeasurementTypeDescription called with measurementType: [%s]", measurementType);
	SaVersionT immVersion = { immReleaseCode, immMajorVersion, immMinorVersion };
	SaImmHandleT immOmHandle;
	SaAisErrorT err = SA_AIS_OK;

	err = autoRetry_saImmOmInitialize(&immOmHandle, NULL, &immVersion);
	if(err != SA_AIS_OK) {
		PMTSA_WARN("PmShowCounters::getMeasurementTypeDescription cannot initialize immOmHandle, error code : [%s]", err);
		return MafFailure;
	}

	// Preparing parameters to invoke saImmOmSearchInitialize_2
	std::string measurementType_Name = MEASUREMENT_TYPE_NAME + "=" + measurementType; //measurementTypeId=CcMT-1

	uint32_t count =0;
	bool receivedGroupIds = false;
	if(NULL != groupIds) {
		for (uint32_t i=0; groupIds[i]!= NULL; i++) {
			count++;
		}
		receivedGroupIds = (count > 0);
		if(count == 1 ){
			if(strcmp(groupIds[0], "*") == 0 ) {
				//if * is provided, then consider it as NULL
				receivedGroupIds = false;
			}
		}
	} else {
		PMTSA_DEBUG("PmShowCounters::getMeasurementTypeDescription received NULL for groupIds");
	}

	SaStringT attrValue_MeasTypeId[2] ={ (char*)measurementType_Name.c_str(),NULL};
	SaImmSearchParametersT_2 params = (SaImmSearchParametersT_2){
		{ (SaImmAttrNameT)MEASUREMENT_TYPE_NAME.c_str(),
		  SA_IMM_ATTR_SASTRINGT,
		  (SaImmAttrValueT)attrValue_MeasTypeId } };
	SaImmSearchOptionsT options = SA_IMM_SEARCH_ONE_ATTR|SA_IMM_SEARCH_GET_SOME_ATTR;
	SaImmSearchHandleT searchHandle;
	SaImmAccessorHandleT accessorHandle;
	SaImmAttrNameT attrNames[2] = { (char *)ATTR_DESCRIPTION.c_str(), NULL};
	SaNameT rootName;

	typedef std::map<std::string, std::string> measType_map;
	measType_map measType_desc;

	if(!receivedGroupIds) {
		SaImmAttrValuesT_2 **attributes = NULL;
		saNameSet(PM_ID.c_str() , &rootName);
		err = autoRetry_saImmOmSearchInitialize_2(immOmHandle, (const SaNameT*)&rootName , SA_IMM_SUBTREE, options, &params, attrNames, &searchHandle);
		if(err != SA_AIS_OK) {
			PMTSA_DEBUG("PmShowCounters::getMeasurementTypeDescription cannot initialize saImmOmSearchInitialize_2, error code : [%d]", (int)err);
			(void)autoRetry_saImmOmFinalize(immOmHandle);
			return MafFailure;
		}
		SaNameT objectName;
		while ((err = autoRetry_saImmOmSearchNext_2(searchHandle, &objectName, &attributes))== SA_AIS_OK)
		{
			std::string dn = "";
			dn.append(saNameGet(&objectName), saNameLen(&objectName));
			PMTSA_DEBUG("PmShowCounters::getGroupIdDescription get description attribute value");
			std::string attrValue = getDescription(attributes);
			// send empty if description is not available
			measType_desc[dn] = attrValue;
		}
		(void)autoRetry_saImmOmSearchFinalize(searchHandle);
	} else {
		err = autoRetry_saImmOmAccessorInitialize(immOmHandle, &accessorHandle);
		if(SA_AIS_OK != err) {
			PMTSA_DEBUG("PmShowCounters::getMeasurementTypeDescription cannot initialize saImmOmAccessorInitialize, error code : [%d]", (int)err);
			(void)autoRetry_saImmOmFinalize(immOmHandle);
			return MafFailure;
		}
		SaImmAttrValuesT_2 **attributes = NULL;
		for (uint32_t i=0; groupIds[i]!= NULL; i++) {
			std::string complete_dn = getCompleteDn(measurementType,groupIds[i]);
			saNameSet(complete_dn.c_str(), &rootName);

			err = autoRetry_saImmOmAccessorGet_2(accessorHandle, (const SaNameT*)&rootName, attrNames, &attributes);
			if(SA_AIS_OK != err){
				PMTSA_DEBUG("PmShowCounters::getMeasurementTypeDescription saImmOmAccessorGet2 returned error code: [%d]", (int)err);
				break;
			} else {
				PMTSA_DEBUG("PmShowCounters::getGroupIdDescription get description attribute value");
				std::string attrValue = getDescription(attributes);
				// send empty if description is not available
				measType_desc[complete_dn] = attrValue;
			}
		}
		(void)autoRetry_saImmOmAccessorFinalize(accessorHandle);
	}
	// Finalize the immOmHandle
	err = autoRetry_saImmOmFinalize(immOmHandle);

	if(measType_desc.size() > 0) {
		description->measurementTypeInfo = new ComOamSpiPmMeasurementTypeInfo_4T[measType_desc.size()];
		description->nrOfValues = 0;
		for (measType_map::iterator it = measType_desc.begin(); it != measType_desc.end(); it++) {
			std::string groupId_value = getGroupIdValue(it->first);
			if(groupId_value.length() != 0)
			{
				description->measurementTypeInfo[description->nrOfValues].groupId = strdup(groupId_value.c_str());
				if(it->second.length() != 0){
					description->measurementTypeInfo[description->nrOfValues].description = strdup(it->second.c_str());
				} else {
					description->measurementTypeInfo[description->nrOfValues].description = "";
				}
				description->nrOfValues++;
			} else {
				PMTSA_DEBUG("PmShowCounters::getMeasurementTypeDescription couldn't extract pmGroupId value from DN : %s",it->first.c_str());
			}
		}
		description->release = &releaseMeasurementTypeDescription;
	} else {
		PMTSA_DEBUG("PmShowCounters::getMeasurementTypeDescription saImmOmSearchNext_2/saImmOmAccessorGet_2 didn't fetch values");
	}

	// Clear the Map
	measType_desc.clear();
	if(receivedGroupIds) {
		if(err == SA_AIS_OK  && description->nrOfValues > 0) {
			retVal = MafOk;
		} else if(err == SA_AIS_ERR_NOT_EXIST) {
			retVal = MafNotExist;
		} else {
			retVal = MafFailure;
		}
	} else {
		if((err == SA_AIS_OK || err == SA_AIS_ERR_NOT_EXIST) && description->nrOfValues > 0) {
			retVal = MafOk;
		} else if( err == SA_AIS_ERR_NOT_EXIST && description->nrOfValues == 0) {
			retVal = MafNotExist;
		}else {
			retVal = MafFailure;
		}
	}
	PMTSA_DEBUG("PmShowCounters::getMeasurementTypeDescription saImmOmSearchNext_2/saImmOmAccessorGet_2 returned code : %d", (int)err);
	LEAVE_PMTSA();
	return retVal;
}


MafReturnT PmShowCounters::executeRequestIdOperation(requestIdOperationT operation, SaUint32T& requestId)
{
	ENTER_PMTSA();
	MafReturnT retVal = MafNotExist;
	pthread_mutex_lock(&_requestMapMutex);
	pthread_t currentThread = pthread_self();
	requestMapIterT it = _requestMap.find(currentThread);
	bool threadExists = (it != _requestMap.end());

	switch(operation)
	{
	case GET_REQUEST_ID:
		if (threadExists)
		{
			requestId = it->second;
			PMTSA_DEBUG("PmShowCounters::executeRequestIdOperation Found requestId associated with current session. requestId=%d", requestId);
			retVal = MafOk;
		}
		break;

	case UPDATE_REQUEST_ID:
		if (threadExists)
		{
			if (requestId == it->second) {
				PMTSA_DEBUG("PmShowCounters::executeRequestIdOperation requestId:%d associated with current session already exists", requestId);
				retVal = MafAlreadyExist;
				break;
			}
			_requestMap.erase(it);
		}
		_requestMap[currentThread] = requestId;
		PMTSA_DEBUG("PmShowCounters::executeRequestIdOperation requestId:%d insterted into requestMap", requestId);
		retVal = MafOk;
		break;

	case ERASE_REQUEST_ID:
		if (threadExists)
		{
			if (requestId != it->second) {
				PMTSA_DEBUG("PmShowCounters::executeRequestIdOperation requestId:%d exists, but is not matching with current session.", requestId);
			}
			_requestMap.erase(it);
		}
		retVal = MafOk;
		break;
	default:
		retVal = MafInvalidArgument;
	}
	pthread_mutex_unlock(&_requestMapMutex);
	PMTSA_DEBUG("PmShowCounters::executeRequestIdOperation Value of RequestId after executeRequestIdOperation: %d", requestId);
	PMTSA_DEBUG("PmShowCounters::executeRequestIdOperation _requestMap.size:%d", _requestMap.size());
	LEAVE_PMTSA();
	return retVal;
}

const char** PmShowCounters::duplicateConstCharArray(const char** input)
{
	if (NULL == input) {
		return NULL;
	}

	unsigned int nrOfElements = 0;
	try {
		while(NULL != input[nrOfElements++]) {};
	}
	catch (...) {
		PMTSA_DEBUG("PmShowCounters::duplicateConstCharArray Unable to process the input array.");
		return NULL;
	}

	char** output = new char*[nrOfElements];
	unsigned int index = 0;
	while(index < (nrOfElements-1))
	{
		output[index] = strdup(input[index]);
		PMTSA_DEBUG("PmShowCounters::duplicateConstCharArray Copied element: %u [%s]", index, output[index]);
		index++;
	}
	output[index] = NULL;
	return (const char**)output;
}

const char* PmShowCounters::trimMeasuredObject(const char* input)
{
	if (NULL != input) {
		std::string tempInstance(input);
		if (tempInstance.size() > 0) {
			size_t pos = 0;
			try {
				pos = tempInstance.find(COM_TOP_MOC_INSTANCE);
				if (pos == 0) {
					pos = tempInstance.find_first_of(",");
					if (pos != std::string::npos) {
						tempInstance = tempInstance.substr(pos + 1);
					}
				}
				PMTSA_DEBUG("PmShowCounters::trimMeasuredObject input[%s], output[%s]", input, tempInstance.c_str());
				return strdup(tempInstance.c_str());
			}
			catch (...) {
				PMTSA_DEBUG("PmShowCounters::trimMeasuredObject: Unable to trim measuredObject.");
			}
		}
		PMTSA_DEBUG("PmShowCounters::trimMeasuredObject Empty input received. returning NULL");
	}
	return NULL;
}

/* ---------------------------- The C interface ---------------------------- */

MafReturnT pmtsa_getMeasurementValues(const char * dn, const ComOamSpiPmMeasurementFilter_3T* filter, ComOamSpiPmMeasuredObject_3T * measuredObject)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("pmtsa_getMeasurementValues is being called");
	MafReturnT retVal = PmShowCounters::instance()->getMeasurementValues(dn, filter, measuredObject);
	LEAVE_PMTSA();
	return retVal;
}

MafReturnT pmtsa_getMeasurementNames(const char * dn, ComOamSpiPmMeasuredObjectMeasurementNames_3T* measurementNames)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("pmtsa_getMeasurementNames is being called");
	MafReturnT retVal = MafNoResources;
	PMTSA_DEBUG("PmShowCounters pmtsa_getMeasurementNames is disabled, returns nothing");
	LEAVE_PMTSA();
	return retVal;
}

MafReturnT pmtsa_getPmJobIds(const char * dn, ComOamSpiPmMeasuredObjectJobIds_3T* measurementJobIds)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("pmtsa_getPmJobIds is being called");
	MafReturnT retVal = MafNoResources;
	PMTSA_DEBUG("PmShowCounters pmtsa_getPmJobIds is disabled, returns nothing");
	LEAVE_PMTSA();
	return retVal;
}

MafReturnT pmtsa_getMeasurementValues_2(const ComOamSpiPmMeasurementFilter_4T* filter, const bool isVerbose, ComOamSpiPmMeasuredObjectMeasurements_4T* measurements)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("pmtsa_getMeasurementValues_2 is being called");
	MafReturnT retVal = PmShowCounters::instance()->getMeasurementValues_2(filter, isVerbose, measurements);
	LEAVE_PMTSA();
	return retVal;
}

MafReturnT pmtsa_cancelGetMeasurementValues()
{
	ENTER_PMTSA();
	PMTSA_DEBUG("cancelGetMeasurementValues is being called");
	MafReturnT retVal = PmShowCounters::instance()->cancelGetMeasurementValues();
	LEAVE_PMTSA();
	return retVal;
}

MafReturnT pmtsa_getMeasurementNames_2(const ComOamSpiPmMeasurementFilter_4T* filter, ComOamSpiPmMeasurementOutput_4T* measurementTypes)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("pmtsa_getMeasurementNames_2 is being called");
	MafReturnT retVal = PmShowCounters::instance()->getMeasurementNames_2(filter, measurementTypes);
	LEAVE_PMTSA();
	return retVal;
}

MafReturnT pmtsa_getPmJobIds_2(const ComOamSpiPmMeasurementFilter_4T* filter, ComOamSpiPmMeasurementOutput_4T* jobIds)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("pmtsa_getPmJobIds_2 is being called");
	MafReturnT retVal = MafNoResources;
	LEAVE_PMTSA();
	return retVal;
}

MafReturnT pmtsa_getPmGroupIds(const ComOamSpiPmMeasurementFilter_4T* filter, ComOamSpiPmMeasurementOutput_4T* groupIds)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("pmtsa_getPmGroupIds is being called");
	MafReturnT retVal = PmShowCounters::instance()->getPmGroupIds(filter, groupIds);
	LEAVE_PMTSA();
	return retVal;
}

MafReturnT pmtsa_getMeasuredObjects(const ComOamSpiPmMeasurementFilter_4T* filter, ComOamSpiPmMeasurementOutput_4T* measuredObjects)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("pmtsa_getMeasuredObjects is being called");
	MafReturnT retVal = PmShowCounters::instance()->getMeasuredObjects(filter, measuredObjects);
	LEAVE_PMTSA();
	return retVal;
}

MafReturnT pmtsa_getMeasurementTypeDescription(const char* measurementType, const char** groupIds, ComOamSpiPmMeasurementTypeDescription_4T* description)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("pmtsa_getMeasurementTypeDescription is being called");
	MafReturnT retVal = PmShowCounters::instance()->getMeasurementTypeDescription(measurementType, groupIds,description);
	LEAVE_PMTSA();
	return retVal;
}

MafReturnT pmtsa_getGroupIdDescription(const char* groupId, ComOamSpiPmGroupIdDescription_4T* description)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("pmtsa_getGroupIdDescription is being called");
	MafReturnT retVal = PmShowCounters::instance()->getGroupIdDescription(groupId, description);
	LEAVE_PMTSA();
	return retVal;
}

void releaseMeasuredObject(struct ComOamSpiPmMeasuredObject_3* measuredObject)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("releaseMeasuredObject is being called");

	if(!measuredObject->release)
	{
		PMTSA_DEBUG("releaseMeasuredObject: measuredObject->release is NULL");
	}
	else
	{
		if(measuredObject->measurements)
		{
			ComOamSpiPmMeasurement_3T* tempMeas= measuredObject->measurements;
			uint32_t i = 0;
			while(i < measuredObject->nrOfMeasurements)
			{
				if(tempMeas[i].value)
				{
					delete[] tempMeas[i].value;
					tempMeas[i].value = NULL;
				}
				if(tempMeas[i].errorInformation)
				{
					free((void*)tempMeas[i].errorInformation);
					tempMeas[i].errorInformation = NULL;
				}
				if(tempMeas[i].jobId)
				{
					free((void*)tempMeas[i].jobId);
					tempMeas[i].jobId = NULL;
				}
				if(tempMeas[i].name)
				{
					free((void*)tempMeas[i].name);
					tempMeas[i].name = NULL;
				}
				i++;
			}
			tempMeas->gpInSeconds = 0;
			tempMeas->nrOfValues = 0;

			delete [] measuredObject->measurements;
			measuredObject->measurements = NULL;
		}
		measuredObject->nrOfMeasurements = 0;
		measuredObject->release = NULL;
	}

	LEAVE_PMTSA();
	return;
}

void releaseMeasurementOutput(ComOamSpiPmMeasurementOutput_4T* measurementOutput)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("releaseMeasurementOutput is being called");
	if (measurementOutput)
	{
		if(measurementOutput->names)
		{
			unsigned int nrOfValues = 0;
			try {
				while(NULL != measurementOutput->names[nrOfValues++]) {}
			}
			catch (...) {
				PMTSA_DEBUG("releaseMeasurementOutput Failed to release. unable to iterate through measurementOutput->names[].");
				return;
			}
			for(unsigned int i = 0; i < (nrOfValues-1); i++){
				free((void*)measurementOutput->names[i]);
				measurementOutput->names[i] = NULL;
			}
			delete[] measurementOutput->names;
			measurementOutput->names = NULL;
			measurementOutput->release = NULL;
		}
	}
	LEAVE_PMTSA();
}

//This function releases the memory allocated to output container of SPI call.
void releaseMeasuredObjectMeasurements(ComOamSpiPmMeasuredObjectMeasurements_4T* measObjMeasurements)
{
	if(NULL != measObjMeasurements) {
		unsigned int nrOfMeasurements = measObjMeasurements->nrOfMeasurements;

		if (NULL != measObjMeasurements->measurements) {
			if (nrOfMeasurements)
			{
				unsigned int mIndex = 0;
				while (mIndex < nrOfMeasurements)
				{
					if (NULL != measObjMeasurements->measurements[mIndex].measurementTypeName) {
						free((void*)measObjMeasurements->measurements[mIndex].measurementTypeName);
						measObjMeasurements->measurements[mIndex].measurementTypeName = NULL;
					}

					if (NULL != measObjMeasurements->measurements[mIndex].measuredObject) {
						free((void*)measObjMeasurements->measurements[mIndex].measuredObject);
						measObjMeasurements->measurements[mIndex].measuredObject = NULL;
					}

					if (NULL != measObjMeasurements->measurements[mIndex].jobId) {
						free((void*)measObjMeasurements->measurements[mIndex].jobId);
						measObjMeasurements->measurements[mIndex].jobId = NULL;
					}

					if (NULL != measObjMeasurements->measurements[mIndex].groupId) {
						free((void*)measObjMeasurements->measurements[mIndex].groupId);
						measObjMeasurements->measurements[mIndex].groupId = NULL;
					}

					measObjMeasurements->measurements[mIndex].nrOfValues = 0;
					if (NULL != measObjMeasurements->measurements[mIndex].value) {
						delete[] measObjMeasurements->measurements[mIndex].value;
						measObjMeasurements->measurements[mIndex].value = NULL;
					}

					if (NULL != measObjMeasurements->measurements[mIndex].errorInformation) {
						free((void*)measObjMeasurements->measurements[mIndex].errorInformation);
						measObjMeasurements->measurements[mIndex].errorInformation = NULL;
					}
					++mIndex;
				}
				measObjMeasurements->nrOfMeasurements = 0;
			}
			delete[] measObjMeasurements->measurements;
			measObjMeasurements->measurements = NULL;
		}
		measObjMeasurements->release = NULL;
	}
}

void releaseGroupIdDescription(ComOamSpiPmGroupIdDescription_4* description)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("releaseGroupIdDescription is being called");

	if(description) {
		if(description->description && (strcmp(description->description,"") != 0)) {
			free((void*)description->description);
		}
		description->description = NULL;
	}
	description->release = NULL;

	LEAVE_PMTSA();
	return;
}

void releaseMeasurementTypeDescription(ComOamSpiPmMeasurementTypeDescription_4* description)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("releaseMeasurementTypeDescription is being called");
	if(description){
		if(description->measurementTypeInfo){
			uint32_t i = 0;
			while( i < description->nrOfValues) {
				if(description->measurementTypeInfo[i].description && (strcmp(description->measurementTypeInfo[i].description,"")!= 0)) {
					free((void*)description->measurementTypeInfo[i].description);
				}
				description->measurementTypeInfo[i].description = NULL;
				if(description->measurementTypeInfo[i].groupId) {
					free((void*)description->measurementTypeInfo[i].groupId);
					description->measurementTypeInfo[i].groupId = NULL;
				}
				i++;
			}
			delete[] description->measurementTypeInfo;
			description->measurementTypeInfo = NULL;
			description->nrOfValues = 0;
			description->release = NULL;
		}
	}
	LEAVE_PMTSA();
	return;
}

void releaseSaConstStringArray(SaConstStringT* array)
{
	if(array)
	{
		uint32_t i = 0;
		while(array[i])
		{
			free((void*)array[i]);
			array[i] = NULL;
			i++;
		}
		delete [] array;
		array = NULL;
	}
}

void releaseSaConstString(SaConstStringT string)
{
	if(NULL != string) {
		try {
			free((void*)string);
		}
		catch (...) {
			PMTSA_DEBUG("releaseSaConstString unable to release memory");
		}
		string = NULL;
	}
}

std::string PmShowCounters::getDescription(SaImmAttrValuesT_2** attributes)
{
	if((attributes == NULL) || (attributes[0] == NULL)) {
		PMTSA_DEBUG("PmShowCounters::getDescription attributes is NULL.");
	} else {
		if(attributes[0]->attrValuesNumber != 0){
			if (attributes[0]->attrValueType == SA_IMM_ATTR_SASTRINGT){
				std::string attrValue(*(char**)(attributes[0]->attrValues[0]));
				LEAVE_PMTSA();
				return attrValue;
			} else {
				PMTSA_DEBUG("PmShowCounters::getDescription datatype of received attribute didn't match.");
			}
		} else {
			PMTSA_DEBUG("PmShowCounters::getDescription number of attributes received is zero.");
		}
	}
	LEAVE_PMTSA();
	return "";
}

std::string PmShowCounters::getCompleteDn(const char* measurementTypeValue, const char* pmGroupIdValue)
{
	//If input is CcMT-1 and CcGroup1
	//Output value is measurementTypeId=CcMT-1,pmGroupId=CcGroup1,CmwPmpmId=1
	ENTER_PMTSA();
	std::string measurementTypeComplete = MEASUREMENT_TYPE_NAME + "=" + measurementTypeValue;
	std::string pmGroupIdComplete = PM_GROUPID_NAME + "=" + pmGroupIdValue;
	std::string completeDn = measurementTypeComplete + "," + pmGroupIdComplete + "," + PM_ID;
	LEAVE_PMTSA();
	return completeDn;
}

std::string PmShowCounters::getGroupIdValue(std::string objectName)
{
	// If input is measurementTypeId=CcMT-1,pmGroupId=CcGroup1,CmwPmpmId=1
	// output will be CcGroup1
	ENTER_PMTSA();
	std::size_t pos_groupId = objectName.find("pmGroupId=");
	if(pos_groupId != std::string::npos) {
		std::string part_objectName = objectName.substr(pos_groupId);
		std::size_t lastpos_groupId = part_objectName.find_first_of(",");
		if(lastpos_groupId != std::string::npos) {
			std::string complete_groupId = part_objectName.substr(0,lastpos_groupId);
			std::size_t pos_equal = complete_groupId.find("=");
			if(pos_equal != std::string::npos) {
				std::string groupId_value = complete_groupId.substr(pos_equal+1);
				LEAVE_PMTSA();
				return groupId_value;
			}
		} else {
			LEAVE_PMTSA();
			return "";
		}
	} else {
		LEAVE_PMTSA();
		return "";
	}
	LEAVE_PMTSA();
	return "";
}
