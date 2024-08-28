#ifndef ComMwSpiAvailabilityController_1_h_
#define ComMwSpiAvailabilityController_1_h_

#include <ComMgmtSpiInterface_1.h>
#include <ComMgmtSpiCommon.h>

typedef void* ComMwSpiAvailabilityInvocationT;

/**
 * Availability controller interface.
 *
 * @file ComMwSpiAvailabilityController_1.h
 */

/**
 * The High Availability modes that COM can operate in.
 */
typedef enum ComMwSpiHaMode {
    /**
     * <p>COM has started its processes and made other initializations
     * but none of its services is offered.
     * COM ends up in this mode after being started, but the
     * Middleware (MW) Support Agent (SA) can also order COM
     * to return to this mode after being in Active or Standby mode.</p>
     */
    ComMwSpiHaModeUnassigned = 1,
    /**
     * <p>COM provides all the services.</p>
     */
    ComMwSpiHaModeActive = 2,
    /**
     * <p>COM acts as standby and keeps the same state while it is
     * operating in Active mode.</p>
     */
    ComMwSpiHaModeStandby = 3
} ComMwSpiHaModeT;


/**
 * The reasons for changes of the High Availability operational mode.
 */
typedef enum ComMwSpiHaReason {
    /**
     * <p>This reason is given when MW SA sets the HA mode the first
     * time to a COM component.</p>
     */
    ComMwSpiHaReasonInit = 1,
    /**
     * <p>This reason is given when MW SA sets the HA mode Active to
     * a COM component that before had the HA mode Standby.
     * The HA mode change was triggered by a fault.</p>
     */
    ComMwSpiHaReasonFail = 2,
    /**
     * <p>This reason is given when MW SA sets a new HA mode due to
     * some other reason, for example a management operation.</p>
     */
    ComMwSpiHaReasonSwitchover = 3
} ComMwSpiHaReasonT;


/**
 * The recovery options that COM can recommend.
 */
typedef enum ComMwSpiRecommendedRecovery {
    /**
     * <p>Restart the current COM component without changing the HA mode.</p>
     */
    ComMwSpiRecommendedRecoveryRestart = 1,
    /**
     * <p>Restart the current COM component and activate the standby COM.</p>
     */
    ComMwSpiRecommendedRecoveryFailover = 2
} ComMwSpiRecommendedRecoveryT;


/**
 * Callback functions.
 * These functions are implemented by COM and invoked from the
 * Support Agent providing Availability Controller service.
 */
typedef struct ComMwSpiAc {
    /**
     * COM sets its HA mode.
     * COM responses by invoking haModeAssumed.
     *
     * @param[in] invocation Identifier for this invocation that shall
     * be returned by COM in the response call.
     *
     * @param[in] haMode HA mode that COM shall operate in.
     *
     * @param[in] haReason Reason for the change of the HA mode.
     */
    void (*setHaMode)(ComMwSpiAvailabilityInvocationT invocation, ComMwSpiHaModeT haMode, ComMwSpiHaReasonT haReason);
    /**
     * COM issues a health report.
     * COM responses by invoking healthCheckReport.
     *
     * @param[in] invocation Identifier for this invocation that shall
     * be returned by COM in the response call.
     */
    void (*healthCheck)(ComMwSpiAvailabilityInvocationT invocation);
    /**
     * COM releases its resources and executes all other preparations
     * for the termination except exiting the COM process.
     * COM responses by invoking prepareTerminationResponse.
     *
     * @param[in] invocation Identifier for this invocation that shall
     * be returned by COM in the response call.
     */
    void (*prepareTermination)(ComMwSpiAvailabilityInvocationT invocation);
    /**
     * COM exits the COM process.
     */
    void (*terminateProcess)( void );
} ComMwSpiAcT;


/**
 * Availability Controller interface.
 * COM invokes these functions, which are implemented by the Support Agent providing the Availability Controller service.
 */
typedef struct ComMwSpiAvailabilityController_1 {
    /**
     * Common interface description.
     * The "base class" for this interface. It contains the component
     * name, the interface name, and the interface version.
     */
    ComMgmtSpiInterface_1T base;
    /**
     * Invoked by COM during the startup of the Availability Controller service.
     *
     * @param[in] acCallbacks Pointer to the COM callback functions.
     *
     * @return  ComOk, or ComInvalidArgument if the parameter is null.
     */
    ComReturnT (*acInitialize) (const ComMwSpiAcT* acCallbacks);
    /**
     * Invoked by COM with the purpose to declare the health of COM.
     * Invoked as a response to the healthCheck callback function,
     * but can also be invoked by COM for other reasons, for example
     * when COM detects an error that it can not recover by itself.
     *
     * @param[in] invocation Invocation identifier. It must have the
     * same value as in the corresponding healthCheck call when
     * responding.
     *
     * @param[in] healthReport Indicates the health of COM.
     *
     * @param[in] recommendedRecovery The way of recovery that COM
     * recommends.
     *
     * @return  ComOk, or ComInvalidArgument if a parameter is null or
     * if the invocation has an invalid value.
     */
    ComReturnT (*healthCheckReport) (ComMwSpiAvailabilityInvocationT invocation, ComReturnT healthReport, ComMwSpiRecommendedRecoveryT recommendedRecovery);
    /**
     * Invoked when COM responses to the setHaMode callback function.
     *
     * @param[in] invocation Invocation identifier. It must have the
     * same value as in the corresponding call to setHaMode.
     *
     * @param[in] error Indicates the result from the corresponding call
     * to setHaMode.
     *
     * @return  ComOK, or ComInvalidArgument if a parameter is null or
     * if the invocation has an invalid value.
     */
    ComReturnT (*haModeAssumed) (ComMwSpiAvailabilityInvocationT invocation, ComReturnT error);
    /**
     * Invoked when COM responses to the prepareTermination callback
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
     * @return  ComOK, or ComInvalidArgument if a parameter is null or
     * if the invocation has an invalid value.
     */
    ComReturnT (*prepareTerminationResponse) (ComMwSpiAvailabilityInvocationT invocation, ComReturnT error);
} ComMwSpiAvailabilityController_1T;

#endif
