#ifndef MafMwSpiAvailabilityController_1_h_
#define MafMwSpiAvailabilityController_1_h_

#include <MafMgmtSpiInterface_1.h>
#include <MafMgmtSpiCommon.h>

typedef void* MafMwSpiAvailabilityInvocationT;

/**
 * Availability controller interface.
 *
 * @file MafMwSpiAvailabilityController_1.h
 * @ingroup MafMwSpi
 */

/**
 * The High Availability modes that MAF can operate in.
 */
typedef enum MafMwSpiHaMode {
    /**
     * <p>MAF has started its processes and made other initializations
     * but none of its services is offered.
     * MAF ends up in this mode after being started, but the
     * Middleware (MW) Support Agent (SA) can also order MAF
     * to return to this mode after being in Active or Standby mode.</p>
     */
    MafMwSpiHaModeUnassigned = 1,
    /**
     * <p>MAF provides all the services.</p>
     */
    MafMwSpiHaModeActive = 2,
    /**
     * <p>MAF acts as standby and keeps the same state while it is
     * operating in Active mode.</p>
     */
    MafMwSpiHaModeStandby = 3
} MafMwSpiHaModeT;


/**
 * The reasons for changes of the High Availability operational mode.
 */
typedef enum MafMwSpiHaReason {
    /**
     * <p>This reason is given when MW SA sets the HA mode the first
     * time to a MAF component.</p>
     */
    MafMwSpiHaReasonInit = 1,
    /**
     * <p>This reason is given when MW SA sets the HA mode Active to
     * a MAF component that before had the HA mode Standby.
     * The HA mode change was triggered by a fault.</p>
     */
    MafMwSpiHaReasonFail = 2,
    /**
     * <p>This reason is given when MW SA sets a new HA mode due to
     * some other reason, for example a management operation.</p>
     */
    MafMwSpiHaReasonSwitchover = 3
} MafMwSpiHaReasonT;


/**
 * The recovery options that MAF can recommend.
 */
typedef enum MafMwSpiRecommendedRecovery {
    /**
     * <p>Restart the current MAF component without changing the HA mode.</p>
     */
    MafMwSpiRecommendedRecoveryRestart = 1,
    /**
     * <p>Restart the current MAF component and activate the standby MAF.</p>
     */
    MafMwSpiRecommendedRecoveryFailover = 2
} MafMwSpiRecommendedRecoveryT;


/**
 * Callback functions.
 *
 * These functions are implemented by MAF and invoked from the
 * Support Agent providing Availability Controller service.
 *
 * This struct should be retained until the component which is implementing
 * the MafMwSpiAvailabilityController_1 interface (for example,SA providing Availability Controller service )
 * is stopped and mafLCMterminate is called on it.
 *
 * Also, it is not allowed to call MafMwApiAc SPI when executing in a callback in MafMwSpiAvailibilityController SPI.
 * That means the implementer of afMwSpiAvailabilityController_1 interface (For example,SA providing Availability
 * Controller service ) should not invoke any callback function from its response callback functions again.
 *
  */
typedef struct MafMwSpiAc {
    /**
     * MAF sets its HA mode.
     * MAF responses by invoking haModeAssumed.
     *
     * @param[in] invocation Identifier for this invocation that shall
     * be returned by MAF in the response call.
     *
     * @param[in] haMode HA mode that MAF shall operate in.
     *
     * @param[in] haReason Reason for the change of the HA mode.
     */
    void (*setHaMode)(MafMwSpiAvailabilityInvocationT invocation, MafMwSpiHaModeT haMode, MafMwSpiHaReasonT haReason);
    /**
     * MAF issues a health report.
     * MAF responses by invoking healthCheckReport.
     *
     * @param[in] invocation Identifier for this invocation that shall
     * be returned by MAF in the response call.
     */
    void (*healthCheck)(MafMwSpiAvailabilityInvocationT invocation);
    /**
     * MAF releases its resources and executes all other preparations
     * for the termination except exiting the MAF process.
     * MAF responses by invoking prepareTerminationResponse.
     *
     * @param[in] invocation Identifier for this invocation that shall
     * be returned by MAF in the response call.
     */
    void (*prepareTermination)(MafMwSpiAvailabilityInvocationT invocation);
    /**
     * MAF exits the MAF process.
     */
    void (*terminateProcess)( void );
} MafMwSpiAcT;


/**
 * Availability Controller interface.
 * MAF invokes these functions, which are implemented by the Support Agent providing the Availability Controller service.
 *
 * It is not allowed to call MafMwApiAc SPI when executing in a callback in MafMwSpiAvailibilityController SPI.
 * That means the Support agent should not invoke any callback function from its response callback functions again.
 */
typedef struct MafMwSpiAvailabilityController_1 {
    /**
     * common interface description.
     * The "base class" for this interface. It contains the component
     * name, the interface name, and the interface version.
     */
    MafMgmtSpiInterface_1T base;
    /**
     * Invoked by MAF during the startup of the Availability Controller service.
     *
     * @param[in] acCallbacks Pointer to the MAF callback functions.
     *
     * @return  MafOk, or MafInvalidArgument if the parameter is null.
     */
    MafReturnT (*acInitialize) (const MafMwSpiAcT* acCallbacks);
    /**
     * Invoked by MAF with the purpose to declare the health of MAF.
     * Invoked as a response to the healthCheck callback function,
     * but can also be invoked by MAF for other reasons, for example
     * when MAF detects an error that it can not recover by itself.
     *
     * @param[in] invocation Invocation identifier. It must have the
     * same value as in the corresponding healthCheck call when
     * responding.
     *
     * @param[in] healthReport Indicates the health of MAF.
     *
     * @param[in] recommendedRecovery The way of recovery that MAF
     * recommends.
     *
     * @return  MafOk, or MafInvalidArgument if a parameter is null or
     * if the invocation has an invalid value.
     */
    MafReturnT (*healthCheckReport) (MafMwSpiAvailabilityInvocationT invocation, MafReturnT healthReport, MafMwSpiRecommendedRecoveryT recommendedRecovery);
    /**
     * Invoked when MAF responses to the setHaMode callback function.
     *
     * @param[in] invocation Invocation identifier. It must have the
     * same value as in the corresponding call to setHaMode.
     *
     * @param[in] error Indicates the result from the corresponding call
     * to setHaMode.
     *
     * @return  MafOK, or MafInvalidArgument if a parameter is null or
     * if the invocation has an invalid value.
     */
    MafReturnT (*haModeAssumed) (MafMwSpiAvailabilityInvocationT invocation, MafReturnT error);
    /**
     * Invoked when MAF responses to the prepareTermination callback
     * function. The Availability Controller service can, depending on the value of
     * the error parameter, decide if it shall continue by invoking
     * terminateProcess or if it shall retry to invoke
     * prepareTermination again.
     *
     * @param[in] invocation Invocation identifier. It must have the
     * same value as in the corresponding call to prepareTermination.
     *
     * @param[in] error Indicates the result from the corresponding call
     * to prepareTermination.
     *
     * @return  MafOK, or MafInvalidArgument if a parameter is null or
     * if the invocation has an invalid value.
     */
    MafReturnT (*prepareTerminationResponse) (MafMwSpiAvailabilityInvocationT invocation, MafReturnT error);
} MafMwSpiAvailabilityController_1T;

#endif
