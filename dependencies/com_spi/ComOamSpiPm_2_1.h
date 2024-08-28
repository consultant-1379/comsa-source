#ifndef ComOamSpiPm_2_1_h__
#define ComOamSpiPm_2_1_h__

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <ComMgmtSpiInterface_1.h>
#include <ComOamSpiPm_2.h>

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
const ComMgmtSpiInterface_1T ComOamSpiPmInterface_2_1Id = {NULL, "ComOamSpiPm", "2_1"};
#endif
#ifndef __cplusplus
#define ComOamSpiPmInterface_2_1Id (ComMgmtSpiInterface_1T){NULL, "ComOamSpiPm", "2_1"}
#endif


/**
 * PM Interface. This is used to transfer GP data from the underlying PM Service to the consumer
 * of the PM events. It should typically implemented by the Service Agent.
 */
typedef struct ComOamSpiPm_2_1 {
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
     * @param gpName [in] of the Gp is required inorder to send parallel gp-data request for each Gp
     *
     * @param gpData [out] Pointer to the GP data
     *
     * @return ComOk, or
     * ComFailure if an internal error occurred.
     * ComNotExists if the pmGpId does not exist or it's lifetime has expired.
     */
    ComReturnT (*getGp2)(ComOamSpiPmGpId_2T pmGpId, const char* gpName, ComOamSpiPmGpData_2T ** gpData);

    /**
     * @brief This method should be called to release the allocated memory for the
     *       gpData pointer retrieved with getGp method. Each registered
     *       consumer of the event "ComOamSpiPmEventTypeGpReady_2" have to call releaseGp
     *       for each ComOamSpiPmGpId_2T in those events, regardless if getGp() was called or not.
     *
     * @param pmGpId [in] Identifier of the Gp value that can be deleted
     * @param gpName [in] Multiple Gp can be be released parallely based on Gp name
     *
     * @return ComOk, or
     * ComFailure if an internal error occurred.
     * ComNotExists if the pmGpId does not exist.
     */
    ComReturnT (*releaseGp2)(ComOamSpiPmGpId_2T pmGpId, const char* gpName);

} ComOamSpiPm_2_1T;

#endif // ComOamSpiPm_2_h__
