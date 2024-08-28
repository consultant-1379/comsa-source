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
 * File: PmShowCounters.hxx
 *
 * Implementation of ComOamSpiPmMeasurements_3.h and ComOamSpiPmMeasurements_4.h
 *
 * Author: xnikvap 2015-08-27
 *
 * Reviewed:
 * Modified: xavikon 6-2-2017 (to implement ComOamSpiPmMeasurements_4.h)
 *
 ************************************************************************** */

#ifndef PMSHOWCOUNTERS_HXX_
#define PMSHOWCOUNTERS_HXX_

#include "ComOamSpiPmMeasurements_3.h"
#include "ComOamSpiPmMeasurements_4.h"
#include "PmtSaTrace.hxx"
#include "MafMgmtSpiCommon.h"
#include "PerfMgmtTransferSA.hxx"
#include "saname_utils.h"
#include "saPmConsumer.h"
#include "saImmOm.h"
#include "PmRunnable.hxx"
#include "PmComComponent.hxx"
#include "saAis.h"
#include "ComSA.h"
#include <string>
#include <vector>
#include <algorithm>
#include <pthread.h>
#include <map>

/**
 * @file PmShowCounters.hxx
 *
 * @ingroup PmtSa
 *
 * @brief This class implements the COM PM Measurement SPI that PMTSA provides for COM.
 *
 * The interface ComOamSpiPmMeasurements_3 and ComOamSpiPmMeasurements_4 provides real-time access to measurement values.
 */

#define PM_JOB_FILTER              2

namespace PmtSa {

/**
 * @ingroup PmtSa
 *
 * @brief This class holds the COM PM Measurements SPI.
 */
class PmShowCounters {
public:
	/**
	 * @ingroup PmtSa
	 *
	 * Constructor
	 */
	PmShowCounters();

	/**
	 * @ingroup PmtSa
	 *
	 * Destructor
	 */
	virtual ~PmShowCounters();

	/**
	 * @ingroup PmtSa
	 *
	 * Returns a pointer to the singleton instance of this class.
	 *
	 * @return	PmShowCounters*		A pointer to the singleton instance.
	 *
	 */
	static PmShowCounters* instance();

	typedef std::map<pthread_t, unsigned int> requestMapT;
	typedef std::map<pthread_t, unsigned int>::iterator requestMapIterT;
	typedef enum {
		GET_REQUEST_ID = 1,
		UPDATE_REQUEST_ID = 2,
		ERASE_REQUEST_ID = 3
	} requestIdOperationT;

	// ------------------------------------------------------------
	// Required as part of the ComOamSpiPmMeasurements_3 interface
	// ------------------------------------------------------------

	/**
	 * @ingroup PmtSa
	 *
	 * This function returns PM measurement names and values associated to a measured object.
	 *
	 * @param [in] dn MO distinguished name in 3GPP format.
	 *			   This is the DN of the MO providing the values.
	 *			   Example: "ManagedElement=1,MoWithCounters=2"
	 *
	 * @param [in] filter
	 *			A filter for filtering of which data to be fetched.
	 *
	 * @param [in|out] measuredObject
	 *
	 *		   The result container contains:
	 *			   An array of measurements associated to this measured object
	 *			   The current value of these measurements.
	 *
	 *		   The result container should be allocated by the caller.
	 *		   The content of the container is allocated by the callee.
	 *		   The allocated memory for the content is released via the release function in the container.
	 *		   The container itself shall be released by the caller.
	 *		   The caller shall invoke the release function if set.
	 *		   The release function shall be provided by the SPI implementation.
	 *		   The implementer may set the function pointer to NULL to indicate that there is nothing to release.
	 *
	 * @return MafOk if the call was successful
	 *		   MafNotExist if a measured object does not exist
	 *		   MafFailure if an internal error occurred
	 */
	MafReturnT getMeasurementValues(const char * dn,
			const ComOamSpiPmMeasurementFilter_3T* filter,
			ComOamSpiPmMeasuredObject_3T * measuredObject);

	/**
	 * @ingroup PmtSa
	 *
	 * This function returns all possible PM measurement names associated to a measured object.
	 *
	 * @param [in] dn MO distinguished name in 3GPP format. Example: "ManagedElement=1,MoWithCounters=2"
	 *
	 * @param [in|out] measurementNames
	 *
	 *		   The result container contains:
	 *			   An array of all possible measurement names associated to this measured object
	 *
	 *		   The result container should be allocated by the caller.
	 *		   The content of the container is allocated by the callee.
	 *		   The allocated memory for the content is released via the release function in the container.
	 *		   The container itself shall be released by the caller.
	 *		   The caller shall invoke the release function if set.
	 *		   The release function shall be provided by the SPI implementation.
	 *		   The implementer may set the function pointer to NULL to indicate that there is nothing to release.
	 *
	 * @return MafOk if the call was successful
	 *		   MafNotExist if a measured object does not exist
	 *		   MafFailure if an internal error occurred
	 */
	MafReturnT getMeasurementNames(const char * dn, ComOamSpiPmMeasuredObjectMeasurementNames_3T* measurementNames);

	/**
	 * @ingroup PmtSa
	 *
	 * This function returns all PM measurement job IDs associated to a measured object.
	 *
	 * @param [in] dn MO distinguished name in 3GPP format. Example: "ManagedElement=1,MoWithCounters=2"
	 *
	 * @param [in|out] measurementJobIds
	 *
	 *		   The result container contains:
	 *			   An array of all PM job IDs associated to this measured object
	 *
	 *		   The result container should be allocated by the caller.
	 *		   The content of the container is allocated by the callee.
	 *		   The allocated memory for the content is released via the release function in the container.
	 *		   The container itself shall be released by the caller.
	 *		   The caller shall invoke the release function if set.
	 *		   The release function shall be provided by the SPI implementation.
	 *		   The implementer may set the function pointer to NULL to indicate that there is nothing to release.
	 *
	 * @return MafOk if the call was successful
	 *		   MafNotExist if a measured object does not exist
	 *		   MafFailure if an internal error occurred
	 */
	MafReturnT getPmJobIds(const char * dn, ComOamSpiPmMeasuredObjectJobIds_3T* measurementJobIds);

	// ------------------------------------------------------------
	// Required as part of the ComOamSpiPmMeasurements_4 interface
	// ------------------------------------------------------------

	/**
	 * This function returns active PM measurement names and values associated to a measured object.
	 *
	 * @param [in] filter
	 * A filter for filtering of which data to be fetched.
	 *
	 * @param [in] isVerbose
	 * true means that verbose information is requested. This implies that also measurement types
	 * that have no values should be returned.
	 *
	 * @param [in|out] measurements
	 * The result container contains:
	 * An array of real-time measurement values associated to this measured object
	 *
	 * The result container should be allocated by the caller.
	 * The content of the container is allocated by the callee.
	 * The allocated memory for the content is released via the release function in the container.
	 * The container itself shall be released by the caller.
	 * The caller shall invoke the release function if set.
	 * The release function shall be provided by the SPI implementation.
	 * The implementer may set the function pointer to NULL to indicate that there is nothing to release.
	 *
	 * @return MafOk if the call was successful.
	 * MafNotExist if a measured object does not exist.
	 * MafTryAgain if the operation was started, but the same method needs to be called again shortly to get the results.
	 * The operation is complete when the return value is not MafTryAgain. An on-going operation can be cancelled by cancelGetMeasurementValues().
	 * MafFailure if an internal error occurred.
	 * MafInvalidArgument if PmGroup ID, measurement type name or PmJob ID input is not according to the PM model. The implementer shall send the
	 * error message on what parameters are invalid using thread context interface.
	 */
	MafReturnT getMeasurementValues_2 (const ComOamSpiPmMeasurementFilter_4T* filter,
			const bool isVerbose,
			ComOamSpiPmMeasuredObjectMeasurements_4T* measurements);

	/**
	 * This function aborts  the in progess getMeasurementValues() opearation.
	 * This shall  invoke on the same thread where getMeasurementValues() is invoked
	 * and returns MafTryAgain
	 *
	 * @return MafOk if the call was successful.
	 * MafFailure if an internal error occurred.
	 */
	MafReturnT cancelGetMeasurementValues();

	/**
	 * This function returns a list of all measurement type names that fulfill the selection criteria.
	 *
	 * @param [in] filter
	 * A filter for filtering of which data to be fetched.
	 *
	 * @param [out] measurementTypes
	 * The result container contains:
	 * An array of all possible measurement names associated to this measured object
	 *
	 * The result container should be allocated by the caller.
	 * The content of the container is allocated by the callee.
	 * The allocated memory for the content is released via the release function in the container.
	 * The container itself shall be released by the caller.
	 * The caller shall invoke the release function if set.
	 * The release function shall be provided by the SPI implementation.
	 * The implementer may set the function pointer to NULL to indicate that there is nothing to release.
	 *
	 * @return MafOk if the call was successful.
	 * MafNotExist if a measured object does not exist.
	 * MafTimeOut if the operation is aborted after the timeout period(~500 ms).
	 * MafFailure if an internal error occurred.
	 * MafInvalidArgument if PmGroup ID, measurement type name or PmJob ID input is not according to the PM model.
	 */
	MafReturnT getMeasurementNames_2(const ComOamSpiPmMeasurementFilter_4T* filter, ComOamSpiPmMeasurementOutput_4T* measurementTypes);

	/**
	 * This function returns a list of PM Group IDs associated to a measured object that fulfill the selection criteria.
	 *
	 * @param [in] filter
	 * A filter for filtering of which data to be fetched.
	 *
	 * @param [out] groupIds
	 * The result container contains:
	 * An array of active Pm Groups associated to the measured object.
	 *
	 * The result container should be allocated by the caller.
	 * The content of the container is allocated by the callee.
	 * The allocated memory for the content is released via the release function in the container.
	 * The container itself shall be released by the caller.
	 * The caller shall invoke the release function if set.
	 * The release function shall be provided by the SPI implementation.
	 * The implementer may set the function pointer to NULL to indicate that there is nothing to release.
	 *
	 * @return MafOk if the call was successful.
	 * MafNotExist if a measured object does not exist.
	 * MafTimeOut if the operation is aborted after the timeout period(~500 ms).
	 * MafFailure if an internal error occurred.
	 * MafInvalidArgument if PmGroup ID, measurement type name or PmJob ID input is not according to the PM model.
	 */
	MafReturnT getPmGroupIds(const ComOamSpiPmMeasurementFilter_4T* filter,ComOamSpiPmMeasurementOutput_4T* groupIds);

	/**
	 * This function returns a list of the MO IDs or LDNs associated to a measured object that fulfill the selection criteria.
	 *
	 * @param [in] filter
	 * A filter for filtering of which data to be fetched.
	 *
	 * @param [out] measuredObjects
	 * The result container contains:
	 * An array of measured objects.
	 *
	 * The result container should be allocated by the caller.
	 * The content of the container is allocated by the callee.
	 * The allocated memory for the content is released via the release function in the container.
	 * The container itself shall be released by the caller.
	 * The caller shall invoke the release function if set.
	 * The release function shall be provided by the SPI implementation.
	 * The implementer may set the function pointer to NULL to indicate that there is nothing to release.
	 *
	 * @return MafOk if the call was successful.
	 * MafNotExist if a measured object does not exist.
	 * MafTimeOut if the operation is after the timeout period(~500 ms).
	 * MafFailure if an internal error occurred.
	 * MafInvalidArgument if PmGroup ID, measurement type name or PmJob ID input is not according to the PM model.
	 */
	MafReturnT getMeasuredObjects(const ComOamSpiPmMeasurementFilter_4T* filter, ComOamSpiPmMeasurementOutput_4T* measuredObjects);

	/**
	 * This function returns a list of descriptions for the Pm Group ID given by caller.
	 *
	 * @param [in] groupId
	 * A NULL terminated string containing the Pm Group ID defined in ECIM PM.
	 *
	 * @param [out] description
	 * The result container contains:
	 * A Pm group description associated with the input Pm Group ID.
	 *
	 * The result container should be allocated by the caller.
	 * The content of the container is allocated by the callee.
	 * The allocated memory for the content is released via the release function in the container.
	 * The container itself shall be released by the caller.
	 * The caller shall invoke the release function if set.
	 * The release function shall be provided by the SPI implementation.
	 * The implementer may set the function pointer to NULL to indicate that there is nothing to release.
	 *
	 * @return MafOk if the call was successful
	 * MafNotExist if a group Id  is NULL or does not exist
	 * MafFailure if an internal error occurred
	 */
	MafReturnT getGroupIdDescription(const char* groupId, ComOamSpiPmGroupIdDescription_4T* description);

	/**
	 * This function returns a list of descriptions for the Measurement Types that fulfill the selection criteria.
	 *
	 * @param [in] measurementType
	 * A NULL terminated string containing the Measurement Type name defined in ECIM PM.

	 *
	 * @param [in] groupIds
	 * A list of groupIds from which the measurement types should be searched for description attribute.
	 * A NULL terminated pointer array with the names of the Pm Group ID.
	 * Each name is a NULL terminated string.
	 * NULL means that all Pm Group IDs is a match.
	 *
	 * param [out] description
	 * The result container contains:
	 * An array of measurement type data associated with the input measurement types(and group Ids)
	 *
	 * The result container should be allocated by the caller.
	 * The content of the container is allocated by the callee.
	 * The allocated memory for the content is released via the release function in the container.
	 * The container itself shall be released by the caller.
	 * The caller shall invoke the release function if set.
	 * The release function shall be provided by the SPI implementation.
	 * The implementer may set the function pointer to NULL to indicate that there is nothing to release.
	 *
	 * @return MafOk if the call was successful
	 * MafNotExist if a measurementType does not exist
	 * MafFailure if an internal error occurred
	 */
	MafReturnT getMeasurementTypeDescription(const char* measurementType, const char** groupIds, ComOamSpiPmMeasurementTypeDescription_4T* description);

private:

	// Pointer to self
	static PmShowCounters* s_instance;

	/**
	 * Map that contains the thread object associated with the session in which SPI call is invoked
	 * against the requestId returned by CoreMw API.
	 *
	 * This Map only contains data of pending requests i.e., requestId of calls in which
	 * CoreMW API returned SA_AIS_TRY_AGAIN code.
	 *
	 * The data will be retained until one of the events occur:
	 * 1. CoreMW returns any error code other than SA_AIS_TRY_AGAIN for the same requestId.
	 * 2. COM NBI(CLI) calls cancelGetMeasurementValues against a certain request associated
	 *    with a session.
	 *
	 * Access to _requestMap through multiple threads is controlled by _requestMapMutex.
	 */
	requestMapT _requestMap;
	pthread_mutex_t _requestMapMutex;

	ComMgmtSpiInterfacePortal_1T* _portal;

	//utility to modify/access requestMap data depending on requestIdOperationT.
	MafReturnT executeRequestIdOperation(requestIdOperationT operation, SaUint32T& operand);

	//utility function to duplicate a const char** to SaConstStringT* and vice-versa.
	const char** duplicateConstCharArray(const char** input);

	//utility to trim off 'ManagedElement=1' from the given input - measuredObject
	const char* trimMeasuredObject(const char* input);

	//utility to get the value of description attribute
	std::string getDescription(SaImmAttrValuesT_2 **attributes);

	//utility to construct the DN ex : measuerementTypeId=CcMT-1,pmGroupId=CcGroup1,CmwPmpmId=1
	std::string getCompleteDn (const char* measurementTypeValue, const char* pmGroupIdValue);

	// utility to extract the pmGroupId value from the complete DN
	std::string getGroupIdValue (std::string objectName);
};

} /* namespace PmtSa */


/**
 * C-interface for PmtSa::PmShowCounters::getMeasurementValues()
 */
extern "C" MafReturnT pmtsa_getMeasurementValues(const char * dn, const ComOamSpiPmMeasurementFilter_3T* filter, ComOamSpiPmMeasuredObject_3T * measuredObject);

/**
 * C-interface for PmtSa::PmShowCounters::getMeasurementNames()
 */
extern "C" MafReturnT pmtsa_getMeasurementNames(const char * dn, ComOamSpiPmMeasuredObjectMeasurementNames_3T* measurementNames);

/**
 * C-interface for PmtSa::PmShowCounters::getPmJobIds()
 */
extern "C" MafReturnT pmtsa_getPmJobIds(const char * dn, ComOamSpiPmMeasuredObjectJobIds_3T* measurementJobIds);

/**
 * C-interface for PmtSa::PmShowCounters::getMeasurementValues_2()
 */
extern "C" MafReturnT pmtsa_getMeasurementValues_2(const ComOamSpiPmMeasurementFilter_4T* filter, const bool isVerbose, ComOamSpiPmMeasuredObjectMeasurements_4T* measurements);

/**
 * C-interface for PmtSa::PmShowCounters::cancelGetMeasurementValues()
 */
extern "C" MafReturnT pmtsa_cancelGetMeasurementValues();
/**
 * C-interface for PmtSa::PmShowCounters::getMeasurementNames_2()
 */
extern "C" MafReturnT pmtsa_getMeasurementNames_2(const ComOamSpiPmMeasurementFilter_4T* filter, ComOamSpiPmMeasurementOutput_4T* measurementTypes);

/**
 * C-interface for PmtSa::PmShowCounters::getPmJobIds_2()
 */
extern "C" MafReturnT pmtsa_getPmJobIds_2(const ComOamSpiPmMeasurementFilter_4T* filter, ComOamSpiPmMeasurementOutput_4T* jobIds);
/**
 * C-interface for PmtSa::PmShowCounters::getPmGroupIds()
 */
extern "C" MafReturnT pmtsa_getPmGroupIds(const ComOamSpiPmMeasurementFilter_4T* filter, ComOamSpiPmMeasurementOutput_4T* groupIds);

/**
 * C-interface for PmtSa::PmShowCounters::getMeasuredObjects()
 */
extern "C" MafReturnT pmtsa_getMeasuredObjects(const ComOamSpiPmMeasurementFilter_4T* filter, ComOamSpiPmMeasurementOutput_4T* measuredObjects);

/**
 * C-interface for PmtSa::PmShowCounters::getMeasurementTypeDescription()
 */
extern "C" MafReturnT pmtsa_getMeasurementTypeDescription(const char* measurementType, const char** groupIds, ComOamSpiPmMeasurementTypeDescription_4T* description);

/**
 * C-interface for PmtSa::PmShowCounters::getGroupIdDescription()
 */
extern "C" MafReturnT pmtsa_getGroupIdDescription(const char* groupId, ComOamSpiPmGroupIdDescription_4T* description);

extern "C" void releaseMeasuredObject(struct ComOamSpiPmMeasuredObject_3* measuredObject);

extern "C" void releaseMeasuredObjectMeasurements(ComOamSpiPmMeasuredObjectMeasurements_4T* measurements);

extern "C" void releaseMeasurementOutput(ComOamSpiPmMeasurementOutput_4T* measurementOutput);

extern "C" void releaseSaConstStringArray(SaConstStringT* array);

extern "C" void releaseSaConstString(SaConstStringT string);

extern "C" void releaseGroupIdDescription(ComOamSpiPmGroupIdDescription_4* description);

extern "C" void releaseMeasurementTypeDescription(ComOamSpiPmMeasurementTypeDescription_4* description);

#endif /* PMSHOWCOUNTERS_HXX_ */
