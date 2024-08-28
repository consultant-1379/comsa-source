#ifndef ComOamSpiPmMeasurements_4_h__
#define ComOamSpiPmMeasurements_4_h__

#include <stdint.h>
#include <stdbool.h>
#include <MafMgmtSpiCommon.h>
#include <MafMgmtSpiInterface_1.h>

/**
 *******************************************************************************************
 * @file ComOamSpiPmMeasurements_4.h
 * @brief Performance Management SPI to provide real-time access to measurement values.
 *
 ********************************************************************************************/


/**
 * The value of a measurement.
 */
typedef struct ComOamSpiPmMeasurementValue_4 {
	/**
	 * Set to True if the counter value is unreliable for some reason
	 */
	bool isSuspect;

	union {
		double floatVal;
		int64_t intVal;
	} value;
} ComOamSpiPmMeasurementValue_4T;

/**
 * The value type of a measurement.
 */
typedef enum ComOamSpiPmMeasurementValueType_4 {
	ComOamSpiPmMeasurementValueType_4_INT = 0,
	ComOamSpiPmMeasurementValueType_4_FLOAT = 1
} ComOamSpiPmMeasurementValueType_4T;

/**
 * A struct for a measurement filter
 * Used as an input container for getMeasurementValues/getMeasuredObjects/getPmGroupIds/getPmJobIds/getMeasurementNames
 */
typedef struct ComOamSpiPmMeasurementFilter_4 {

	/**
	 * A NULL terminated string (or) filter construct with wildcard(*) containing the measured object name to match.
	 * NULL means that all measuredObjects is a match
	 */
	const char* measuredObject;

	/**
	 * A NULL terminated string (or) filter construct with wildcard(*) containing the job ID to match.
	 * NULL means that all job IDs is a match.
	 */
	const char* jobId;

	/**
	 * A NULL terminated pointer array with the names of the group ID.
	 * Each name is a NULL terminated string (or) filter construct with wildcard(*).
	 * Only group IDs in this is a match.
	 * NULL means that all group IDs is a match.
	 * For getPmGroupIds(), the first element of the NULL terminated pointer
	 * array would be considered as input.
	 */
	const char** groupIds;

	/**
	 * A NULL terminated pointer array with the names of the measurement types.
	 * Each name is a NULL terminated string (or) filter construct with wildcard(*).
	 * Only measurement types in this is a match.
	 * NULL means that all measurement types is a match.
	 * For getMeasurementNames(), the first element of the NULL terminated
	 * pointer array would be considered as input.
	 */
	const char** measurementTypes;

} ComOamSpiPmMeasurementFilter_4T;

/**
 * A container struct for a measurement
 */
typedef struct ComOamSpiPmMeasurement_4 {

	/**
	 * The measurement type name as defined in ECIM PM.
	 */
	const char* measurementTypeName;

	/**
	 * The measurement object name.
	 */
	const char* measuredObject;

	/**
	 * The ID of the PmJob. If no PM job is active, this shall be NULL.
	 */
	const char* jobId;

	/**
	 * The ID of the PmGroup. If no PM group is configured, this shall be NULL.
	 */
	const char* groupId;

	/**
	 * The measurement value(s). This may be NULL if there are no values (nrOfValues == 0).
	 */
	ComOamSpiPmMeasurementValue_4T* value;

	/**
	 * The type of value, that is int or float.
	 */
	ComOamSpiPmMeasurementValueType_4T valueType;

	/**
	 * This is the length of the value array.
	 */
	uint32_t nrOfValues;

	/**
	 * If some value(s) from a measurement was not possible to retrieve, it is possible to return an error message which
	 * will be shown to the end user. If there is no error information, this shall be NULL.
	 */
	const char* errorInformation;

	/**
	 * the GP in seconds. If the counter is not from a PM job, this shall be 0
	 */
	uint32_t gpInSeconds;

} ComOamSpiPmMeasurement_4T;


/**
 * A struct containing all measurements associated to a measured object
 */
typedef struct ComOamSpiPmMeasuredObjectMeasurements_4 {

	/**
	 * An array of measurements related to this measured object. This may be NULL if there are no measurements (nrOfMeasurements == 0)
	 */
	ComOamSpiPmMeasurement_4T* measurements;

	/**
	 * The length of the measurements array
	 */
	uint32_t nrOfMeasurements;

	/**
	 * This function shall be invoked by the caller of the SPI functions to release allocated data in this container.
	 * The container itself shall not be released by this function.
	 * The pointer to the function is provided by the implementer of the SPI.
	 * The function pointer may be set to NULL within the release function.
	 * @param measuredObjectMeasurements The pointer to the container to release.
	 */
	void (*release)(struct ComOamSpiPmMeasuredObjectMeasurements_4* measuredObjectMeasurements);

} ComOamSpiPmMeasuredObjectMeasurements_4T;

/**
 * A struct containing array of measured objects/measurement types/job Ids/group Ids
 */
typedef struct ComOamSpiPmMeasurementOutput_4 {
	/**
	 * A NULL terminated array of measured objects/measurement types/PmJob IDs/PmGroup IDs.
	 * Each name is a NULL terminated ASCII string.
	 */
	const char** names;

	/**
	 * This function shall be invoked by the caller of the SPI functions to release allocated data in this container.
	 * The container itself shall not be released by this function.
	 * The pointer to the function is provided by the implementer of the SPI.
	 * The function pointer may be set to NULL within the release function.
	 * @param measurementOutput The pointer to the container to release.
	 */
	void (*release)(struct ComOamSpiPmMeasurementOutput_4* measurementOutput);


} ComOamSpiPmMeasurementOutput_4T;

/**
 * A struct containing description for a PmGroup ID.
 */
typedef struct ComOamSpiPmGroupIdDescription_4 {

	/**
	 * A NULL terminated string containing the description of the PmGroup ID.
	 */
	const char* description;

	/**
	 * This function shall be invoked by the caller of the SPI functions to release allocated data in this container.
	 * The container itself shall not be released by this function.
	 * The pointer to the function is provided by the implementer of the SPI.
	 * The function pointer may be set to NULL within the release function.
	 * @param groupIdDescription The pointer to the container to release.
	 */
	void (*release)(struct ComOamSpiPmGroupIdDescription_4* groupIdDescription);

} ComOamSpiPmGroupIdDescription_4T;

/**
 * A container struct for a measurement type description
 */
typedef struct ComOamSpiPmMeasurementTypeInfo_4 {

	/**
	 * A NULL terminated string containing the PmGroup ID defined in ECIM PM.
	 */
	const char* groupId;

	/**
	 *  A NULL terminated string containing the description of the measurement type name.
	 */
	const char* description;

} ComOamSpiPmMeasurementTypeInfo_4T;

/**
 * A struct containing measurement type information for the measurement type name  in different pmGroup IDs.
 */
typedef struct ComOamSpiPmMeasurementTypeDescription_4 {

	/**
	 * An array of measurement type information. This may be NULL if there are no measurements (nrOfValues == 0)
	 */
	ComOamSpiPmMeasurementTypeInfo_4T* measurementTypeInfo;

	/**
	 * The length of the measurementTypeInfo array
	 */
	uint32_t nrOfValues;

	/**
	 * This function shall be invoked by the caller of the SPI functions to release allocated data in this container.
	 * The container itself shall not be released by this function.
	 * The pointer to the function is provided by the implementer of the SPI.
	 * The function pointer may be set to NULL within the release function.
	 * @param measurementTypeDescription The pointer to the container to release.
	 */
	void (*release)(struct ComOamSpiPmMeasurementTypeDescription_4* measurementTypeDescription);

} ComOamSpiPmMeasurementTypeDescription_4T;

/**
 * PM measurement interface. Implemented by the SA to provide real-time access to measurement values.
 */
typedef struct ComOamSpiPmMeasurements_4 {

	/**
	 * Common interface description.
	 * The "base class" for this interface contains the
	 * component name, interface name, and interface version.
	 */
	MafMgmtSpiInterface_1T base;

	/**
	 * This function returns real-time PM measurement values that fulfill the selection criteria.
	 *
	 * @param [in] filter
	 * A filter for filtering of which data to be fetched.
	 *
	 * @param [in] isVerbose
	 * true means that verbose information is requested. This implies that also measurement types
	 * that have no values should be returned.
	 *
	 * @param [out] measurements
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
	MafReturnT (*getMeasurementValues)(const ComOamSpiPmMeasurementFilter_4T* filter, const bool isVerbose,  ComOamSpiPmMeasuredObjectMeasurements_4T* measurements);

	/**
	 * This function aborts  the in progress getMeasurementValues() operation.
	 * This shall  invoke on the same thread where getMeasurementValues() is invoked and it returns MafTryAgain.
	 *
	 * @return MafOk if the call was successful.
	 * MafFailure if an internal error occurred.
	 */
	MafReturnT (*cancelGetMeasurementValues)();

	/**
	 * This function returns a list of measurement type names that fulfill the selection criteria.
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
	MafReturnT (*getMeasurementNames)(const ComOamSpiPmMeasurementFilter_4T* filter, ComOamSpiPmMeasurementOutput_4T* measurementTypes);

	/**
	 * This function returns a list of active PM job IDs that fulfill the selection criteria.
	 *
	 * @param [in] filter
	 * A filter for filtering of which data to be fetched.
	 *
	 * @param [out] jobIds
	 * The result container contains:
	 * An array of PM job IDs associated to the measured object.
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
	 * MafInvalidArgument if PmGroup ID, measurement type name or PmJob ID input is not according to the PM model
	 */
	MafReturnT (*getPmJobIds)(const ComOamSpiPmMeasurementFilter_4T* filter, ComOamSpiPmMeasurementOutput_4T* jobIds);

	/**
	 * This function returns a list of PM Group IDs that fulfill the selection criteria.
	 *
	 * @param [in] filter
	 * A filter for filtering of which data to be fetched.
	 *
	 * @param [out] groupIds
	 * The result container contains:
	 * An array of active PM Groups associated to the measured object.
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
	MafReturnT (*getPmGroupIds)(const ComOamSpiPmMeasurementFilter_4T* filter, ComOamSpiPmMeasurementOutput_4T* groupIds);

	/**
	 * This function returns a list of measured objects that fulfill the selection criteria.
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
	MafReturnT (*getMeasuredObjects)(const ComOamSpiPmMeasurementFilter_4T* filter, ComOamSpiPmMeasurementOutput_4T* measuredObjects);

	/**
	 * This function returns a list of descriptions for the measurement types that fulfill the selection criteria.
	 *
	 * @param [in] measurementType
	 * A NULL terminated string containing the Measurement Type name defined in ECIM PM.

	 *
	 * @param [in] groupIds
	 * A list of groupIds from which the measurement types should be searched for description attribute.
	 * A NULL terminated pointer array with the names of the PM Group ID.
	 * Each name is a NULL terminated string.
	 * NULL means that all PM Group ID is a match.
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
	MafReturnT (*getMeasurementTypeDescription)(const char* measurementType, const char** groupIds, ComOamSpiPmMeasurementTypeDescription_4T* description);

	/**
	 * This function returns the description for PM Group ID given by caller.
	 *
	 * @param [in] groupId
	 * A NULL terminated string containing the PM Group ID defined in ECIM PM.
	 *
	 * @param [out] description
	 * The result container contains:
	 * A PM group description associated with the input PM Group ID.
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
	MafReturnT (*getGroupIdDescription)(const char* groupId, ComOamSpiPmGroupIdDescription_4T* description);

} ComOamSpiPmMeasurements_4T;

#endif // ComOamSpiPmMeasurements_4_h__
