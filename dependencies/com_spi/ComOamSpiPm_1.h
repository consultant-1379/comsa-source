#ifndef ComOamSpiPm_1_h__
#define ComOamSpiPm_1_h__

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <ComMgmtSpiInterface_1.h>

/**
 * @file ComOamSpiPm_1.h
 * Performance Management SPI.
 * The COM Event Service shall be used by the implementor of this SPI to signal
 * when new measurement results are ready. The event used to signal that is
 * called "ComOamSpiPmEventTypeGpReady_1" and  is defined in this header file. The
 * event value contains an id (ComOamSpiPmGpId) identifying measurement data for
 * one granularity period (GP) in one PM Job (refered to here as GP data).
 * The consumer of the event shall call the method getGp to retrieve GP data. When the
 * consumer has processed that GP data it shall call the SPI method releaseGp to signal
 * that the allocated memory for that GP data can be released.
 * Note: The memory for the GP data is owned by the SA/MW until COM calls getGp. When COM has
 * called getGp, COM takes over the ownage of that memory. The memory can not be deleted before
 * COM calls releaseGp. If COM crashes, that mean an implicit release of the memory.
 */


/**
 * Identity of the Performance Management SPI.
 */
#ifdef __cplusplus
const ComMgmtSpiInterface_1T ComOamSpiPmInterface_1Id = {NULL, "ComOamSpiPm", "1"};
#endif
#ifndef __cplusplus
#define ComOamSpiPmInterface_1Id (ComMgmtSpiInterface_1T){NULL, "ComOamSpiPm", "1"}
#endif

/**
 * Identifier of a GP data entity. Typically data for one GP in a Pm Job.
 */
typedef uint64_t ComOamSpiPmGpId_1T;

/**
 * Event Type Definition. This event is used to signal that new GP data is ready.
 */
#define ComOamSpiPmEventTypeGpReady_1 "ComOamSpiPmEventTypeGpReady_1"

/**
 * Event Value Definition
 */
typedef struct ComOamSpiPmEventValue_1 {

    /**
     * Identifier of the GP data.
     */
    ComOamSpiPmGpId_1T gpId;
    /**
     * The id of the PM Job. This id should correspond to
     * the PmJob MO keyvalue in the ECIM PM fragment.
     */
    char *jobId;
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

} ComOamSpiPmEventValue_1T;


/*
 * ===========================
 * Pm GP data value definition
 * ===========================
 */

/**
 * The value of a counter.
 */
typedef union ComOamSpiPmValue_1 {
    double floatVal;
    int64_t intVal;
} ComOamSpiPmValue_1T;

/**
 * Container struct for an aggregated counter value.
 */
typedef struct ComOamSpiPmAggregatedValue_1 {

    /**
     * Name of the measurement type
     */
    const char* measType;
    /**
     * Counter value
     */
    ComOamSpiPmValue_1T value;
    /**
     * Set to True if the counter value is unreliable for some reason
     */
    bool isSuspect;
    /**
     * Set to True if the counter value is a float value.
     * False if it's an int64_t value.
     */
    bool isFloat;

} ComOamSpiPmAggregatedValue_1T;

/**
 * Container struct for an instance.
 */
typedef struct ComOamSpiPmInstance_1 {

    /**
     * The name of the instance.
     */
    const char* measObjLDN;
    /**
     * An array with all the aggregated values of the instance.
     */
    ComOamSpiPmAggregatedValue_1T* values;
    /**
     * The length of the array of ComOamSpiPmAggregatedValues.
     */
    uint32_t size;

} ComOamSpiPmInstance_1T;

/**
 * Container struct for a group/class of measurement types.
 */
typedef struct ComOamSpiPmMeasObjClass_1 {

    /**
     * The name of the PmGroup.
     */
    const char* measObjClass;
    /**
     * An array with all the instances for this group/class.
     */
    ComOamSpiPmInstance_1T* instances;
    /**
     * The length of the array of ComOamSpiPmInstances.
     */
    uint32_t size;

} ComOamSpiPmMeasObjClass_1T;

/**
 * Container struct for GP data for one Job.
 */
typedef struct ComOamSpiPmGpData_1 {

    /**
     * Array of PmGroups with measurement types.
     */
    ComOamSpiPmMeasObjClass_1T* measObjClasses;
    /**
     * The length of the array of ComOamSpiPmMeasObjClasses.
     */
    uint32_t size;
    /**
     * Set to True if this GP Data is unreliable for some reason.
     */
    bool isSuspect;

} ComOamSpiPmGpData_1T;

/*
 * ===============================
 * End Pm GP data value definition
 * ===============================
 */


/**
 * PM Interface. This is used to transfer GP data from the underlying PM Service to the consumer
 * of the PM events. It should typically implemented by the Service Agent.
 */
typedef struct ComOamSpiPm_1 {
    /**
     * Common interface description.
     * The "base class" for this interface contains the
     * component name, interface name, and interface version.
     */
    ComMgmtSpiInterface_1T base;

    /**
     * @brief This method is used to retrieve GP data (ComOamSpiPmGpData_1T) from
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
    ComReturnT (*getGp)(ComOamSpiPmGpId_1T pmGpId, ComOamSpiPmGpData_1T ** gpData);

    /**
     * @brief This method should be called to release the allocated memory for the
     *       gpData pointer retrieved with getGp method. Each registered
     *       consumer of the event "ComOamSpiPmEventTypeGpReady_1" have to call releaseGp
     *       for each ComOamSpiPmGpId_1T in those events, regardless if getGp() was called or not.
     *
     * @param pmGpId [in] Identifier of the Gp value that can be deleted
     *
     * @return ComOk, or
     * ComFailure if an internal error occurred.
     * ComNotExists if the pmGpId does not exist.
     */
    ComReturnT (*releaseGp)(ComOamSpiPmGpId_1T pmGpId);

} ComOamSpiPm_1T;

#endif // ComOamSpiPm_1_h__ 
