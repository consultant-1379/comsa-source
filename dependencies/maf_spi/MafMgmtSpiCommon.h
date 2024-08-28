#ifndef MafMgmtSpiCommon_h
#define MafMgmtSpiCommon_h
/**
 * Common return codes.
 *
 * @file MafMgmtSpiCommon.h
 * @ingroup MafMgmtSpi
 *
 * Contains the common return codes.
 */

/**
 * The function return codes must be used by all the interfaces.
 * The purpose of these codes is to allow the caller of a function to
 * come to a programmatic decision based on the return code.
 * It is recommended to return human-readable error information using the
 * MafMgmtSpiThreadContext if a return code is to be interpreted as an error.
 *
 * Return value = 0 is always OK. @n
 * Return value < 0 is normally an error. @n
 * Return value > 0 is normally not an error but
 * informs the caller that the service could not be provided.
 */
typedef enum {
    /**
     * The function call executed successfully.
     */
    MafOk = 0,
    /**
     * <p>The function could not provide any service at this point in time.
     * The problem that occured is temporary, and the caller may retry later.
     *
     * <p>Note that this error code is only to be returned from interfaces
     * that explicitely declares that this error code can be returned
     * and have this meaning. Currently all use cases of MAF SPIs interprets
     * anything other than MafOk
     * a failure and will abort the operation.</p>
     */
    MafTryAgain = 1,
    /**
     * <p>The function could not provide any service since the service is not started.</p>
     */
    MafNotActive = 2,
    /**
     * <p>The function call failed, an error has occured which is specific
     * for the function implementation.</p>
     */
    MafFailure = -1,
    /**
     * <p>The function call failed since something sought after did not exist.
     * Detailed information can be obtained from MafMgmtSpiThreadContext.</p>
     */
    MafNotExist = -2,
    /**
     * <p>The function call failed since something that was to be created already exists.
     * Detailed information can be obtained from MafMgmtSpiThreadContext.</p>
     */
    MafAlreadyExist = -3,
    /**
     * <p>The function call failed and was aborted. The function did not change
     * any persistent data.
     * Detailed information can be obtained from MafMgmtSpiThreadContext.</p>
     */
    MafAborted = -4,
    /**
     * <p>The function call failed since an object is locked.</p>
     */
    MafObjectLocked = -5,
    /**
     * <p>The function call failed in the prepare phase of the transaction.
     * The transaction has been aborted.</p>
     */
    MafPrepareFailed = -6,
    /**
     * <p>The function call failed in the commit phase.
     * Some participants may have failed to commit and the
     * total transactional result may be inconsistent. A human
     * may be needed to resolve the situation.</p>
     */
    MafCommitFailed = -7,
    /**
     * <p>The function call failed since an argument is invalid.</p>
     */
    MafInvalidArgument = -8,
    /**
     * <p>The function call failed since the data did not validate.
     * The only time this error code should be used is as return
     * code from the commit operation</p>
     */
    MafValidationFailed = -9,
    /**
     * <p>The function call failed since there was no available resource, such as memory.</p>
     */
    MafNoResources = -10,
    /**
     * <p>Some vital resource needed in the function call has timed out.
     * It is unspecified whether the call succeeded or whether it did not,
     * the user of interface must find it out.</p>
     */
    MafTimeOut = -11
} MafReturnT;

/**
 * The type is of no concern to anyone except the MAF.
 */
typedef unsigned long MafOamSpiTransactionHandleT;


/**
 * The struct defines a filter parameter.
 */
typedef struct MafNameValuePair {
    /**
     * Name of the filter parameter.
     */
    const char* name;
    /**
     * Value of the filter parameter.
     */
    const char* value;
} MafNameValuePairT;

#endif
