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
* File: PmConsumerInterface.cxx
*
* Author: ejonajo 2011-09-01
*
* Reviewed: efaiami 2012-04-14
* Modified: xtronle 2013-12-25 Support PDF counter
* Modified: xtronle 2015-09-15 MR36219 PM counters to support troubleshooting
*
************************************************************************** */
#include "PmConsumerInterface.hxx"
#include "PmtSaTrace.hxx"

#include <cstddef> // NULL
#include <iostream>

/**
* @file PmConsumerInterface.cxx
*
* @brief Implementation of the PM Consumer interface
*
* The object tries upon creation to load the dynamic library (PMC_LibraryName)
* to fetch the different methods required in order to provide service. Upon
* success of loading ALL required methods, the state is set to OK, but if one
* single interface fails during load, the whole load-operation is cancelled
* and marked as failed.
*
* Once we've managed to load all interfaces, they will remain loaded until the
* object is destroyed.
*/

PmConsumerInterface* PmConsumerInterface::s_instance = NULL;

PmConsumerInterface::PmConsumerInterface() :
		m_foundSaPmConsumer(false),
		m_dlHandle(NULL),
		m_showCountersFeatureV2(UNASSIGNED),
		PMC_LibraryName("libSaPmConsumer.so.0"),
		PMC_Initialize("saPmCInitialize"),
		PMC_Initialize_2("saPmCInitialize_2"),
		PMC_Finalize("saPmCFinalize"),
		PMC_Activate("saPmCActivate"),
		PMC_Deactivate("saPmCDeactivate"),
		PMC_SelectionObjectGet("saPmCSelectionObjectGet"),
		PMC_Dispatch("saPmCDispatch"),
		PMC_IteratorInitialize("saPmCIteratorInitialize"),
		PMC_IteratorInitialize_2("saPmCIteratorInitialize_2"),
		PMC_IteratorNext("saPmCIteratorNext"),
		PMC_IteratorNext_2("saPmCIteratorNext_2"),
		PMC_IteratorFinalize("saPmCIteratorFinalize"),
		PMC_IteratorFinalize_2("saPmCIteratorFinalize_2"),
		PMC_JobGPDataRemove("saPmCJobGranularityPeriodDataRemove"),
		PMC_CurrentValuesGet("saPmCCurrentValuesGet"),
		PMC_GetInstanceValues("saPmCGetInstanceValues"),
		PMC_InstanceValuesMemoryFree("saPmCInstanceValuesMemoryFree"),
		PMC_IsFeatureEnabled("saPmCIsFeatureEnabled"),
		PMC_ShowCountersFeatureName("SAPMC_FEATURE_SHOW_COUNTERS"),
		PMC_ShowCountersVersionName("SAPMC_VERSION_2"),
		PMC_NamesMemoryFree("saPmCNamesMemoryFree"),
		PMC_GetInstanceValues_2("saPmCGetInstanceValues_2"),
		PMC_InstanceValuesMemoryFree_2("saPmCInstanceValuesMemoryFree_2"),
		PMC_CancelGetInstanceValues_2("saPmCCancelGetInstanceValues_2"),
		PMC_GetMeasurementTypeNames("saPmCGetMeasurementTypeNames"),
		PMC_GetPmGroupNames("saPmCGetPmGroupNames"),
		PMC_GetInstanceNames("saPmCGetInstanceNames"),
		saPmCInitialize_dl(NULL),
		saPmCInitialize_2_dl(NULL),
		saPmCFinalize_dl(NULL),
		saPmCActivate_dl(NULL),
		saPmCDeactivate_dl(NULL),
		saPmCSelectionObjectGet_dl(NULL),
		saPmCDispatch_dl(NULL),
		saPmCIteratorInitialize_dl(NULL),
		saPmCIteratorInitialize_2_dl(NULL),
		saPmCIteratorNext_dl(NULL),
		saPmCIteratorNext_2_dl(NULL),
		saPmCIteratorFinalize_dl(NULL),
		saPmCIteratorFinalize_2_dl(NULL),
		saPmCJobGranularityPeriodDataRemove_dl(NULL),
		saPmCCurrentValuesGet_dl(NULL),
		saPmCGetInstanceValues_dl(NULL),
		saPmCInstanceValuesMemoryFree_dl(NULL),
		saPmCIsFeatureEnabled_dl(NULL),
		saPmCNamesMemoryFree_dl(NULL),
		saPmCGetInstanceValues_2_dl(NULL),
		saPmCInstanceValuesMemoryFree_2_dl(NULL),
		saPmCCancelGetInstanceValues_2_dl(NULL),
		saPmCGetMeasurementTypeNames_dl(NULL),
		saPmCGetPmGroupNames_dl(NULL),
		saPmCGetInstanceNames_dl(NULL)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::PmConsumerInterface() called");
	if (s_instance == NULL)
	{
		s_instance = this;
		m_foundSaPmConsumer = loadSaPmConsumer();
	}
	LEAVE_PMTSA();
}

PmConsumerInterface::~PmConsumerInterface()
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::~PmConsumerInterface() called");
	if (m_dlHandle != NULL)
	{
		dlclose(m_dlHandle);
		m_dlHandle = NULL;
	}
	m_showCountersFeatureV2 = UNASSIGNED;
	m_foundSaPmConsumer = false;
	setDynamicInterfacesNull();
	s_instance = NULL;
	LEAVE_PMTSA();
}

PmConsumerInterface* PmConsumerInterface::instance()
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::instance() called");
	if (s_instance == NULL)
	{
		s_instance = new PmConsumerInterface();
	}
	LEAVE_PMTSA();
	return s_instance;
}

/**
* Make sure all foreign interfaces are NULL.
*/
void PmConsumerInterface::setDynamicInterfacesNull()
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::setDynamicInterfacesNull() called");
	saPmCInitialize_dl = NULL;
	saPmCInitialize_2_dl = NULL;
	saPmCFinalize_dl = NULL;
	saPmCActivate_dl = NULL;
	saPmCDeactivate_dl = NULL;
	saPmCSelectionObjectGet_dl = NULL;
	saPmCDispatch_dl = NULL;
	saPmCIteratorInitialize_dl = NULL;
	saPmCIteratorInitialize_2_dl = NULL;
	saPmCIteratorNext_dl = NULL;
	saPmCIteratorNext_2_dl = NULL;
	saPmCIteratorFinalize_dl = NULL;
	saPmCIteratorFinalize_2_dl = NULL;
	saPmCJobGranularityPeriodDataRemove_dl = NULL;
	saPmCCurrentValuesGet_dl = NULL;
	saPmCGetInstanceValues_dl = NULL;
	saPmCInstanceValuesMemoryFree_dl = NULL;
	saPmCIsFeatureEnabled_dl = NULL;
	saPmCNamesMemoryFree_dl = NULL;
	saPmCGetInstanceValues_2_dl = NULL;
	saPmCInstanceValuesMemoryFree_2_dl = NULL;
	saPmCCancelGetInstanceValues_2_dl = NULL;
	saPmCGetMeasurementTypeNames_dl = NULL;
	saPmCGetPmGroupNames_dl = NULL;
	saPmCGetInstanceNames_dl = NULL;

	LEAVE_PMTSA();
}

bool PmConsumerInterface::loadSaPmConsumer()
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::loadSaPmConsumer() called");
	if (m_dlHandle != NULL)
	{
		LEAVE_PMTSA();
		return m_foundSaPmConsumer;
	}
	m_dlHandle = dlopen(PMC_LibraryName.c_str(), RTLD_LAZY);
	if (!m_dlHandle)
	{
		m_foundSaPmConsumer = false;
		PMTSA_WARN("Failed loading %s, dlerror() reports %s", PMC_LibraryName.c_str(), dlerror());
		setDynamicInterfacesNull();
		LEAVE_PMTSA();
		return m_foundSaPmConsumer;
	}

	// Reset errors
	dlerror();
	// Start loading interfaces, if ANY fails, we should
	// close the dl-handle, clear all ptrs and return false.
	saPmCInitialize_dl = loadSymName<saPmCInitialize_t>(PMC_Initialize);
	saPmCInitialize_2_dl = loadSymName<saPmCInitialize_2_t>(PMC_Initialize_2);
	saPmCFinalize_dl = loadSymName<saPmCFinalize_t>(PMC_Finalize);
	saPmCActivate_dl = loadSymName<saPmCActivate_t>(PMC_Activate);
	saPmCDeactivate_dl = loadSymName<saPmCDeactivate_t>(PMC_Deactivate);
	saPmCSelectionObjectGet_dl = loadSymName<saPmCSelectionObjectGet_t>(PMC_SelectionObjectGet);
	saPmCDispatch_dl = loadSymName<saPmCDispatch_t>(PMC_Dispatch);
	saPmCIteratorInitialize_dl = loadSymName<saPmCIteratorInitialize_t>(PMC_IteratorInitialize);
	saPmCIteratorInitialize_2_dl = loadSymName<saPmCIteratorInitialize_2_t>(PMC_IteratorInitialize_2);
	saPmCIteratorNext_dl = loadSymName<saPmCIteratorNext_t>(PMC_IteratorNext);
	saPmCIteratorNext_2_dl = loadSymName<saPmCIteratorNext_2_t>(PMC_IteratorNext_2);
	saPmCIteratorFinalize_dl = loadSymName<saPmCIteratorFinalize_t>(PMC_IteratorFinalize);
	saPmCIteratorFinalize_2_dl = loadSymName<saPmCIteratorFinalize_2_t>(PMC_IteratorFinalize_2);
	saPmCJobGranularityPeriodDataRemove_dl = loadSymName<saPmCJobGranularityPeriodDataRemove_t>(PMC_JobGPDataRemove);
	saPmCCurrentValuesGet_dl = loadSymName<saPmCCurrentValuesGet_t>(PMC_CurrentValuesGet);
	saPmCIsFeatureEnabled_dl = loadSymName<saPmCIsFeatureEnabled_t>(PMC_IsFeatureEnabled);

	if(isShowCounterV2Enabled())
	{
		saPmCGetInstanceValues_2_dl         = loadSymName<saPmCGetInstanceValues_2_t>(PMC_GetInstanceValues_2);
		saPmCInstanceValuesMemoryFree_2_dl  = loadSymName<saPmCInstanceValuesMemoryFree_2_t>(PMC_InstanceValuesMemoryFree_2);
		saPmCNamesMemoryFree_dl             = loadSymName<saPmCNamesMemoryFree_t>(PMC_NamesMemoryFree);
		saPmCCancelGetInstanceValues_2_dl         = loadSymName<saPmCCancelGetInstanceValues_2_t>(PMC_CancelGetInstanceValues_2);
		saPmCGetMeasurementTypeNames_dl     = loadSymName<saPmCGetMeasurementTypeNames_t>(PMC_GetMeasurementTypeNames);
		saPmCGetPmGroupNames_dl             =loadSymName<saPmCGetPmGroupNames_t>(PMC_GetPmGroupNames);
		saPmCGetInstanceNames_dl            = loadSymName<saPmCGetInstanceNames_t>(PMC_GetInstanceNames);
	}
	else
	{
		saPmCGetInstanceValues_dl = loadSymName<saPmCGetInstanceValues_t>(PMC_GetInstanceValues);
		saPmCInstanceValuesMemoryFree_dl = loadSymName<saPmCInstanceValuesMemoryFree_t>(PMC_InstanceValuesMemoryFree);
	}
	m_foundSaPmConsumer = true;
	LEAVE_PMTSA();
	return m_foundSaPmConsumer;
}

SaAisErrorT PmConsumerInterface::initialize(SaPmCHandleT* pmHandle,
						SaVersionT* version,
						SaPmfJobTypeT jobType,
						SaPmCJobDataOrganizationT option,
						SaPmCSubscribeInfoT* subscribeInfo)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::initialize() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCInitialize_dl(pmHandle, version, jobType, option, subscribeInfo);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::initialize_2(SaPmCHandleT* pmHandle,
						SaVersionT* version,
						SaPmfJobTypeT jobType,
						SaPmCJobDataOrganizationT option,
						SaPmCSubscribeInfoT_2* subscribeInfo)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::initialize_2() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCInitialize_2_dl(pmHandle, version, jobType, option, subscribeInfo);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::finalize(SaPmCHandleT pmHandle)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::finalize() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCFinalize_dl(pmHandle);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::activate(SaPmCHandleT pmHandle)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::activate() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCActivate_dl(pmHandle);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::deactivate(SaPmCHandleT pmHandle)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::deactivate() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCDeactivate_dl(pmHandle);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::selectionObjectGet(SaPmCHandleT pmHandle, SaSelectionObjectT *selfd)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::selectionObjectGet() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCSelectionObjectGet_dl(pmHandle, selfd);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::dispatch(SaPmCHandleT pmHandle, SaDispatchFlagsT flags)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::dispatch() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCDispatch_dl(pmHandle, flags);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::iteratorInitialize(SaPmCJobGranularityPeriodDataT hdl,
							SaPmCIteratorHandleT *iteratorHandle)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::iteratorInitialize() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCIteratorInitialize_dl(hdl, iteratorHandle);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::iteratorInitialize_2(SaPmCJobGranularityPeriodDataT hdl,
							SaPmCIteratorHandleT_2 *iteratorHandle)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::iteratorInitialize_2() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCIteratorInitialize_2_dl(hdl, iteratorHandle);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::iteratorNext(SaPmCIteratorHandleT iteratorHandle,
						SaPmCIteratorInfoT *iteratorInfo)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::iteratorNext() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCIteratorNext_dl(iteratorHandle, iteratorInfo);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::iteratorNext_2(SaPmCIteratorHandleT_2 iteratorHandle,
						SaPmCIteratorInfoT_2 *iteratorInfo,
						SaPmCAggregatedValuesT *values)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::iteratorNext_2() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCIteratorNext_2_dl(iteratorHandle, iteratorInfo, values);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::iteratorFinalize(SaPmCIteratorHandleT iteratorHandle)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::iteratorFinalize() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCIteratorFinalize_dl(iteratorHandle);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::iteratorFinalize_2(SaPmCIteratorHandleT_2 iteratorHandle)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::iteratorFinalize_2() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCIteratorFinalize_2_dl(iteratorHandle);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::jobGranularityPeriodDataRemove(SaPmCJobGranularityPeriodDataT hdl)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::jobGranularityPeriodDataRemove() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCJobGranularityPeriodDataRemove_dl(hdl);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::saPmCCurrentValuesGet(SaPmCHandleT pmHandle)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::saPmCCurrentValuesGet() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCCurrentValuesGet_dl(pmHandle);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::saPmCGetInstanceValues(SaPmCHandleT pmHandle,
							SaConstStringT instance,
							SaConstStringT *jobNames,
							SaConstStringT *measurementTypes,
							SaTimeT timeout,
							SaPmCInstanceValuesT ***instanceValues)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::saPmCGetInstanceValues() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCGetInstanceValues_dl(pmHandle, instance, jobNames, measurementTypes, timeout, instanceValues);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::saPmCInstanceValuesMemoryFree(SaPmCHandleT pmHandle, SaPmCInstanceValuesT **instanceValues)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::saPmCInstanceValuesMemoryFree() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCInstanceValuesMemoryFree_dl(pmHandle, instanceValues);
	}
	LEAVE_PMTSA();
	return res;
}

bool PmConsumerInterface::isShowCounterV2Enabled()
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::isFeatureEnabled() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (UNASSIGNED == m_showCountersFeatureV2)
	{
		SaBoolT feature = SA_FALSE;
		res = saPmCIsFeatureEnabled(&feature);
		if (SA_AIS_OK == res)
		{
			if (SA_TRUE == feature){
				m_showCountersFeatureV2 = ENABLED;
				PMTSA_DEBUG("PmConsumerInterface::saPmCIsFeatureEnabled Optional CoreMW PM API features are enabled.");
				LEAVE_PMTSA();
				return true;
			}
		}
		m_showCountersFeatureV2 = DISABLED;
		PMTSA_DEBUG("PmConsumerInterface::saPmCIsFeatureEnabled Optional CoreMW PM API features are not available. res:%d", res);
	}
	LEAVE_PMTSA();
	return (ENABLED == m_showCountersFeatureV2);
}

SaAisErrorT PmConsumerInterface::saPmCIsFeatureEnabled(SaBoolT *enabled)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::saPmCIsFeatureEnabled() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;;
	if (NULL != saPmCIsFeatureEnabled_dl)
	{
		SaConstStringT* tempFeatureName    = reinterpret_cast<SaConstStringT*>(dlsym(m_dlHandle, PMC_ShowCountersFeatureName.c_str()));
		SaConstStringT* tempFeatureVersion = reinterpret_cast<SaConstStringT*>(dlsym(m_dlHandle, PMC_ShowCountersVersionName.c_str()));
		if (dlerror() || !tempFeatureName || !tempFeatureVersion) {
			PMTSA_DEBUG("PmConsumerInterface::saPmCIsFeatureEnabled Unable to load SAPMC_FEATURE_SHOW_COUNTERS/SAPMC_VERSION_2 symbols from dl");
			LEAVE_PMTSA();
			return res;
		}
		SaPmCHandleT osafPmHandle;
		SaVersionT version = {'A', 3, 1};
		SaPmfJobTypeT jobType = SA_PMF_JOB_TYPE_MEASUREMENT;
		SaPmCJobDataOrganizationT option = PMC_JOB_DATA_ORGANIZATION_MOCLASS_MEASOBJ;
		SaPmCSubscribeInfoT_2 subscribeInfo;
		subscribeInfo.rmInfo.clientHdlr = 0;
		subscribeInfo.rmInfo.cbPtr = NULL;
		res = saPmCInitialize_2_dl(&osafPmHandle, &version, jobType, option, &subscribeInfo);
		PMTSA_DEBUG("PmConsumerInterface::saPmCIsFeatureEnabled SaVersionT returned from saPmCInitialize_2 is releaseCode[%c], majorVersion[%d], minorVersion[%d]",
				version.releaseCode, version.majorVersion, version.minorVersion);
		if (SA_AIS_OK == res)
		{
			res = saPmCIsFeatureEnabled_dl(osafPmHandle, *tempFeatureName, *tempFeatureVersion, enabled);
			if (SA_AIS_OK != res) {
				PMTSA_DEBUG("PmConsumerInterface::saPmCIsFeatureEnabled() failed saPmCIsFeatureEnabled_dl=%d", res);
			}
			(void)saPmCFinalize_dl(osafPmHandle);
		}
		else {
			PMTSA_DEBUG("PmConsumerInterface::saPmCIsFeatureEnabled() failed saPmCInitialize_2_dl=%d", res);
		}
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::saPmCGetInstanceValues_2(
			SaPmCHandleT pmHandle,
			SaConstStringT instance,
			SaConstStringT *measurementTypes,
			SaConstStringT *pmGroups,
			SaTimeT timeout,
			SaUint32T *requestId,
			SaPmCInstanceValuesT_2 ***instanceValues,
			SaConstStringT *errorText)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::saPmCGetInstanceValues_2() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCGetInstanceValues_2_dl(pmHandle, instance, measurementTypes, pmGroups, timeout, requestId, instanceValues, errorText);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::saPmCInstanceValuesMemoryFree_2(
			SaPmCHandleT pmHandle,
			SaPmCInstanceValuesT_2 **instanceValues,
			SaConstStringT errorText)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::saPmCInstanceValuesMemoryFree_2() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCInstanceValuesMemoryFree_2_dl(pmHandle, instanceValues, errorText);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::saPmCNamesMemoryFree(SaPmCHandleT pmHandle, SaConstStringT *names)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::saPmCNamesMemoryFree() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCNamesMemoryFree_dl(pmHandle, names);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::saPmCCancelGetInstanceValues_2(
		SaPmCHandleT pmHandle,
		SaUint32T requestId)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::saPmCCancel() called");
	SaAisErrorT res = saPmCCancelGetInstanceValues_2_dl(pmHandle, requestId);
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::saPmCGetMeasurementTypeNames(
		SaPmCHandleT pmHandle,
		SaConstStringT instance,
		SaConstStringT measurementType,
		SaConstStringT *pmGroups,
		SaTimeT timeout,
		SaConstStringT **measurementTypeNames)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::saPmCGetMeasurementTypeNames() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCGetMeasurementTypeNames_dl(pmHandle, instance, measurementType, pmGroups, timeout, measurementTypeNames);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::saPmCGetPmGroupNames(
				SaPmCHandleT pmHandle,
				SaConstStringT instances,
				SaConstStringT *measurementTypes,
				SaConstStringT pmGroup,
				SaTimeT timeout,
				SaConstStringT **pmGroupNames)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::saPmCGetPmGroupNames() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCGetPmGroupNames_dl(pmHandle, instances, measurementTypes, pmGroup, timeout, pmGroupNames);
	}
	LEAVE_PMTSA();
	return res;
}

SaAisErrorT PmConsumerInterface::saPmCGetInstanceNames(
		SaPmCHandleT pmHandle,
		SaConstStringT instance,
		SaConstStringT *measurementTypes,
		SaConstStringT *pmGroups,
		SaTimeT timeout,
		SaConstStringT **instanceNames)
{
	ENTER_PMTSA();
	PMTSA_DEBUG("PmConsumerInterface::saPmCGetInstanceNames() called");
	SaAisErrorT res = SA_AIS_ERR_UNAVAILABLE;
	if (m_foundSaPmConsumer)
	{
		res = saPmCGetInstanceNames_dl(pmHandle, instance, measurementTypes, pmGroups, timeout, instanceNames);
	}
	LEAVE_PMTSA();
	return res;
}
