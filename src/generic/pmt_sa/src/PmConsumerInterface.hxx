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
* File: PmConsumerInterface.hxx
*
* Author: ejonajo 2011-09-01
*
* Reviewed: efaiami 2012-04-14
* Modified: xtronle 2013-12-25 Support PDF counter
* Modified: xtronle 2015-09-15 MR36219 PM counters to support troubleshooting
*
************************************************************************** */
#ifndef PMCONSUMERINTERFACE_HXX_
#define PMCONSUMERINTERFACE_HXX_

#include "PmtSaTrace.hxx"
#include <saPmConsumer.h>
#include <string>
#include <iostream>
#include <dlfcn.h>

/**
* @file PmConsumerInterface.hxx
*
* @brief Defines an iterator-like interface to PM-data.
*
* This interface is analogue with the methods provided by saPmConsumer.h
* but they're wrapped into a C++ interface.
*/

/**
* @brief Loads the PM Consumer Interfaces.
*
* Wrapper-class for the OsafPM consumer interface. Notice that if the
* initialization of this class fails, all interfaces will return error.
*/
class PmConsumerInterface
{
public:

	/**
	* Constructor, makes sure the instance-pointer is set, and also
	* loads the dynamic library for the real PM C-functions. If the
	* loading fails, all methods will return an error-code.  It's
	* always possible to check the state with foundSaPmConsumer().
	*/
	PmConsumerInterface();

	/**
	* Destructor, make sure the instance-pointer is set to NULL
	*/
	virtual ~PmConsumerInterface();

	/**
	* Used for retrieving a pointer to the interface
	*
	* @return 	PmConsumerInterface*    Pointer to the interface
	*
	*/
	static PmConsumerInterface* instance();

	/**
	* Used for connecting to the Osaf PM Services.  Notice that the
	* actual consumption cannot start before the activate() call has
	* been done.
	*
	* @param[out] 		pmHandle    Handle to the PM consumer service
	* @param[in,out]	version     PM consumer version requested in the way in
	*                              and service version provided in return
	* @param[in] 	    jobType     What kind of job-type we're looking for, typically SA_PMF_JOB_TYPE_MEASUREMENT
	* @param[in]       option      Currently PMC_JOB_DATA_ORGANIZATION_MOCLASS_MEASOBJ
	* @param[in]   subscribeInfo   Callback-info for the PM-service etc.
	*
	* @return 	SaAisErrorT			Result of operation
	*
	*/
	virtual SaAisErrorT initialize(SaPmCHandleT* pmHandle,
		SaVersionT* version, SaPmfJobTypeT jobType,
		SaPmCJobDataOrganizationT option, SaPmCSubscribeInfoT* subscribeInfo);

	/**
	* Used for connecting to the Osaf PM Services.  Notice that the
	* actual consumption cannot start before the activate() call has
	* been done.
	*
	* @param[out] 		pmHandle    Handle to the PM consumer service
	* @param[in,out]	version     PM consumer version requested in the way in
	*                              and service version provided in return
	* @param[in] 	    jobType     What kind of job-type we're looking for, typically PMSV_JOB_TYPE_MEASUREMENT_JOB & PMSV_JOB_TYPE_RESOURCEMONITOR_JOB
	* @param[in]       option      Currently PMC_JOB_DATA_ORGANIZATION_MOCLASS_MEASOBJ
	* @param[in]   subscribeInfo   Callback-info for the PM-service etc.
	*
	* @return 	SaAisErrorT			Result of operation
	*
	*/
	virtual SaAisErrorT initialize_2(SaPmCHandleT* pmHandle,
	SaVersionT* version, SaPmfJobTypeT jobType,
	SaPmCJobDataOrganizationT option, SaPmCSubscribeInfoT_2* subscribeInfo);

	/**
	* Close and remove the "current" job received from the dispatch() method
	*
	* @param[in] 		pmHandle    Handle to the PM consumer service
	*
	* @return  SaAisErrorT         Result of operation
	*
	*/
	virtual SaAisErrorT finalize(SaPmCHandleT pmHandle);

	/**
	* Activate the actual aggregation of PM data on the system.  Before this method
	* is called, no data is actually collected from the system, since the PM services
	* should be able to start up without actually doing some work (ie on a standby node).
	*
	* @param[in]       pmHandle    Handle to the PM consumer service
	*
	* @return  SaAisErrorT         Result of operation
	*/
	virtual SaAisErrorT activate(SaPmCHandleT pmHandle);

	/**
	* De-activate the aggregation of PM data on the system.  Once this method
	* is called, no data is actually collected from the system, since the PM services
	* should be able to run without actually doing some work (ie on a standby node).
	*
	* @param[in]       pmHandle    Handle to the PM consumer service
	*
	* @return  SaAisErrorT         Result of operation
	*/
	virtual SaAisErrorT deactivate(SaPmCHandleT pmHandle);

	/**
	* Get a file descriptor from the PM services so that we can fetch data
	* once some data is available.
	*
	* @param[in] 		pmHandle    Handle to the PM consumer service
	* @param[in,out] 	selfd       Pointer to a file descriptor
	* @return  SaAisErrorT         Result of operation
	*/
	virtual SaAisErrorT selectionObjectGet(SaPmCHandleT pmHandle, SaSelectionObjectT *selfd);

	/**
	* Once we get an event on the file descriptor (@see selectionObjectGet), this method
	* should be called.  That will invoke the callback on the method specified
	*
	* @param[in] 		pmHandle    Handle to the PM consumer service
	* @param[in] 		flags       What action should be taken
	*
	* @return  SaAisErrorT         Result of operation
	*/
	virtual SaAisErrorT dispatch(SaPmCHandleT pmHandle, SaDispatchFlagsT flags);

	/**
	* Fetch data for one PM-job.
	*
	* @param[in] 		hdl             Handle to PM-job
	* @param[out] 		iteratorHandle  Handle to iterator
	*
	* @return 	SaAisErrorT				Result of operation
	*
	*/
	virtual SaAisErrorT iteratorInitialize(
		SaPmCJobGranularityPeriodDataT hdl,
		SaPmCIteratorHandleT *iteratorHandle
	);
	/**
	* Fetch data for one PM-job.
	*
	* @param[in] 		hdl             Handle to PM-job
	* @param[out] 		iteratorHandle  Handle to iterator
	*
	* @return 	SaAisErrorT				Result of operation
	*
	*/
	virtual SaAisErrorT iteratorInitialize_2(
		SaPmCJobGranularityPeriodDataT hdl,
		SaPmCIteratorHandleT_2 *iteratorHandle
	);

	/**
	* Fetch more ('next') data for one PM-job.
	*
	* @param[in]       iteratorHandle  Handle to iterator
	* @param[in,out]   iteratorInfo    Information about current job
	*
	* @return  SaAisErrorT             Result of operation
	*
	*/
	virtual SaAisErrorT iteratorNext(
		SaPmCIteratorHandleT iteratorHandle,
		SaPmCIteratorInfoT *iteratorInfo
	);

	/**
	* Fetch more ('next') data for one PM-job.
	*
	* @param[in]       iteratorHandle  Handle to iterator
	* @param[in,out]   iteratorInfo    Information about current job
	* @param[out]      values          Returned values
	*
	* @return  SaAisErrorT             Result of operation
	*
	*/

	virtual SaAisErrorT iteratorNext_2(
		SaPmCIteratorHandleT_2 iteratorHandle,
		SaPmCIteratorInfoT_2 *iteratorInfo,
		SaPmCAggregatedValuesT *values
	);

	/**
	* Finalize use of fetched iterator.
	*
	* @param[in] 		iteratorHandle      Handle to an iterator
	*
	* @return 	SaAisErrorT     Result of operation
	*
	*/
	virtual SaAisErrorT iteratorFinalize(SaPmCIteratorHandleT iteratorHandle);

	/**
	* Finalize use of fetched iterator.
	*
	* @param[in] 		iteratorHandle      Handle to an iterator
	*
	* @return 	SaAisErrorT     Result of operation
	*
	*/
	virtual SaAisErrorT iteratorFinalize_2(SaPmCIteratorHandleT_2 iteratorHandle);

	/**
	* Remove all data associated with one granularity period.
	*
	* @param[in] 		hdl     Handle to one PM gp id.
	*
	* @return 	SaAisErrorT     Result of operation
	*
	*/
	virtual SaAisErrorT jobGranularityPeriodDataRemove(
		SaPmCJobGranularityPeriodDataT hdl
	);

	/**
	* Reads current, non-GP aligned, local counter values.
	*
	* @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	*
	* @return SaAisErrorT
	*/
	virtual SaAisErrorT saPmCCurrentValuesGet(SaPmCHandleT pmHandle);

	/*
	* Read current, non-GP aligned, values of a measurement instance.
	*
	* @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	* @param instance : [in] instance name
	* @param jobNames : [in] pointer to a NULL-terminated array of job names.
	* @param measurementTypes : [in] pointer to a NULL-terminated array measurement type names.
	* @param timeout : [in] The saPmCGetInstanceValues() invocation is considered to have failed
	* if it does not complete by the time specified.
	* @param instanceValues : [out] pointer to a pointer to NULL-terminated array of pointer to instance values.
	*
	* @return SaAisErrorT
	*/
	virtual SaAisErrorT saPmCGetInstanceValues(SaPmCHandleT pmHandle,
							SaConstStringT instance,
							SaConstStringT *jobNames,
							SaConstStringT *measurementTypes,
							SaTimeT timeout,
							SaPmCInstanceValuesT ***instanceValues);
	/*
	* Free memory  associated with  instance values.
	*
	* @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	* @param instanceValues : [in] pointer to a NULL-terminated array of pointer to instance values to be freed.
	*
	* @return SaAisErrorT
	*/
	virtual SaAisErrorT saPmCInstanceValuesMemoryFree(SaPmCHandleT pmHandle,
			SaPmCInstanceValuesT **instanceValues);

	/**
	 * @ingroup PmConsumer
	 *
	 * @brief Check status of optional API features.
	 *
	 * @param enabled : [out] enabled status
	 *
	 * @return SaAisErrorT
	 */
	virtual SaAisErrorT saPmCIsFeatureEnabled(SaBoolT *enabled);
	/**
	 * @ingroup PmConsumer
	 *
	 * @brief Read real-time measurement instance values.
	 *
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 * @param instance : [in] instance name, either a NULL string or wildcard filter construct
	 * @param measurementTypes : [in] pointer to a NULL-terminated array of measurement type names
	 * @param pmGroups : [in] pointer to a NULL-terminated array of pm group names
	 * @param timeout : [in] saPmCGetInstanceValues_2 will return if it does not complete by the time specified
	 * @param requestId : [in:out] id used to identify an on-going operation
	 * @param instanceValues : [out] pointer to a pointer to NULL-terminated array of pointers to instance values
	 * @param errorText : [out] pointer to string identifying invalid parameter
	 *
	 * @return SaAisErrorT
	 */
	virtual SaAisErrorT saPmCGetInstanceValues_2(
			SaPmCHandleT pmHandle,
			SaConstStringT instance,
			SaConstStringT *measurementTypes,
			SaConstStringT *pmGroups,
			SaTimeT timeout,
			SaUint32T *requestId,
			SaPmCInstanceValuesT_2 ***instanceValues,
			SaConstStringT *errorText);

	/**
	 * @ingroup PmConsumer
	 *
	 * @brief Free memory associated with real-time instance values or real-time instance value error.
	 *
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 * @param instanceValues : [in] pointer to NULL-terminated array of instance values to be freed
	 * @param errorText : [in] error string to be freed
	 *
	 * @return SaAisErrorT
	 */
	virtual SaAisErrorT saPmCInstanceValuesMemoryFree_2(
			SaPmCHandleT pmHandle,
			SaPmCInstanceValuesT_2 **instanceValues,
			SaConstStringT errorText);

	/**
	 * @ingroup PmConsumer
	 *
	 * @brief Cancel in-progress #saPmCGetInstanceValues_2 operation.
	 *
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 * @param requestId : [in] the requestId returned by #saPmCGetInstanceValues_2
	 *
	 * @return SaAisErrorT
	 */
	virtual SaAisErrorT saPmCCancelGetInstanceValues_2(
			SaPmCHandleT pmHandle,
			SaUint32T requestId);

	/**
	 * @ingroup PmConsumer
	 *
	 * @brief Free memory associated with real-time instance, measurement type or PM group names.
	 *
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 * @param names : [in] pointer to NULL-terminated array of names to be freed
	 *
	 * @return SaAisErrorT
	 */
	virtual SaAisErrorT saPmCNamesMemoryFree(
			SaPmCHandleT pmHandle,
			SaConstStringT *names);
	/**
	 * Return measurement type names associated with real-time instance values.
	 *
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 * @param instances : [in] instance name
	 * @param measurementType : [in]  measurement type name, either a NULL string or wildcard filter construct
	 * @param pmGroups : [in] pointer to a NULL-terminated array of pm group names
	 * @param timeout : [in] The saPmCGetMeasurementTypeNames invocation is considered to have failed if it does
	 * not complete by the time specified
	 * @param measurementTypeNames : [out] pointer to a pointer to NULL-terminated array of measurement type names
	 *
	 * @return SaAisErrorT
	 */
	virtual SaAisErrorT saPmCGetMeasurementTypeNames(SaPmCHandleT pmHandle,
			SaConstStringT instance,
			SaConstStringT measurementType,
			SaConstStringT *pmGroups,
			SaTimeT timeout,
			SaConstStringT **measurementTypeNames);

	/**
	 * @ingroup PmConsumer
	 *
	 * @brief Return PM group names associated with real-time instance values.
	 *
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 * @param instance : [in] instance name
	 * @param measurementTypes : [in] pointer to a NULL-terminated array of measurement type names
	 * @param pmGroup : [in] pm group name, either a NULL string or wildcard filter construct
	 * @param timeout : [in] The saPmCGetPmGroupNames invocation is considered to have failed if it does
	 * not complete by the time specified
	 * @param pmGroupNames : [out] pointer to a pointer to NULL-terminated array of PM group names
	 *
	 * @return SaAisErrorT
	 */
	virtual SaAisErrorT saPmCGetPmGroupNames(
			SaPmCHandleT pmHandle,
			SaConstStringT instance,
			SaConstStringT *measurementTypes,
			SaConstStringT pmGroup,
			SaTimeT timeout,
			SaConstStringT **pmGroupNames);
	/**
	 * @ingroup PmConsumer
	 *
	 * @brief Return names of real-time measurement instance values.
	 *
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 * @param instance : [in] instance name, either a NULL string or wildcard filter construct
	 * @param measurementTypes : [in] pointer to a NULL-terminated array of measurement type names
	 * @param pmGroups : [in] pointer to a NULL-terminated array of pm group names
	 * @param timeout : [in] The saPmCGetInstanceNames invocation is considered to have failed if it does
	 * not complete by the time specified
	 * @param instanceNames : [out] pointer to a pointer to NULL-terminated array of instance names
	 *
	 * @return SaAisErrorT
	 *
	 * This function returns names of real-time instance values.
	 *
	 * This function allocates the memory to return the instance names. When the calling process no longer needs
	 * to access the instance names, the memory must be freed by calling #saPmCNamesMemoryFree.
	 */
	virtual SaAisErrorT saPmCGetInstanceNames(
			SaPmCHandleT pmHandle,
			SaConstStringT instance,
			SaConstStringT *measurementTypes,
			SaConstStringT *pmGroups,
			SaTimeT timeout,
			SaConstStringT **instanceNames);

	/**
	* Pointer to the singleton instance of the object.  Is public due to
	* historical reasons.
	*/
	static PmConsumerInterface* s_instance;

	/**
	* Predicate telling if we managed to load the PM consumer lib.
	*
	* @return  bool    true if found and loaded, false otherwise
	*/
	virtual bool foundSaPmConsumer() { ENTER_PMTSA(); LEAVE_PMTSA(); return m_foundSaPmConsumer; };

	/**
	 * Predicate telling if COMSA is registered to support CoreMW PM API optional features.
	 * Checks if PM show-counters v1.1 feature support is enabled by CoreMW PM Service or not.
	 *
	 * @return  bool    true if feature supported.
	 *
	 * Note: This function doesn't check with CoreMW. However, when m_showCountersFeatureV2 is
	 * set to UNASSIGNED, this function inturn invokes saPmCIsFeatureEnabled() which gets the
	 * status from CoreMW and updates the result accordingly.
	 */
	bool isShowCounterV2Enabled();

	/**
	* Method that will attempt to load the PM consumer shared library.
	* The method can be called multiple times.  If the lib isn't loaded
	* it will try to load it, if the lib is already loaded the
	* method won't do anything.
	*
	* @return  bool    true on success, false otherwise
	*/
	virtual bool loadSaPmConsumer();

	typedef enum {
		UNASSIGNED = 0,
		ENABLED = 1,
		DISABLED = 2
	} showCountersFeatureStatusT;

private:
	volatile bool m_foundSaPmConsumer; /*!< Load-status */
	void* m_dlHandle; /*!< Handle to loaded dynamic lib */

	//Flag to know the status of show-counters optional features
	showCountersFeatureStatusT m_showCountersFeatureV2;
	void setDynamicInterfacesNull();

	const std::string PMC_LibraryName;          /*!< Name of library-file */
	const std::string PMC_Initialize;           /*!< Name of initialize-method */
	const std::string PMC_Initialize_2;         /*!< Name of initialize_2-method */
	const std::string PMC_Finalize;             /*!< Name of finalize-method */
	const std::string PMC_Activate;             /*!< Name of activate-method */
	const std::string PMC_Deactivate;           /*!< Name of deactivate-method */
	const std::string PMC_SelectionObjectGet;   /*!< Name of selectionObjectGet-method */
	const std::string PMC_Dispatch;             /*!< Name of dispatch-method */
	const std::string PMC_IteratorInitialize;   /*!< Name of iteratorInitialize-method */
	const std::string PMC_IteratorInitialize_2; /*!< Name of iteratorInitialize_2-method */
	const std::string PMC_IteratorNext;         /*!< Name of iteratorNext-method */
	const std::string PMC_IteratorNext_2;       /*!< Name of iteratorNext_2-method */
	const std::string PMC_IteratorFinalize;     /*!< Name of iteratorFinalize-method */
	const std::string PMC_IteratorFinalize_2;   /*!< Name of iteratorFinalize_2-method */
	const std::string PMC_JobGPDataRemove;      /*!< Name of jobGPDataRemove-method */
	const std::string PMC_CurrentValuesGet;      /*!< Name of saPmCCurrentValuesGet-method */
	const std::string PMC_GetInstanceValues;     /*!< Name of saPmCGetInstanceValues-method */
	const std::string PMC_InstanceValuesMemoryFree;/*!< Name of saPmCInstanceValuesMemoryFree-method */
	const std::string PMC_IsFeatureEnabled;     /*Name of the saPmCIsFeatureEnabled method*/
	const std::string PMC_ShowCountersFeatureName; /*Name of SAPMC_FEATURE_SHOW_COUNTERS*/
	const std::string PMC_ShowCountersVersionName; /*Name of SAPMC_VERSION_2*/
	const std::string PMC_NamesMemoryFree;      /*Name of the saPmCNamesMemoryFree method*/
	const std::string PMC_GetInstanceValues_2;  /*Name of the saPmCGetInstanceValues_2*/
	const std::string PMC_InstanceValuesMemoryFree_2; /*Name of the saPmCInstanceValuesMemoryFree_2 method*/
	const std::string PMC_CancelGetInstanceValues_2; /*Name of the saPmCCancelGetInstanceValues_2 method*/
	const std::string PMC_GetMeasurementTypeNames;/*!< Name of saPmCGetMeasurementTypeNames-method */
	const std::string PMC_GetPmGroupNames;      /*Name of the saPmCGetPmGroupNames-method*/
	const std::string PMC_GetInstanceNames;   /*Name of the saPmGetMeasuredObjects method*/
	/**
	* Initialize-method signature
	*/
	typedef SaAisErrorT (*saPmCInitialize_t)(SaPmCHandleT* pmHandle,
			SaVersionT* version, SaPmfJobTypeT jobType,
			SaPmCJobDataOrganizationT option, SaPmCSubscribeInfoT* subscribeInfo);
	/**
	* Initialize_2-method signature
	*/
	typedef SaAisErrorT (*saPmCInitialize_2_t)(SaPmCHandleT* pmHandle,
			SaVersionT* version, SaPmfJobTypeT jobType,
			SaPmCJobDataOrganizationT option, SaPmCSubscribeInfoT_2* subscribeInfo);
	/**
	* Finalize-method signature
	*/
	typedef SaAisErrorT (*saPmCFinalize_t)(SaPmCHandleT pmHandle);
	/**
	* Activate-method signature
	*/
	typedef SaAisErrorT (*saPmCActivate_t)(SaPmCHandleT pmHandle);
	/**
	* Deactivate-method signature
	*/
	typedef SaAisErrorT (*saPmCDeactivate_t)(SaPmCHandleT pmHandle);
	/**
	* SelectionObjectGet-method signature
	*/
	typedef SaAisErrorT (*saPmCSelectionObjectGet_t)(SaPmCHandleT pmHandle,
			SaSelectionObjectT *selfd);
	/**
	* Dispatch-method signature
	*/
	typedef SaAisErrorT (*saPmCDispatch_t)(SaPmCHandleT pmHandle, SaDispatchFlagsT flags);
	/**
	* IteratorInitialize-method signature
	*/
	typedef SaAisErrorT (*saPmCIteratorInitialize_t)(SaPmCJobGranularityPeriodDataT hdl,
			SaPmCIteratorHandleT *iteratorHandle);
	/**
	* IteratorInitialize-method signature
	*/
	typedef SaAisErrorT (*saPmCIteratorInitialize_2_t)(SaPmCJobGranularityPeriodDataT hdl,
			SaPmCIteratorHandleT_2 *iteratorHandle);
	/**
	* IteratorNext-method signature
	*/
	typedef SaAisErrorT (*saPmCIteratorNext_t)(SaPmCIteratorHandleT iteratorHandle,
			SaPmCIteratorInfoT *iteratorInfo);
	/**
	* IteratorNext_2-method signature
	*/
	typedef SaAisErrorT (*saPmCIteratorNext_2_t)(SaPmCIteratorHandleT_2 iteratorHandle,
							SaPmCIteratorInfoT_2 *iteratorInfo,
							SaPmCAggregatedValuesT *values);
	/**
	* IteratorFinalize-method signature
	*/
	typedef SaAisErrorT (*saPmCIteratorFinalize_t)(SaPmCIteratorHandleT iteratorHandle);
	/**
	* IteratorFinalize_2-method signature
	*/
	typedef SaAisErrorT (*saPmCIteratorFinalize_2_t)(SaPmCIteratorHandleT_2 iteratorHandle);
	/**
	* JobGPDataRemove-method signature
	*/
	typedef SaAisErrorT (*saPmCJobGranularityPeriodDataRemove_t)(SaPmCJobGranularityPeriodDataT hdl);
	/**
	* saPmCCurrentValuesGet-method signature
	*/
	typedef SaAisErrorT (*saPmCCurrentValuesGet_t)(SaPmCHandleT pmHandle);
	/**
	* saPmCGetInstanceValues-method signature
	*/
	typedef SaAisErrorT (*saPmCGetInstanceValues_t)(SaPmCHandleT pmHandle,
							SaConstStringT instance,
							SaConstStringT *jobNames,
							SaConstStringT *measurementTypes,
							SaTimeT timeout,
							SaPmCInstanceValuesT ***instanceValues);
	/**
	* saPmCInstanceValuesMemoryFree-method signature
	*/
	typedef SaAisErrorT (*saPmCInstanceValuesMemoryFree_t)(SaPmCHandleT pmHandle,
			SaPmCInstanceValuesT **instanceValues);

	/**
	 * saPmCIsFeatureEnabled-method signature
	 */
	typedef SaAisErrorT (*saPmCIsFeatureEnabled_t)(SaPmCHandleT pmHandle,
			SaConstStringT featureName,
			SaConstStringT featureVersion,
			SaBoolT *enabled);

	// saPmCGetInstanceValues_2-method signature
	typedef SaAisErrorT (*saPmCGetInstanceValues_2_t)(
			SaPmCHandleT pmHandle,
			SaConstStringT instance,
			SaConstStringT *measurementTypes,
			SaConstStringT *pmGroups,
			SaTimeT timeout,
			SaUint32T *requestId,
			SaPmCInstanceValuesT_2 ***instanceValues,
			SaConstStringT *errorText);

	// saPmCInstanceValuesMemoryFree_2-method signature
	typedef SaAisErrorT (*saPmCInstanceValuesMemoryFree_2_t)(
			SaPmCHandleT pmHandle,
			SaPmCInstanceValuesT_2 **instanceValues,
			SaConstStringT errorText);

	/**
	 * saPmCCancelGetInstanceValues_2-method signature
	 */
	typedef SaAisErrorT (*saPmCCancelGetInstanceValues_2_t)(
			SaPmCHandleT pmHandle,
			SaUint32T requestId);

	/**
	 * saPmCNamesMemoryFree-method signature
	 */
	typedef SaAisErrorT (*saPmCNamesMemoryFree_t)(SaPmCHandleT pmHandle, SaConstStringT *names);

	/**
	 * saPmCGetMeasurementTypeNames-method signature
	 */
	typedef SaAisErrorT (*saPmCGetMeasurementTypeNames_t)(SaPmCHandleT pmHandle,
			SaConstStringT instance,
			SaConstStringT measurementType,
			SaConstStringT *pmGroups,
			SaTimeT timeout,
			SaConstStringT **measurementTypeNames);

	/**
	 * saPmCGetPmGroupNames-method signature
	 */
	typedef SaAisErrorT (*saPmCGetPmGroupNames_t)(SaPmCHandleT pmHandle,
			SaConstStringT instances,
			SaConstStringT *measurementTypes,
			SaConstStringT pmGroup,
			SaTimeT timeout,
			SaConstStringT **pmGroupNames);

	/**
	 * saPmCGetInstanceNames-method signature
	 */
	typedef SaAisErrorT (*saPmCGetInstanceNames_t)(SaPmCHandleT pmHandle,
			SaConstStringT instance,
			SaConstStringT *measurementTypes,
			SaConstStringT *pmGroups,
			SaTimeT timeout,
			SaConstStringT **instanceNames);

	saPmCInitialize_t saPmCInitialize_dl; /*!< Function-pointer to dynamically loaded method */
	saPmCInitialize_2_t saPmCInitialize_2_dl; /*!< Function-pointer to dynamically loaded method */
	saPmCFinalize_t saPmCFinalize_dl; /*!< Function-pointer to dynamically loaded method */
	saPmCActivate_t saPmCActivate_dl; /*!< Function-pointer to dynamically loaded method */
	saPmCDeactivate_t saPmCDeactivate_dl; /*!< Function-pointer to dynamically loaded method */
	saPmCSelectionObjectGet_t saPmCSelectionObjectGet_dl; /*!< Function-pointer to dynamically loaded method */
	saPmCDispatch_t saPmCDispatch_dl; /*!< Function-pointer to dynamically loaded method */
	saPmCIteratorInitialize_t saPmCIteratorInitialize_dl; /*!< Function-pointer to dynamically loaded method */
	saPmCIteratorInitialize_2_t saPmCIteratorInitialize_2_dl; /*!< Function-pointer to dynamically loaded method */
	saPmCIteratorNext_t saPmCIteratorNext_dl; /*!< Function-pointer to dynamically loaded method */
	saPmCIteratorNext_2_t saPmCIteratorNext_2_dl; /*!< Function-pointer to dynamically loaded method */
	saPmCIteratorFinalize_t saPmCIteratorFinalize_dl; /*!< Function-pointer to dynamically loaded method */
	saPmCIteratorFinalize_2_t saPmCIteratorFinalize_2_dl; /*!< Function-pointer to dynamically loaded method */
	saPmCJobGranularityPeriodDataRemove_t saPmCJobGranularityPeriodDataRemove_dl; /*!< Function-pointer to dynamically loaded method */
	saPmCCurrentValuesGet_t saPmCCurrentValuesGet_dl;/*Function-pointer to dynamically loaded method*/
	saPmCGetInstanceValues_t saPmCGetInstanceValues_dl;/*Function-pointer to dynamically loaded method*/
	saPmCInstanceValuesMemoryFree_t saPmCInstanceValuesMemoryFree_dl;/*Function-pointer to dynamically loaded method*/
	saPmCIsFeatureEnabled_t saPmCIsFeatureEnabled_dl; /*Function-pointer to dynamically loaded method*/
	saPmCNamesMemoryFree_t saPmCNamesMemoryFree_dl; /*Function-pointer to dynamically loaded method*/
	saPmCGetInstanceValues_2_t saPmCGetInstanceValues_2_dl; /*Function-pointer to dynamically loaded method*/
	saPmCInstanceValuesMemoryFree_2_t saPmCInstanceValuesMemoryFree_2_dl; /*Function-pointer to dynamically loaded method*/
	saPmCCancelGetInstanceValues_2_t saPmCCancelGetInstanceValues_2_dl; /*Function-pointer to dynamically loaded method*/
	saPmCGetMeasurementTypeNames_t saPmCGetMeasurementTypeNames_dl;/*Function-pointer to dynamically loaded method*/
	saPmCGetPmGroupNames_t saPmCGetPmGroupNames_dl; /*Function-pointer to dynamically loaded method*/
	saPmCGetInstanceNames_t saPmCGetInstanceNames_dl; /*Function-pointer to dynamically loaded method*/

	template <class T> T loadSymName(const std::string& symName);
};

/**
* Method that loads the given symbol from a dynamic library and returns
* it to the caller.
*
* @par Pre- and post-conditions
* The method requires that the dynamic library is opened. If we fail
* resolving the symbol name, the library is closed, the handle to it is
* NULLED as well as all symbols we've loaded before the failure.
*
* @param[in] 		symName     Typically a function in a dynamic library
*
* @return 	T       NULL if the symbol wasn't found, otherwise a ref to the function
*/
template <class T>
T PmConsumerInterface::loadSymName(const std::string& symName)
{
	ENTER_PMTSA();
	if (m_dlHandle != NULL)
	{
		T res = reinterpret_cast<T>(dlsym(m_dlHandle, symName.c_str()));
		char* dynLoadError = dlerror();
		if ((dynLoadError != NULL)
				&& (symName != "saPmCIteratorInitialize_2")   // These are available only for PDF counter
				&& (symName != "saPmCIteratorNext_2")         //
				&& (symName != "saPmCIteratorFinalize_2")     //
				&& (symName != "saPmCIsFeatureEnabled"))       // and will fail with older CoreMW
		{
			std::cerr << "Failed looking up " << symName << " in library: " << dynLoadError << std::endl;
			dlclose(m_dlHandle);
			m_dlHandle = NULL;
			m_foundSaPmConsumer = false;
			m_showCountersFeatureV2 = UNASSIGNED;
			setDynamicInterfacesNull();
		}
		LEAVE_PMTSA();
		return res;
	}
	LEAVE_PMTSA();
	return NULL;
}
#endif /* PMCONSUMERINTERFACE_HXX_ */

