/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Ericsson AB
 *
 */

/**
 * @cond OSAF_PM_API
 * @mainpage Performance Management Consumer API
 * @endcond
 *
 * @section public_sec Public API's
 *
 * @subsection pm_consumer The PM Consumer API
 * The PM Consumer API is the used to retrieve aggregated jobs.
 * The API is documented in
 * @ref saPmConsumer.h
 *
 */


/**
 * @file saPmConsumer.h
 * @ingroup PmConsumer
 * @brief This file contains the PM Consumer API
 * @example pmsv_consumer_example.c
 * This example shows how to use the PM Consumer API
 *
 */


#ifndef SAPMCONSUMER_H
#define	SAPMCONSUMER_H

#include "saAis.h"
#include "saPm.h"

#ifdef	__cplusplus
extern "C" {
#endif
	/**
	 * A handle used when accessing PM Consumer API.
	 */
	typedef void *SaPmCHandleT;

	/**
	 * @cond OSAF_PM_API
	 * A handle used iterating data in a job.
	 * @deprecated Since A.02.01
	 */
	typedef void * SaPmCIteratorHandleT;
	/* @endcond */

	/**
	 * New version of SaPmCIteratorHandleT
	 *
	 */
	typedef void * SaPmCIteratorHandleT_2;
	/**
	 * An identifier for a deliverance of data for a job
	 */
	typedef SaUint64T SaPmCJobGranularityPeriodDataT;

	/**
	 * This type is used for handle set by the client when registering the callback
	 */
	typedef SaUint64T SaPmCClientHandleT;

	/**
	 * This is sent as argument to callback.
	 *
	 * Note that the values of this structure are only valid until the
	 * callback returns
	 */
	typedef struct {
		/** The identifier set by the client when subscribing, compare with
		 * #SaPmCSubscribeInfoT
		 */
		SaPmCClientHandleT clientHdlr;
		/**A handler for the job*/
		SaPmCJobGranularityPeriodDataT jobHandler;
		/**The name of the job*/
		SaStringT jobname;
		/**The start time for the job*/
		SaTimeT gpStartTimestamp;
		/**The end time for the job*/
		SaTimeT gpEndTimestamp;
	} SaPmCCallbackInfoT;

	/**
	 * Typedef for the callback function
	 */
	typedef SaAisErrorT(*SaPmCCallbackT) (SaPmCCallbackInfoT * info);

	/**
	 * The different types of job data requests we can get.
	 */
	typedef enum {
		/**Request for GP aligned the job data.*/
		SA_PMF_JOB_DATA_GP_CALLBACK = 1,
		/**Request for poll immediate, non-GP aligned  job data.*/
		SA_PMF_JOB_DATA_READ_CURRENT = 2
	} SaPmCJobDataTypeT;

	/**
	 * This contains information about the callback used when subscribing
	 */
	typedef struct {
		/**The supplied callback that should be called once for each job*/
		SaPmCCallbackT cbPtr;
		/**An identifier set by the client*/
		SaPmCClientHandleT clientHdlr;
	} SaPmCSubscribeInfoT;

	/**
	 * The structure #SaPmCRMSubscribeInfoT holds job subscription information
	 * for Resource Monitor job types.
	 */
	typedef struct {
		/**The supplied callback that should be called once for each job*/
		SaPmCCallbackT cbPtr;
		/**An identifier set by the client*/
		SaPmCClientHandleT clientHdlr;
		/**Job data type*/
		SaPmCJobDataTypeT jobDataType;
		/**NULL terminated list of job names for specific job subscription*/
		SaStringT *jobList;
		/**NULL terminated list of instance names that the consumer subscribes to */
		SaStringT *instanceList;
	} SaPmCRMSubscribeInfoT;

	/**
	 * Union to store subscription options.
	 */
	typedef union  {
		/**For measurement type jobs*/
		SaPmCSubscribeInfoT info;
		/**For resource monitor jobs*/
		SaPmCRMSubscribeInfoT rmInfo;
	} SaPmCSubscribeInfoT_2;

	/**
	 * The different ways the data could be organized.
         * Note: PMC_JOB_DATA_ORGANIZATION_NODE_MOCLASS_MEASOBJ is only valid
         * for resource monitor job types with GP end callback job data type.
	 */
	typedef enum {
		/**The data should be organized as moClass, measObject, type-value*/
		PMC_JOB_DATA_ORGANIZATION_MOCLASS_MEASOBJ = 1,
                /**The data should be organized as node, moClass, measObject, type-value*/
		PMC_JOB_DATA_ORGANIZATION_NODE_MOCLASS_MEASOBJ = 2
	} SaPmCJobDataOrganizationT;

	/**
	 * The different kind of object we can get during iteration.
	 */
	typedef enum {
		/**The object is an moClass */
		PMF_JOB_ITERATOR_OBJECT_MOCLASS = 1,
		/**The object is a measObjectLdn*/
		PMF_JOB_ITERATOR_OBJECT_LDN = 2,
		/**The object is a measurement value*/
		PMF_JOB_ITERATOR_OBJECT_VALUE = 3,
		/**The object is a  system/cluster node identifier*/
		PMF_JOB_ITERATOR_OBJECT_NODE = 4
	} SaPmfJobIteratorObjectT;

	/**
	 * Union to store aggregated value
	 */
	typedef union {
		/**The aggregated value if it is a float*/
		double floatVal;
		/**The aggregated value if it is an integer*/
		SaUint64T intVal;
	} SaPmAggregatedValueT;

	/**
	 * @cond OSAF_PM_API
	 * The structure #SaPmAggregatedValueInfoT holds an aggregated value
	 * together with additional information.
	 * @deprecated Since A.02.01
	 */
	typedef struct {
		/**The value itself*/
		SaPmAggregatedValueT value;
		/**TRUE if value is a float*/
		SaBoolT isFloat;
		/**TRUE if value is suspect, i.e. it belongs to a collection of
		 data that might not be complete */
		SaBoolT isSuspect;
	} SaPmAggregatedValueInfoT;
	/* @endcond */

	/**
	 * The structure #SaPmAggregatedValueInfoT_2 holds  the information of node.
	 * It doesn't contain the aggregated values
	 */
	typedef struct {
		/** A count of the values */
		SaInt16T multiplicity;
		/**TRUE if value is a float*/
		SaBoolT isFloat;
		/**TRUE if value is suspect, i.e. it belongs to a collection of
		 data that might not be complete */
		SaBoolT isSuspect;
	} SaPmAggregatedValueInfoT_2;
	/**
	 *
	 * This type is used to retrieve data for Consummer App
	 */
	typedef SaPmAggregatedValueT* SaPmCAggregatedValuesT;

	/**
	 * @cond OSAF_PM_API
	 * Information associated with a node.
	 * @deprecated Since A.02.01
	 */
	typedef struct {
		/**True if the current node is a leaf*/
		SaBoolT isLeaf;
		/**The kind of object */
		SaPmfJobIteratorObjectT objectType;
		/**The name or identifying string for the node*/
		SaStringT nodeName;
		/**When #isLeaf is true, this will contain the value of the object
		 * otherwise the content will be undefined
		 */
		SaPmAggregatedValueInfoT valueInfo;
	} SaPmCIteratorInfoT;
	/* @endcond */

	/**
	 * Information associated with a node.
	 */
	typedef struct {
		/**True if the current node is a leaf*/
		SaBoolT isLeaf;
		/**The kind of object */
		SaPmfJobIteratorObjectT objectType;
		/**The name or identifying string for the node*/
		SaStringT nodeName;
		/**When #isLeaf is true, this will contain the values of the object
		 * otherwise the content will be undefined
		 */
		SaPmAggregatedValueInfoT_2 valueInfo;
	} SaPmCIteratorInfoT_2;
	/**
	 * The structure #SaPmCInstanceJobValueT holds the aggregated, non-GP aligned,
	 * values for a job returned by #saPmCGetInstanceValues
	 */
	typedef struct {
		/**Name of the job*/
		SaConstStringT jobName;
		/**Job's GP*/
		SaTimeT granularityPeriod;
		/**Info used to access the values member*/
		SaPmAggregatedValueInfoT_2 valueInfo;
		/**Current aggregated values of the job*/
		SaPmCAggregatedValuesT values;
	} SaPmCInstanceJobValueT;
	/**
	 * The structure #SaPmCInstanceValuesT holds the information about aggregated,
	 * non-GP aligned, values of a measurement type associated with an instance.
	 */
	typedef struct {
		/**Name of measurementType*/
		SaConstStringT measurementType;
		/**Number of jobValues, use to access next member*/
		SaSizeT jobValuesNumber;
		/**Array of SaPmCInstanceJobValueT*/
		SaPmCInstanceJobValueT *jobValues;
	} SaPmCInstanceValuesT;

	/**
	 * The structure #SaPmCInstanceValuesT_2 holds the information about
	 * real-time values of a measurement type associated with an instance.
	 */
	typedef struct {
		/**Name of measurementType*/
		SaConstStringT measurementType;
		/**Name of instance*/
		SaConstStringT instance;
		/**Name of group*/
		SaConstStringT pmGroup;
		/**Info used to access the values member*/
		SaPmAggregatedValueInfoT_2 valueInfo;
		/**Real-time values*/
		SaPmCAggregatedValuesT values;
	} SaPmCInstanceValuesT_2;

	extern SaConstStringT SAPMC_FEATURE_SHOW_COUNTERS;
	extern SaConstStringT SAPMC_VERSION_1;
	extern SaConstStringT SAPMC_VERSION_2;

	/**
         * @cond OSAF_PM_API
	 * @ingroup PmConsumer
	 * @brief Initialize the PM Consumer Agent and subscribe to one type of jobs
	 * @param pmHandle : [out] handle for PM Agent
	 * @param version : [in:out] version information
	 * @param jobType : [in] indicates the type of jobs the call subscribes to
	 * @param option : [in] indicates how the data in the job should be
	 * organized, see #SaPmCJobDataOrganizationT. @b Note, valid only for
	 * measurement jobs.
	 * @param subscribeInfo : [in] struct initialized with subscription info
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK The function completed successfully.
	 * @retval SA_AIS_ERR_BAD_OPERATION PM Consumer Agent already initialized
	 * @retval SA_AIS_ERR_INVALID_PARAM A parameter is invalid
	 * @retval SA_AIS_ERR_VERSION Unsupported version
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * This function initializes the PM Consumer for the invoking process.
	 * This function must be invoked prior to the invocation of any other
	 * PM Consumer functions. The handle pointed to by pmHandle is returned
	 * by the PM Service as the reference to this association between the
	 * process and the PM Consumer. The process uses this handle in
	 * subsequent communication with the PM Consumer.
	 *
	 * A call to #saPmCInitialize also subscribes to one type of jobs.
	 * Allowed value of @b jobType is
	 * @li PMSV_JOB_TYPE_MEASUREMENT_JOB, subscribe to measurement jobs
	 *
	 * Note that the enum #SaPmfJobTypeT has more possible values, but
	 * only the one mentioned above is allowed.
	 *
	 * The important members of the @b subscribeInfo are
	 * @li cbPtr, the callback that should be called for each job
	 * @li clientHdlr, this can be set by the user of the PM Consumer API
	 * and will be kept in the callback.
	 *
	 * @b Example:
	 * @code
	 * version.releaseCode = 'A';
	 * version.majorVersion = 2;
	 * version.minorVersion = 1;
	 *
	 * SaPmfJobTypeT jType = SA_PMF_JOB_TYPE_MEASUREMENT;
	 * SaPmCJobDataOrganizationT org = PMC_JOB_DATA_ORGANIZATION_MOCLASS_MEASOBJ;
	 * SaPmCSubscribeInfoT subinfo;
	 * subinfo.cbPtr = consumerCb;
	 * subinfo.clientHdlr = 0;
	 *
	 * saPmCInitialize(&pmHandle, &version, jType, org, &subinfo);
	 * @endcode
	 * @deprecated Since A.03.01
	 */
	extern SaAisErrorT saPmCInitialize(SaPmCHandleT* pmHandle,
		SaVersionT* version, SaPmfJobTypeT jobType,
		SaPmCJobDataOrganizationT option, SaPmCSubscribeInfoT* subscribeInfo);
	/* @endcond */

	/**
         * @ingroup PmConsumer
	 * @brief Initialize the PM Consumer Agent and subscribe to one type of jobs
	 * @param pmHandle : [out] handle for PM Agent
	 * @param version : [in:out] version information
	 * @param jobType : [in] indicates the type of jobs the call subscribes to
	 * @param option : [in] indicates how the data in the job should be
	 * organized, see #SaPmCJobDataOrganizationT.
	 * @param subscribeInfo : [in] struct initialized with subscription info
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK The function completed successfully.
	 * @retval SA_AIS_ERR_BAD_OPERATION PM Consumer Agent already initialized
	 * @retval SA_AIS_ERR_INVALID_PARAM A parameter is invalid
	 * @retval SA_AIS_ERR_VERSION Unsupported version
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * This function initializes the PM Consumer for the invoking process.
	 * This function must be invoked prior to the invocation of any other
	 * PM Consumer functions. The handle pointed to by pmHandle is returned
	 * by the PM Service as the reference to this association between the
	 * process and the PM Consumer. The process uses this handle in
	 * subsequent communication with the PM Consumer.
	 *
	 * A call to #saPmCInitialize_2 also subscribes to one type of job.
	 * Allowed values of @b jobType are
	 * @li PMSV_JOB_TYPE_MEASUREMENT_JOB, subscribe to measurement jobs
	 * @li PMSV_JOB_TYPE_RESOURCEMONITOR_JOB, subscribe to resource monitor jobs
	 *
	 * Note that the enum #SaPmfJobTypeT has more possible values, but
	 * only the two mentioned above are allowed.
	 *
	 * The important members of the @b subscribeInfo parameter are
	 *
	 * <B>For both measurement and resource monitor job:</B>
	 *
	 * @li cbPtr, the callback that should be called for each job
	 * @li clientHdlr, this can be set by the user of the PM Consumer API
	 * and will be kept in the callback.
	 *
	 * <B>For resource monitor jobs only:</B>
	 *
	 * @li jobDataType, the data type being subscribed to. SA_PMF_JOB_DATA_GP_CALLBACK to receive
	 * a callback with aggregated data at GP end, as per measurement job consumer, or SA_PMF_JOB_DATA_READ_CURRENT
	 *to immediately read local non-aggregated current values, ie poll values on the the current node.
	 * @li jobList, an optional list of resource monitor job names. If jobList is NULL, consumer subscribes to all resource monitor jobs.
	 * @li instanceList, an optional list of instance names. If instanceList is NULL, consumer subscribes to all instance names.
	 *
	 * @b Examples:
	 *
	 * <B>Measurement Job</B>
	 * @code
	 * version.releaseCode = 'A';
	 * version.majorVersion = 3;
	 * version.minorVersion = 1;
	 * SaPmfJobTypeT jType = SA_PMF_JOB_TYPE_MEASUREMENT;
	 * SaPmCJobDataOrganizationT org = PMC_JOB_DATA_ORGANIZATION_MOCLASS_MEASOBJ;
	 * SaPmCSubscribeInfoT subinfo;
	 * subinfo.cbPtr = consumerCb;
	 * subinfo.clientHdlr = 0;
	 *
	 * saPmCInitialize(&pmHandle, &version, jType, org, &subinfo);
	 *
	 * @endcode
	 *
	 * <B>Resource Monitor Job</B>
	 *
	 * @code
	 *
	 * SaPmfJobTypeT jType = SA_PMF_JOB_TYPE_RESOURCEMONITOR;
	 * SaPmCJobDataOrganizationT org = PMC_JOB_DATA_ORGANIZATION_MOCLASS_MEASOBJ;
	 * SaStringT jobNames[3] = { "safRmJob=PM-1", "safRmJob=PM-2", NULL };
	 * SaStringT instanceNames[3] = { "FRUId=19", "AppXId=1", NULL };
	 * SaPmCSubscribeInfoT_2 subinfo;
	 * subinfo.rmInfo.cbPtr = consumerCb;
	 * subinfo.rmInfo.clientHdlr = 0;
	 * subinfo.rmInfo.jobDataType = SA_PMF_JOB_DATA_GP_CALLBACK ;
	 * subinfo.rmInfo.jobList = jobNames;
	 * subinfo.rmInfo.instanceList = instanceNames;
	 *
	 * saPmCInitialize_2(&pmHandle, &version, jType, org, &subinfo);
	 * @endcode
	 *
	 * @b Examples:
	 * pmsv_consumer_example.c
	 */
	extern SaAisErrorT saPmCInitialize_2(SaPmCHandleT* pmHandle,
		SaVersionT* version,
		SaPmfJobTypeT jobType,
		SaPmCJobDataOrganizationT option,
		SaPmCSubscribeInfoT_2* subscribeInfo);

	/**
         * @ingroup PmConsumer
	 * @brief Finalizes the PM Consumer Agent
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK The function completed successfully.
	 * @retval SA_AIS_ERR_INVALID_PARAM The parameter, pmHandle, is invalid
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * The #saPmCFinalize function closes the association represented by
	 * the pmHandle parameter between the invoking process and the PM Consumer.
	 * The process must have invoked #saPmCInitialize_2 before it invokes
	 * this function.
	 *
	 * If PM Consumer is active it will be automatically deactivated.
	 *
	 * @b Example:
	 * @code
	 * saPmCFinalize(pmHandle);
	 * @endcode
	 */
	extern SaAisErrorT saPmCFinalize(SaPmCHandleT pmHandle);

	/**
	 * @ingroup PmConsumer
	 * @brief Returns the operating system poll/select handle associated with
	 * the pmHandle.
	 *
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 * @param selfd : [out] The poll/select handle will be returned here.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK The function completed successfully.
	 */
	extern SaAisErrorT saPmCSelectionObjectGet(SaPmCHandleT pmHandle, SaSelectionObjectT *selfd);

	/**
	 * @cond OSAF_PM_API
	 * @ingroup PmConsumer
	 * @brief Activate a #SaPmCHandleT
	 *
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK The function completed successfully.
	 * @retval SA_AIS_ERR_INVALID_PARAM pmHandle has not been correctly
	 * initialized. It must be initialized by a successful call to #saPmCInitialize_2.
	 * @retval SA_AIS_ERR_NO_OP Another consumer is active.
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * After PM Consumer Agent is initialized, see #saPmCInitialize_2, it is not yet
	 * active. That is, it will not receive any notifications on aggregated
	 * jobs.
	 *
	 * The call to #saPmCActivate activates PM Consumer Agent.
	 *
	 * In an HA setup with two consumers, one should be active and one should
	 * passive.
	 * @deprecated Since A.04.00
         */
	extern SaAisErrorT saPmCActivate(SaPmCHandleT pmHandle);
	/* @endcond */

	/**
	 * @ingroup PmConsumer
	 * @brief Activate a #SaPmCHandleT
	 *
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 * @param consumerName : [in] name of the consumer
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK The function completed successfully.
	 * @retval SA_AIS_ERR_BAD_HANDLE pmHandle has not been correctly
	 * initialized. It must be initialized by a successful call to #saPmCInitialize_2.
	 * @retval SA_AIS_ERR_BAD_OPERATION pmHandle is already associated with an active consumer.
	 * @retval SA_AIS_ERR_NO_OP Another consumer is active for consumerName.
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * After PM Consumer Agent is initialized, see #saPmCInitialize_2, it is not yet
	 * active. That is, it will not receive any notifications on aggregated
	 * jobs.
	 *
	 * The call to #saPmCActivate_2 activates PM Consumer Agent.
	 *
	 * In an HA setup with two consumer instancess, one should be active and one should
	 * be passive. If consumerName is specified the PM Service will only allow one active instance
	 * in the entire cluster for consumerName. If consumerName is not specified the
	 * PM Service will allow multiple active instances.
         */
	extern SaAisErrorT saPmCActivate_2(SaPmCHandleT pmHandle,
		SaConstStringT consumerName);

	/**
	 * @ingroup PmConsumer
	 * @brief Deactivate a #SaPmCHandleT
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK The function completed successfully.
	 * @retval SA_AIS_ERR_NO_OP The PM Consumer Agent is not active.
	 * @retval SA_AIS_ERR_INVALID_PARAM pmHandle has not been correctly
	 * initialized. It must be initialized by a successful call to #saPmCInitialize_2.
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * Deactivates PM Consumer Agent, compare with #saPmCActivate.
         */
	extern SaAisErrorT saPmCDeactivate(SaPmCHandleT pmHandle);

	/**
	 * @ingroup PmConsumer
	 * @brief Invoke pending callbacks
	 * @param pmHandle : [in] The handle returned by #saPmCInitialize_2.
	 * @param flags : [in] Flags that specify the callback execution behavior.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK The function completed successfully.
	 * @retval SA_AIS_ERR_INVALID_PARAM flags does not have any of the allowed
	 * values
	 *
	 * This function invokes, in the context of the calling thread,
	 * one or all of the pending callbacks for the handle pmHandle.
	 *
	 * Allowed values of @b flags are
	 * @li SA_DISPATCH_ONE, invoke one pending callback
	 * @li SA_DISPATCH_ALL, invoke all pending callbacks
	 *
	 * Note that the enum #SaDispatchFlagsT has more possible values, but
	 * only the two mentioned above are allowed.
	 */
	extern SaAisErrorT saPmCDispatch(SaPmCHandleT pmHandle, SaDispatchFlagsT flags);

	/**
	 * @cond OSAF_PM_API
	 * @ingroup PmConsumer
	 * @deprecated Since A.02.01
	 * @brief Initializes an iterator
	 * @param hdl : [in] Job handler that was part of the argument to the callback.
	 * @param iteratorHandle : [out] Pointer to the iterator handle later used
	 * while iterating.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK The function completed successfully.
	 * @retval SA_AIS_ERR_NOT_EXIST There is no job associated with the handle.
	 * @retval SA_AIS_ERR_UNAVAILABLE The job has expired.
	 * @retval SA_AIS_ERR_INVALID_PARAM Bad value in iteratorOptions.
	 *
	 * This function initializes an iterator for the job.
	 * It is undefined what will happen if this function is called more than
	 * once for a job.
	 */
	extern SaAisErrorT saPmCIteratorInitialize(
		SaPmCJobGranularityPeriodDataT hdl,
		SaPmCIteratorHandleT *iteratorHandle
		);
	/* @endcond */

	/**
	 * @ingroup PmConsumer
	 * @brief Initializes an iterator
	 * @param hdl : [in] Job handler that was part of the argument to the callback.
	 * @param iteratorHandle : [out] Pointer to the iterator handle later used
	 * while iterating.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK The function completed successfully.
	 * @retval SA_AIS_ERR_NOT_EXIST There is no job associated with the handle.
	 * @retval SA_AIS_ERR_UNAVAILABLE The job has expired.
	 * @retval SA_AIS_ERR_INVALID_PARAM Bad value in iteratorOptions.
	 *
	 * This function initializes an iterator for the job.
	 * It is undefined what will happen if this function is called more than
	 * once for a job.
	 */
	extern SaAisErrorT saPmCIteratorInitialize_2(
		SaPmCJobGranularityPeriodDataT hdl,
		SaPmCIteratorHandleT_2 *iteratorHandle
		);

	/**
	 * @cond OSAF_PM_API
	 * @ingroup PmConsumer
	 * @deprecated Since A.02.01
	 * @brief Next node in iteration
	 * @param iteratorHandle : [in] Iterator handle returned by
	 * #saPmCIteratorInitialize.
	 * @param iteratorInfo : [out] Information about the node.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK The function completed successfully.
	 * @retval SA_AIS_ERR_NOT_EXIST Iteration is done, all nodes has been visited.
	 * @retval SA_AIS_ERR_UNAVAILABLE The job has expired.
	 * @retval SA_AIS_ERR_FAILED_OPERATION Unexpected error.
	 * @retval SA_AIS_ERR_TOO_BIG Maximum number of levels has been reached.
	 * @retval SA_AIS_ERR_NO_MEMORY Out of memory.
	 *
	 * This function is used to obtain the next node while iterating.
	 *
	 * Each leaf is visited once. Each node is visited twice, once on the way down
	 * and once on the way up.
	 *
	 * The structure pointed to by #iteratorInfo will be filled in with values
	 * associated with the new node.
	 *
	 * If #saPmCIteratorNext has once failed,
	 * it will always fail again with the same error for the same handle.
	 * After an error has occurred the only valid call for the handle is
	 * #saPmCIteratorFinalize.
	 */
	extern SaAisErrorT saPmCIteratorNext(
		SaPmCIteratorHandleT iteratorHandle,
		SaPmCIteratorInfoT *iteratorInfo
		);
	/* @endcond */

	/**
	 * @ingroup PmConsumer
	 * @brief Next node in iteration
	 * @param iteratorHandle : [in] Iterator handle returned by
	 * #saPmCIteratorInitialize_2.
	 * @param iteratorInfo : [out] Information about the node.
	 * @param values : [out] Pointer to an array of SaPmAggregatedValueT values.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK The function completed successfully.
	 * @retval SA_AIS_ERR_NOT_EXIST Iteration is done, all nodes has been visited.
	 * @retval SA_AIS_ERR_UNAVAILABLE The job has expired.
	 * @retval SA_AIS_ERR_FAILED_OPERATION Unexpected error.
	 * @retval SA_AIS_ERR_TOO_BIG Maximum number of levels has been reached.
	 * @retval SA_AIS_ERR_NO_MEMORY Out of memory.
	 *
	 * This function is used to obtain the next node while iterating.
	 *
	 * Each leaf is visited once. Each node is visited twice, once on the way down
	 * and once on the way up.
	 *
	 * The structure pointed to by iteratorInfo will be filled in with values
	 * associated with the new node.
	 *
	 * If the new node is a leaf, the memory used to return the leaf values is allocated
	 * and will be deallocated at the next invocation of saPmCIteratorNext_2()
	 * or saPmCIteratorFinalize_2().
	 *
	 * If #saPmCIteratorNext_2 has once failed,
	 * it will always fail again with the same error for the same handle.
	 * After an error has occurred the only valid call for the handle is
	 * #saPmCIteratorFinalize_2.
	 */
	extern SaAisErrorT saPmCIteratorNext_2(
		SaPmCIteratorHandleT_2 iteratorHandle,
			SaPmCIteratorInfoT_2 *iteratorInfo, SaPmCAggregatedValuesT *values
		);

	/**
	 * @cond OSAF_PM_API
	 * @ingroup PmConsumer
	 * @deprecated Since A.02.01
	 * @brief Finalizes an iterator
	 * @param iteratorHandle : [in] Iterator handle returned by
	 * #saPmCIteratorInitialize.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK The function completed successfully.
	 * @retval SA_AIS_ERR_FAILED_OPERATION Unexpected error.
	 *
	 * This function finalizes the iteration initialized by a previous call to the
	 * #saPmCIteratorInitialize function.
	 * It frees all memory previously allocated by that iteration.
	 */
	extern SaAisErrorT saPmCIteratorFinalize(
		SaPmCIteratorHandleT iteratorHandle
		);
	/* @endcond */

	/**
	 * @ingroup PmConsumer
	 * @brief Finalizes an iterator
	 * @param iteratorHandle : [in] Iterator handle returned by
	 * #saPmCIteratorInitialize_2.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK The function completed successfully.
	 * @retval SA_AIS_ERR_FAILED_OPERATION Unexpected error.
	 *
	 * This function finalizes the iteration initialized by a previous call to the
	 * #saPmCIteratorInitialize_2 function.
	 * It frees all memory previously allocated by that iteration.
	 */
	extern SaAisErrorT saPmCIteratorFinalize_2(
		SaPmCIteratorHandleT_2 iteratorHandle
		);

	/**
	 * @ingroup PmConsumer
	 * @brief Remove a job after processing
	 * @param hdl : [in] Job handle that was part of the argument to the callback.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK The function completed successfully.
	 * @retval SA_AIS_ERR_NOT_EXIST The job does not exist..
	 * @retval SA_AIS_ERR_UNAVAILABLE The job has expired.
	 *
	 * This function removes the job and frees all allocated memory connected
	 * to it.
	 */
	extern SaAisErrorT saPmCJobGranularityPeriodDataRemove(
		SaPmCJobGranularityPeriodDataT hdl
		);

	/**
	 * @ingroup PmConsumer
	 *
	 * @brief Reads current, non-GP aligned, local counter values.
	 *
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 *
	 * @return SaAisErrorT
	 *
	 * @retval SA_AIS_OK The function completed successfully.
	 * @retval SA_AIS_ERR_INVALID_PARAM pmHandle has not been correctly initialized.
	 * It must be initialized by a successful call to #saPmCInitialize_2.
	 * @retval SA_AIS_ERR_BAD_OPERATION PM Consumer Agent not subscribed for current values job data type.
	 * @retval SA_AIS_ERR_BUSY PM Service is busy with another task for PM Consumer Agent.
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library (such as corruption).
	 *
	 * This function reads the current values, of the resource monitor jobs subscribed to
	 * by the PM Consumer agent, that have been set on the local cluster node.
	 * The values are not GP aligned and are only from the local node.
	 * Global aggregation is not performed. Values are returned to the invoking process
	 * via PM Consumer  event and callback mechanism. The callback function is specified in #saPmCInitialize_2.
	 * The Consumer agent must be subscribed for resource monitor job type, otherwise the operation will fail.
	 * The Consumer agent must be subscribed for current values  job data type, otherwise the operation will fail.
	 *
	 */
	extern SaAisErrorT saPmCCurrentValuesGet(SaPmCHandleT pmHandle);

	/**
	 * @ingroup PmConsumer
	 *
	 * @brief Read current, non-GP aligned, values of a measurement instance.
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
	 *
	 * @retval : SA_AIS_OK The function completed successfully.
	 * @retval : SA_AIS_ERR_BAD_HANDLE pmHandle has not been correctly initialized. It must be initialized by a
	 * successful call to #saPmCInitialize_2.
	 * @retval : SA_AIS_ERR_INVALID_PARAM active job or measurement type not found.
	 * @retval : SA_AIS_ERR_NOT_EXIST instance not found.
	 * @retval : SA_AIS_ERR_TIMEOUT an implement-dependent timeout occurred, or the timeout, specified by timeout
	 * parameter, occurred before the call could complete.
	 * @retval : SA_AIS_ERR_LIBRARY an unexpected problem occurred in the library (such as corruption).
	 *
	 * This function reads the aggregated current values  of MTs associated with an instance. The values are not
	 * GP aligned. If jobNames is NULL the values of MTs associated with the instance, for all active jobs, are
	 * returned. If measurementTypes is NULL the values of all MTs associated with the instance, are returned.
	 *
	 * This function allocates the memory to return the instance values. When the calling process no longer needs
	 * to access the instance values, the memory must be freed by calling the #saPmCInstanceValuesMemoryFree() function.
	 *
	 * If PM Consumer Agent is initialized, see saPmCInitialize_2, but is not active, see saPmCActivate, this function
	 * will return instance values.
	 *
	 */
	extern SaAisErrorT saPmCGetInstanceValues(
			SaPmCHandleT pmHandle,
			SaConstStringT instance,
			SaConstStringT *jobNames,
			SaConstStringT *measurementTypes,
			SaTimeT timeout,
			SaPmCInstanceValuesT ***instanceValues);

	/**
	 * @ingroup PmConsumer
	 *
	 * @brief Free memory  associated with  instance values.
	 *
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 * @param instanceValues : [in] pointer to a NULL-terminated array of pointer to instance values to be freed.
	 *
	 * @return SaAisErrorT
	 *
	 * @retval : SA_AIS_OK The function completed successfully.
	 * @retval : SA_AIS_ERR_INVALID_PARAM pmHandle has not been correctly initialized. It must be initialized by a
	 * successful call to #saPmCInitialize_2.
	 * @retval : SA_AIS_ERR_LIBRARY an unexpected problem occurred in the library (such as corruption).
	 *
	 * This function deallocates the memory that was allocated by a previous call to the #saPmCGetInstanceValues()
	 * function; this deallocation includes:
	 *     - the memory areas containing the job value and aggregated values definitions which are referred to by
	 *       the pointers held in the NULL-terminated array referred to by instanceValues, and
	 *     - the memory of the NULL-terminated array of pointers referred to by instanceValues.
	 *
	 */
	extern SaAisErrorT saPmCInstanceValuesMemoryFree(
			SaPmCHandleT pmHandle, SaPmCInstanceValuesT **instanceValues);

	/**
	 * @ingroup PmConsumer
	 *
	 * @brief Check status of optional API features.
	 *
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 * @param featureName : [in] feature name
	 * @param featureVersion : [in] feature version
	 * @param enabled : [out] enabled status
	 *
	 * @return SaAisErrorT
	 *
	 * @retval : SA_AIS_OK The function completed successfully.
	 * @retval : SA_AIS_ERR_BAD_HANDLE pmHandle has not been correctly initialized. It must be initialized
	 * by a successful call to #saPmCInitialize_2.
	 * @retval : SA_AIS_ERR_INVALID_PARAM feature name or feature version is not defined.
	 * @retval : SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library (such as corruption).
	 *
	 */
	extern SaAisErrorT saPmCIsFeatureEnabled(
			SaPmCHandleT pmHandle,
			SaConstStringT featureName,
			SaConstStringT featureVersion,
			SaBoolT *enabled);

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
	 * @retval : SA_AIS_OK The function completed successfully.
	 * @retval : SA_AIS_ERR_BAD_HANDLE pmHandle has not been correctly initialized. It must be initialized
	 * by a successful call to #saPmCInitialize_2.
	 * @retval : SA_AIS_NOT_SUPPORTED SAPMC_VERSION_2 is not enabled.
	 * @retval : SA_AIS_ERR_INVALID_PARAM pm group or measurement type is not defined.
	 * @retval : SA_AIS_ERR_NOT_EXIST instance not found.
	 * @retval : SA_AIS_ERR_TIMEOUT An implementation-dependent timeout occurred, or the timeout, specified
	 * by the timeout parameter, occurred before the call could complete.
	 * @retval : SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library (such as corruption).
	 *
	 * This function returns names of real-time instance values.
	 *
	 * All matching entries are returned that satisfy the MT, pmGroup and instance selection criteria.
	 * A NULL selection parameter indicates all values of that parameter will used for matching.
	 * If timeout is not a positive number the default value of 3000 ms (3 seconds) is used.
	 *
	 * This function allocates the memory to return the instance names. When the calling process no longer needs
	 * to access the instance names, the memory must be freed by calling #saPmCNamesMemoryFree.
	 *
	 * If PM Consumer Agent is initialized, see #saPmCInitialize_2, this function will return instance names.
	 * It can be called by both active and passive PM Consumer Agents.
	 *
	 */
	extern SaAisErrorT saPmCGetInstanceNames(
			SaPmCHandleT pmHandle,
			SaConstStringT instance,
			SaConstStringT *measurementTypes,
			SaConstStringT *pmGroups,
			SaTimeT timeout,
			SaConstStringT **instanceNames);

	/**
	 * @ingroup PmConsumer
	 *
	 * @brief Return measurement type names associated with real-time instance values.
	 *
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 * @param instance : [in] instance name
	 * @param measurementType : [in] measurement type name, either a NULL string or wildcard filter construct
	 * @param pmGroups : [in] pointer to a NULL-terminated array of pm group names
	 * @param timeout : [in] The saPmCGetMeasurementTypeNames invocation is considered to have failed if it does
	 * not complete by the time specified
	 * @param measurementTypeNames : [out] pointer to a pointer to NULL-terminated array of measurement type names
	 *
	 * @return SaAisErrorT
	 *
	 * @retval : SA_AIS_OK The function completed successfully.
	 * @retval : SA_AIS_ERR_BAD_HANDLE pmHandle has not been correctly initialized. It must be initialized
	 * by a successful call to #saPmCInitialize_2.
	 * @retval : SA_AIS_NOT_SUPPORTED SAPMC_VERSION_2 is not enabled.
	 * @retval : SA_AIS_ERR_INVALID_PARAM pm group or measurement type is not defined.
	 * @retval : SA_AIS_ERR_NOT_EXIST instance not found.
	 * @retval : SA_AIS_ERR_TIMEOUT An implementation-dependent timeout occurred, or the timeout, specified
	 * by the timeout parameter, occurred before the call could complete.
	 * @retval : SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library (such as corruption).
	 *
	 * This function returns measurement type names associated with real-time instance values.
	 *
	 * All matching entries are returned that satisfy the MT, pmGroup and instance selection criteria.
	 * A NULL selection parameter indicates all values of that parameter will used for matching.
	 * If timeout is not a positive number the default value of 3000 ms (3 seconds) is used.
	 *
	 * This function allocates the memory to return the measurement type names. When the calling process no longer needs
	 * to access the measurement type names, the memory must be freed by calling #saPmCNamesMemoryFree.
	 *
	 * If PM Consumer Agent is initialized, see #saPmCInitialize_2, this function will return measurement type names.
	 * It can be called by both active and passive PM Consumer Agents.
	 *
	 */
	extern SaAisErrorT saPmCGetMeasurementTypeNames(
			SaPmCHandleT pmHandle,
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
	 *
	 * @retval : SA_AIS_OK The function completed successfully.
	 * @retval : SA_AIS_ERR_BAD_HANDLE pmHandle has not been correctly initialized. It must be initialized
	 * by a successful call to #saPmCInitialize_2.
	 * @retval : SA_AIS_NOT_SUPPORTED SAPMC_VERSION_2 is not enabled.
	 * @retval : SA_AIS_ERR_INVALID_PARAM pm group or measurement type is not defined.
	 * @retval : SA_AIS_ERR_NOT_EXIST instance not found.
	 * @retval : SA_AIS_ERR_TIMEOUT An implementation-dependent timeout occurred, or the timeout, specified
	 * by the timeout parameter, occurred before the call could complete.
	 * @retval : SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library (such as corruption).
	 *
	 * This function returns PM group names associated with real-time instance values.
	 *
	 * All matching entries are returned that satisfy the MT, pmGroup and instance selection criteria.
	 * A NULL selection parameter indicates all values of that parameter will used for matching.
	 * If timeout is not a positive number the default value of 3000 ms (3 seconds) is used.
	 *
	 * This function allocates the memory to return the PM group names. When the calling process no longer needs
	 * to access the PM group names, the memory must be freed by calling #saPmCNamesMemoryFree.
	 *
	 * If PM Consumer Agent is initialized, see #saPmCInitialize_2, this function will return PM group names.
	 * It can be called by both active and passive PM Consumer Agents.
	 *
	 */
	extern SaAisErrorT saPmCGetPmGroupNames(
			SaPmCHandleT pmHandle,
			SaConstStringT instance,
			SaConstStringT *measurementTypes,
			SaConstStringT pmGroup,
			SaTimeT timeout,
			SaConstStringT **pmGroupNames);

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
	 *
	 * @retval : SA_AIS_OK instanceValues are returned
	 * @retval : SA_AIS_ERR_TRY_AGAIN instanceValues are not yet available. Try again shortly
	 * @retval : SA_AIS_ERR_BAD_HANDLE pmHandle has not been correctly initialized. It must be initialized
	 * by a successful call to #saPmCInitialize_2.
	 * @retval : SA_AIS_NOT_SUPPORTED SAPMC_VERSION_2 is not enabled.
	 * @retval : SA_AIS_ERR_UNAVAILABLE In-progress operation associated with requestId is not found
	 * @retval : SA_AIS_ERR_INVALID_PARAM pm group or measurement type is not defined
	 * @retval : SA_AIS_ERR_NOT_EXIST instance not found.
	 * @retval : SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library (such as corruption).
	 *
	 * This function reads the real-time instance values. The values are not aggregated nor GP aligned.
	 *
	 * All matching entries are returned that satisfy the MT, pmGroup and instance selection criteria.
	 * A NULL selection parameter indicates all values of that parameter will used for matching.
	 * If timeout is not a positive number the default value of 100 ms is used.
	 *
	 * requestId is used internally to maintain context between on-going operations by multiple PM Consumer Agents.
	 * On the first call to #saPmCGetInstanceValues _2, requestId should equal zero.
	 * If timeout occurs the operation continues; the return value is SA_AIS_ERR_TRY_AGAIN and requestId is returned.
	 * In subsequent calls requestId should be unchanged since the previous call.
	 * The operation is complete when the return value is not SA_AIS_ERR_TRY_AGAIN.
	 * An on-going operation can be cancelled by #saPmCCancelGetInstanceValues_2.
	 *
	 * If return value is SA_AIS_ERR_INVALID_PARAM errorText is returned, specifying which parameter is not
	 * defined, eg "Invalid value '<value>' for <'PmGroup' or 'MeasurementType'>".
	 *
	 * This function allocates the memory to return the instance values or error string. When the calling process
	 * no longer needs to access the instance values or error string, the memory must be freed by calling
	 * #saPmCInstanceValuesMemoryFree_2.
	 *
	 * If PM Consumer Agent is initialized, see #saPmCInitialize_2, this function will return instance values.
	 * It can be called by both active and passive PM Consumer Agents.
	 *
	 */
	extern SaAisErrorT saPmCGetInstanceValues_2(
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
	 * @brief Cancel in-progress #saPmCGetInstanceValues_2 operation.
	 *
	 * @param pmHandle : [in] the handle returned by #saPmCInitialize_2
	 * @param requestId : [in] the requestId returned by #saPmCGetInstanceValues_2
	 *
	 * @return SaAisErrorT
	 *
	 * @retval : SA_AIS_OK The function completed successfully.
	 * @retval : SA_AIS_ERR_BAD_HANDLE pmHandle has not been correctly initialized. It must be initialized
	 * by a successful call to #saPmCInitialize_2.
	 * @retval : SA_AIS_NOT_SUPPORTED SAPMC_VERSION_2 is not enabled.
	 * @retval : SA_AIS_ERR_UNAVAILABLE In-progress operation associated with requestId is not found
	 * @retval : SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library (such as corruption).
	 *
	 * This function cancels the in-progress saPmCGetInstanceValues_2 operation associated with requestId.
	 *
	 */
	extern SaAisErrorT saPmCCancelGetInstanceValues_2(
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
	 *
	 * @retval : SA_AIS_OK The function completed successfully.
	 * @retval : SA_AIS_ERR_BAD_HANDLE pmHandle has not been correctly initialized. It must be initialized
	 * by a successful call to #saPmCInitialize_2.
	 * @retval : SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library (such as corruption).
	 *
	 * This function deallocates the memory that was allocated by a previous call to #saPmCGetInstanceNames,
	 * #saPmCGetMeasurementTypeNames or #saPmCGetPmGroupNames.
	 *
	 */
	extern SaAisErrorT saPmCNamesMemoryFree(
			SaPmCHandleT pmHandle,
			SaConstStringT *names);

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
	 *
	 * @retval : SA_AIS_OK The function completed successfully.
	 * @retval : SA_AIS_ERR_BAD_HANDLE pmHandle has not been correctly initialized. It must be initialized
	 * by a successful call to #saPmCInitialize_2.
	 * @retval : SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library (such as corruption).
	 *
	 * This function deallocates the memory that was allocated by a previous call to #saPmCGetInstanceValues_2.
	 *
	 */
	extern SaAisErrorT saPmCInstanceValuesMemoryFree_2(
			SaPmCHandleT pmHandle,
			SaPmCInstanceValuesT_2 **instanceValues,
			SaConstStringT errorText);

#ifdef	__cplusplus
}
#endif

#endif	/* SAPMCONSUMER_H */

