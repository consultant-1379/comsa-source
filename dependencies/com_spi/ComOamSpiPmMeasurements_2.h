
#ifndef ComOamSpiPmMeasurements_2_h__
#define ComOamSpiPmMeasurements_2_h__

#include <stdint.h>
#include <stdbool.h>
#include <MafMgmtSpiCommon.h>
#include <MafMgmtSpiInterface_1.h>

/**
*******************************************************************************************
* @file ComOamSpiPmMeasurements_2.h
* @brief Performance Management SPI to provide real-time access to measurement values.
*
*
********************************************************************************************/


/**
* The value of a measurement.
*/
typedef struct ComOamSpiPmMeasurementValue_2 {
    /**
     * Set to True if the counter value is unreliable for some reason
     */
    bool isSuspect;

#if __GNUC__
    __extension__
#endif
    union {
        double floatVal;
        int64_t intVal;
    };
} ComOamSpiPmMeasurementValue_2T;

/**
* The value type of a measurement.
*/
typedef enum ComOamSpiPmMeasurementValueType_2 {
    ComOamSpiPmMeasurementValueType_2_INT = 0,
    ComOamSpiPmMeasurementValueType_2_FLOAT = 1

} ComOamSpiPmMeasurementValueType_2T;


/**
* A container struct for a measurement definition
*/
typedef struct ComOamSpiPmMeasurementDefinition_2 {

    /**
     * The measurement type name as defined in ECIM PM.
     * This shall be a NULL terminated ASCII string.
     */
    const char* name;

    /**
     * An optional description of the measurement type used in the CLI help.
     * This shall be a NULL terminated ASCII string.
     */
    const char* description;

} ComOamSpiPmMeasurementDefinition_2T;


/**
* A container struct for a measurement
*/
typedef struct ComOamSpiPmMeasurement_2 {

    /**
     * The measurement type name as defined in ECIM PM.
     */
    const char* name;

    /**
     * the ID of the PmJob. If no PM job is active, this shall be NULL.
     */
    const char* jobId;


    /**
     * The measurement value(s). This may be NULL if there are no values (nrOfValues == 0).
     */
    ComOamSpiPmMeasurementValue_2T* value;

    /**
     * The type of value, that is int or float.
     */
    ComOamSpiPmMeasurementValueType_2T valueType;

    /**
     * This is the length of the value array.
     */
    uint32_t nrOfValues;


    /**
     * If some value(s) from a measurement was not possible to retrieve, it is possible return an error message which
     * will be shown to the end user. If there is no error information, this shall be NULL.
     */
    const char* errorInformation;

    /**
     * the GP in seconds. If the counter is not from a PM job, this shall be 0
     */
    uint32_t gpInSeconds;


} ComOamSpiPmMeasurement_2T;


/**
* A struct containing all measurements associated to a measured object
*/
typedef struct ComOamSpiPmMeasuredObject_2 {

    /**
     * An array of measurements related to this measured object. This may be NULL if there are no measurements (nrOfMeasurements == 0)
     */
    ComOamSpiPmMeasurement_2T* measurements;

    /**
     * The length of the measurements array
     */
    uint32_t nrOfMeasurements;

    /**
     * This function shall be invoked by the caller of the SPI functions to release allocated data in this container.
     * The container itself shall not be released by this function.
     * The pointer to the function is provided by the implementer of the SPI.
     * The function pointer may be set to NULL within the release function.
     * @param measuredObject The pointer to the container to release.
     */
    void (*release)(struct ComOamSpiPmMeasuredObject_2* measuredObject);

} ComOamSpiPmMeasuredObject_2T;


/**
* A data structure containing all measurement definitions associated to a measured object.
*/
typedef struct ComOamSpiPmMeasuredObjectMeasurementNames_2 {

    /**
     * An array of measurement definitions related to this measured object. This may be NULL if there are none (nrOfValues == 0)
     */
    ComOamSpiPmMeasurementDefinition_2T* definitions;

    /**
     * The length of the definitions array
     */
    uint32_t nrOfValues;

    /**
     * This function shall be invoked by the caller of the SPI functions to release allocated data in this container.
     * The container itself shall not be released by this function.
     * The pointer to the function is provided by the implementer of the SPI.
     * The function pointer may be set to NULL within the release function.
     * @param measurementNames The pointer to the container to release.
     */
    void (*release)(struct ComOamSpiPmMeasuredObjectMeasurementNames_2* measurementNames);

} ComOamSpiPmMeasuredObjectMeasurementNames_2T;


/**
* A struct for a measurement filter
*/
typedef struct ComOamSpiPmMeasurementFilter_2 {

    /**
     * is verbose means that verbose information is requested. This implies that also measurement types that have no values
     * should be returned.
     */
    bool isVerbose;

    /**
     * A NULL terminated string containing the job ID to match.
     * NULL means that all job IDs is a match.
     */
    const char* jobId;

    /**
     * A is NULL terminated pointer array with the names of the measurement types. Each name is a NULL terminated string.
     * Only measurement types in this is a match.
     * NULL means that all measurement types is a match.
     */
    const char** measurementTypes;


} ComOamSpiPmMeasurementFilter_2T;


/**
* A struct containing all measurement job IDs associated to a measured object
*/
typedef struct ComOamSpiPmMeasuredObjectJobIds_2 {
    /**
     * An NULL terminated array of measurement job IDs related to this measured object. Each
     * ID is a NULL terminated ASCII string.
     */
    const char** ids;

    /**
     * This function shall be invoked by the caller of the SPI functions to release allocated data in this container.
     * The container itself shall not be released by this function.
     * The pointer to the function is provided by the implementer of the SPI.
     * The function pointer may be set to NULL within the release function.
     * @param ids The pointer to the container to release.
     */
    void (*release)(struct ComOamSpiPmMeasuredObjectJobIds_2* ids);

}  ComOamSpiPmMeasuredObjectJobIds_2T;


/**
* PM measurement interface. Implemented by the SA to provide real-time access to measurement values.
*/
typedef struct ComOamSpiPmMeasurements_2 {

    /**
     * Common interface description.
    * The "base class" for this interface contains the
     * component name, interface name, and interface version.
     */
    MafMgmtSpiInterface_1T base;

    /**
     * This function returns PM measurement names and values associated to a measured object.
     *
     * @param [in] dn MO distinguished name in 3GPP format. This is the DN of the MO providing the values. Example: "ManagedElement=1,MoWithCounters=2"
     *
     * @param [in] filter
     *          A filter for filtering of which data to be fetched.
     *
     * @param [in|out] measuredObject
     *
     *         The result container contains:
     *             An array of measurements associated to this measured object
     *             The current value of these measurements.
     *
     *         The result container should be allocated by the caller.
     *         The content of the container is allocated by the callee.
     *         The allocated memory for the content is released via the release function in the container.
     *         The container itself shall be released by the caller.
     *         The caller shall invoke the release function if set.
     *         The release function shall be provided by the SPI implementation.
     *         The implementer may set the function pointer to NULL to indicate that there is nothing to release.
     *
     * @return MafOk if the call was successful
     *         MafNotExist if a measured object does not exist
     *         MafFailure if an internal error occurred
     */
    MafReturnT (*getMeasurementValues)(const char * dn, const ComOamSpiPmMeasurementFilter_2T* filter, ComOamSpiPmMeasuredObject_2T * measuredObject);

    /**
     * This function returns all possible PM measurement names associated to a measured object.
     *
     * @param [in] dn MO distinguished name in 3GPP format. Example: "ManagedElement=1,MoWithCounters=2"
     *
     * @param [in|out] measurementNames
     *
     *         The result container contains:
     *             An array of all possible measurement names associated to this measured object
     *
     *         The result container should be allocated by the caller.
     *         The content of the container is allocated by the callee.
     *         The allocated memory for the content is released via the release function in the container.
     *         The container itself shall be released by the caller.
     *         The caller shall invoke the release function if set.
     *         The release function shall be provided by the SPI implementation.
     *         The implementer may set the function pointer to NULL to indicate that there is nothing to release.
     *
     * @return MafOk if the call was successful
     *         MafNotExist if a measured object does not exist
     *         MafFailure if an internal error occurred
     */
    MafReturnT (*getMeasurementNames)(const char * dn, ComOamSpiPmMeasuredObjectMeasurementNames_2T* measurementNames);




    /**
    * This function returns all PM measurement job ids associated to a measured object.
    *
    * @param [in] dn MO distinguished name in 3GPP format. Example: "ManagedElement=1,MoWithCounters=2"
    *
    * @param [in|out] measurementJobIds
    *
    *         The result container contains:
    *             An array of all PM job IDs associated to this measured object
    *
    *         The result container should be allocated by the caller.
    *         The content of the container is allocated by the callee.
    *         The allocated memory for the content is released via the release function in the container.
    *         The container itself shall be released by the caller.
    *         The caller shall invoke the release function if set.
    *         The release function shall be provided by the SPI implementation.
    *         The implementer may set the function pointer to NULL to indicate that there is nothing to release.
    *
    * @return MafOk if the call was successful
    *         MafNotExist if a measured object does not exist
    *         MafFailure if an internal error occurred
    */
    MafReturnT (*getPmJobIds)(const char * dn, ComOamSpiPmMeasuredObjectJobIds_2T* measurementJobIds);




} ComOamSpiPmMeasurements_2T;

#endif // ComOamSpiPmMeasurements_2_h__
