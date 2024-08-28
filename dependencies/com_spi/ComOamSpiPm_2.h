#ifndef ComOamSpiPm_2_h__
#define ComOamSpiPm_2_h__

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <ComMgmtSpiInterface_1.h>

/**
 * @file ComOamSpiPm_2.h
 * Performance Management SPI.
 * The COM Event Service shall be used by the implementor of this SPI to signal
 * when new measurement results are ready. The event used to signal that is
 * called "ComOamSpiPmEventTypeGpReady_2" and  is defined in this header file. The
 * event value contains an id (ComOamSpiPmGpId) identifying measurement data for
 * one granularity period (GP) in one PM Job (refered to here as GP data).
 * The consumer of the event shall call the method getGp to retrieve GP data. When the
 * consumer has processed that GP data it shall call the SPI method releaseGp to signal
 * that the allocated memory for that GP data can be released.
 * Note: The memory for the GP data is owned by the SA/Middleware until COM calls getGp. When COM has
 * called getGp, COM takes over the ownage of that memory. The memory can not be deleted before
 * COM calls releaseGp. If COM crashes, that mean an implicit release of the memory.
 *
 * The responsibility of PmMeasurementCapabilities attributes is shared between COM and the SA/Middleware
 *
 * Capabilities owned/managed by COM are:
 * maxNoOfPmFiles
 * fileLocation
 * fileGroup
 * fileRPSupported
 * supportedCompressionTypes
 * supportedRopPeriods
 * jobGroupingSupport
 *
 * Capabilities owned/managed by SA/Middleware are:
 * maxNoOfJobs
 * jobStartStopSupport
 * jobPrioritizationSupport
 * alignedReportingPeriod
 * measurementJobSupport
 * realTimeJobSupport
 * thresholdSupport
 * supportedMeasJobGps
 * supportedRtJobGps
 * supportedThreshJobGps
 *
 * To be responsible for an attribute means responsibility to set correct value at start up.
 *
 * COM sets the values of the attributes it is responsible for using the OAM MO SPI, which means
 * the OAM SA must accept write orders although the attributes are declared as read-only in the MOM.
 *
 */


/**
 * Identity of the Performance Management SPI.
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiPmInterface_2Id = {NULL, "ComOamSpiPm", "2"};
#endif
#ifndef __cplusplus
#define ComOamSpiPmInterface_2Id (ComMgmtSpiInterface_1T){NULL, "ComOamSpiPm", "2"}
#endif

/**
 * Identifier of a GP data entity. Typically data for one GP in a Pm Job.
 */
typedef uint64_t ComOamSpiPmGpId_2T;

/**
 * Event Type Definition. This event is used to signal that new GP data is ready.
 */
#define ComOamSpiPmEventTypeGpReady_2 "ComOamSpiPmEventTypeGpReady_2"

/**
 * Event Value Definition
 */
typedef struct ComOamSpiPmEventValue_2 {

    /**
     * Identifier of the GP data.
     */
    ComOamSpiPmGpId_2T gpId;
    /**
     * The id of the PM Job. This id should correspond to
     * the PmJob MO keyvalue in the ECIM PM fragment.
     */
    char* jobId;
    /**
     * Timestamp when the collection of measurement data for
     * this GP was started. Expressed in nanoseconds since Epoch.
     */
    uint64_t gpStartTimestampInNanoSeconds;
    /**
     * Timestamp when the collection of measurement data for
     * this GP was ended. Expressed in nanoseconds since Epoch.
     */
    uint64_t gpEndTimestampInNanoSeconds;

} ComOamSpiPmEventValue_2T;


/*
 * ===========================
 * Pm GP data value definition
 * ===========================
 */

/**
 * The value of a counter.
 */
typedef union ComOamSpiPmValue_2 {
    double floatVal;
    int64_t intVal;
    double* floatArr;
    int64_t* intArr;
} ComOamSpiPmValue_2T;

typedef enum ComOamSpiPmValueType_2 {
    ComOamSpiPmValueType_2_INT = 0,
    ComOamSpiPmValueType_2_FLOAT = 1,
    ComOamSpiPmValueType_2_INTARR = 2,
    ComOamSpiPmValueType_2_FLOATARR = 3,
    ComOamSpiPmValueType_2_NIL = 4
} ComOamSpiPmValueType_2T;

/**
 * Container struct for an aggregated counter value.
 */
typedef struct ComOamSpiPmAggregatedValue_2 {

    /**
     * Name of the measurement type
     */
    const char* measType;

    /**
     * Counter value
     */
    ComOamSpiPmValue_2T value;

    /**
     * The type of value, i.e. int, float, int[] or float[]
     */
    ComOamSpiPmValueType_2T valueType;

    /**
     * The number of values in case of array data. Not relevant for simple int and float.
     */
    uint32_t valueSize;

    /**
     * Set to True if the counter value is unreliable for some reason
     */
    bool isSuspect;


} ComOamSpiPmAggregatedValue_2T;

/**
 * Container struct for an instance.
 */
typedef struct ComOamSpiPmInstance_2 {

    /**
     * The name of the instance.
     */
    const char* measObjLDN;
    /**
     * An array with all the aggregated values of the instance.
     */
    ComOamSpiPmAggregatedValue_2T* values;
    /**
     * The length of the array of ComOamSpiPmAggregatedValues.
     */
    uint32_t size;

} ComOamSpiPmInstance_2T;

/**
 * Container struct for a group/class of measurement types.
 */
typedef struct ComOamSpiPmMeasObjClass_2 {

    /**
     * The name of the PmGroup.
     */
    const char* measObjClass;
    /**
     * An array with all the instances for this group/class.
     */
    ComOamSpiPmInstance_2T* instances;
    /**
     * The length of the array of ComOamSpiPmInstances.
     */
    uint32_t size;

} ComOamSpiPmMeasObjClass_2T;

/**
 * Container struct for GP data for one Job.
 */
typedef struct ComOamSpiPmGpData_2 {

    /**
     * Array of PmGroups with measurement types.
     */
    ComOamSpiPmMeasObjClass_2T* measObjClasses;
    /**
     * The length of the array of ComOamSpiPmMeasObjClasses.
     */
    uint32_t size;
    /**
     * Set to True if this GP Data is unreliable for some reason.
     */
    bool isSuspect;

} ComOamSpiPmGpData_2T;

/*
 * ===============================
 * End Pm GP data value definition
 * ===============================
 */


/**
 * PM Interface. This is used to transfer GP data from the underlying PM Service to the consumer
 * of the PM events. It should typically implemented by the Service Agent.
 */
typedef struct ComOamSpiPm_2 {
    /**
     * Common interface description.
     * The "base class" for this interface contains the
     * component name, interface name, and interface version.
     */
    ComMgmtSpiInterface_1T base;

    /**
     * @brief This method is used to retrieve GP data (ComOamSpiPmGpData_2T) from
     *       the implementor of the SPI
     *       Note: It is not allowed to free the gpData pointer until releaseGp is called.
     *
     * @param pmGpId [in] Identifier of the GP data that is to be retrieved
     *
     * @param gpData [out] Pointer to the GP data
     *
     * @return ComOk, or
     * ComFailure if an internal error occurred.
     * ComNotExists if the pmGpId does not exist or it's lifetime has expired.
     */
    ComReturnT (*getGp)(ComOamSpiPmGpId_2T pmGpId, ComOamSpiPmGpData_2T ** gpData);

    /**
     * @brief This method should be called to release the allocated memory for the
     *       gpData pointer retrieved with getGp method. Each registered
     *       consumer of the event "ComOamSpiPmEventTypeGpReady_2" have to call releaseGp
     *       for each ComOamSpiPmGpId_2T in those events, regardless if getGp() was called or not.
     *
     * @param pmGpId [in] Identifier of the Gp value that can be deleted
     *
     * @return ComOk, or
     * ComFailure if an internal error occurred.
     * ComNotExists if the pmGpId does not exist.
     */
    ComReturnT (*releaseGp)(ComOamSpiPmGpId_2T pmGpId);

} ComOamSpiPm_2T;

#endif // ComOamSpiPm_2_h__
