#ifndef MafMgmtSpiSession_1_h
#define MafMgmtSpiSession_1_h

#include <stdint.h>
#include <MafMgmtSpiCommon.h>
#include <MafMgmtSpiInterface_1.h>
#include <MafMgmtSpiComponent_1.h>

/**
 * Session management.
 *
 * @file MafMgmtSpiSession_1.h
 * @ingroup MafMgmtSpi
 *
 * Handles session creation, destruction, session variables and session
 * listeners. The interface can be fetched from MafMgmtSpiInterfacePortal using
 * the identity MafMgmtSpiSessionManagement_1Id definition that is provided by
 * MafMgmtSpiServiceIdentities_1.h.
 *
 */


/**
 * Name of the communication protocol
 * Data contains the values immediately;
 *   0 = unspecified
 *   1 = NETCONF
 *   2 = CLI
 */
#define MafSessionVariableProtocol_1 "MafSessionVariableProtocol_1"

/**
 * Name of logged in user
 * Data format is a pointer to a NULL-terminated string (char *)
 * NULL means "no user logged in"
 */
#define MafSessionVariableUsername_1 "MafSessionVariableUsername_1"


/**
 * Defines the session handle as an unsigned 64-bit integer.
 */
typedef uint64_t MafMgmtSpiSessionHandle_1T;

/**
 * Defines the callback handle as an unsigned 64-bit integer.
 */
typedef uint64_t MafMgmtSpiSessionCallbackHandle_1T;


/**
 * Callback interface for Sessions
 */
typedef struct MafMgmtSpiSessionCallback_1 {
    /**
     * A callback function for session created events. Default variables have
     * been set at this point.
     *
     * @param[in] handle A handle to the session
     */
    void (*sessionCreated)(MafMgmtSpiSessionHandle_1T handle);

    /**
     * A callback function for session destroyed events. Variables still exist
     * at this point.
     *
     * @param[in] handle A handle to the session
     */
    void (*sessionDestroyed)(MafMgmtSpiSessionHandle_1T handle);

} MafMgmtSpiSessionCallback_1T;


typedef struct MafMgmtSpiSessionVariable_1 {
    /**
     * Identifying name of this variable.
     */
    const char *name;

    /**
     * The variable's data. The data is owned by the creator of the variable;
     * any resources should be cleaned up in the destructor.
     */
    void *data;

    /**
     * Destructor for session variables; it should free any resources held by the
     * variable.
     *
     * @param[in] varname The identifying string for the variable
     * @param[in] vardata The data contained in the variable
     */
    MafReturnT (*destructor)(const char *varname, void *vardata);
} MafMgmtSpiSessionVariable_1T;

/**
 * Session management interface.
 */
typedef struct MafMgmtSpiSession_1 {

    /**
     * Common interface description.
     * The "base class" for this interface contains the component
     * name, interface name, and interface version.
     */
    MafMgmtSpiInterface_1T base;

    /**
     * Creates a new session and binds it to the current thread, then
     * sets all the variables provided in the 'variables' parameter.
     * After the initial variables have been set, all registered
     * 'session created' callbacks will be invoked.
     *
     * If the thread already has a session bound, the operation will fail.
     *
     * @param[in] num_vars The numbers of variables in 'variables' to set
     * @param[in] variables An array of variables that should be set during
     * creation of the session.
     * @param[out] handle A handle to the newly created session
     * @return MafOk if the operation succeeded, MafAlreadyExist if a session
     * has already been assigned to the thread.
     */
    MafReturnT (*initSession)(unsigned num_vars,
                              MafMgmtSpiSessionVariable_1T *variables,
                              MafMgmtSpiSessionHandle_1T *handle);

    /**
     * Retrieves the calling thread's active session.
     *
     * @param[out] handle A pointer to a handle that will receive the active
     * handle
     * @return MafOk if successful, MafNotExist if no session has been bound
     */
    MafReturnT (*getActive)(MafMgmtSpiSessionHandle_1T *handle);

    /**
     * Associates the calling thread with a session, can be used in situations
     * where multiple sessions will share a thread.
     * Will override any previously bound session on this thread.
     *
     * @param[in] handle A handle to the new session. 0 will unbind the session,
     * but it won't be destroyed
     * @return MafOk if successful
     */
    MafReturnT (*setActive)(MafMgmtSpiSessionHandle_1T handle);

    /**
     * Destroys the calling thread's session. The session will be unbound from
     * the current thread and memory for the session will be freed.
     * Variables will get their destructors called.
     * Global 'session destroyed' callbacks will be invoked in sequence, the
     * function will not be able to return until all callbacks have been
     * notified.
     *
     * @return MafOk if all the internal memory was deallocated and all variable
     * destructors returned MafOk, or if no session has been associated to the
     * thread.
     */
    MafReturnT (*destroySession)();

    /**
     * Registers global callbacks to be invoked when a session is created or
     * destroyed. An invocation of sessionDestroyed might not necessarily
     * correlate to an earlier invocation of sessionCreated.
     *
     * @param[in] sessionCallbacks Callbacks that will be invoked when a session is
     *            created or destroyed, can be NULL
     * @param[out] handle A handle to the callbacks that can be used for
     *             unregistering.
     * @param MafOk if it successfully registered the callbacks
     *
     * @n @b Example:
     * @code
     * ...
     * static void sessionCreated(MafMgmtSpiSessionHandle_1T session) {
     *    ...
     * }
     *
     * static void sessionDestroyed(MafMgmtSpiSessionHandle_1T session) {
     *    ...
     * }
     *
     * MafMgmtSpiSessionCallbackHandle_1T callbacks;
     * MafMgmtSpiSessionCallback_1T sessionCallbacks = ...;
     * MafMgmtSpiSession *s_mgmt = ...;
     *
     * // register callbacks
     * MafReturnT ret = s_mgmt->registerCallbacks(sessionCallbacks,
     *                                            &callbacks);
     * if (ret == MafFailure) {
     *    ... handle error ...
     * }
     * ...
     *
     * // unregister callbacks when done
     * s_mgmt->unregisterCallbacks(callbacks);
     * @endcode
     */
    MafReturnT (*registerCallbacks)(
        MafMgmtSpiSessionCallback_1T sessionCallbacks,
        MafMgmtSpiSessionCallbackHandle_1T *handle);

    /**
     * Removes the global callbacks associated with the handle.
     *
     * @param[in] callbacks Handle to callbacks
     * @return MafOk if the callbacks were removed, MafNotExist if the callbacks
     * have already been unregistered or if the handle is invalid
     */
    MafReturnT (*unregisterCallbacks)(
        MafMgmtSpiSessionCallbackHandle_1T callbacks);

    /**
     * Creates or overwrites a variable in a session. If a variable by the same
     * name already exists, it will get its destructor called.
     * Variable names are prefixed by a namespace; "MafSessionVariable", for
     * example.
     * The format of the data is defined by the producer and should be
     * associated to the variable name.
     *
     * @param[in] session The sesssion to set the variable in
     * @param[in] variable A struct containing the variable information
     * @return MafOk if the variable could be set or overwritten by destroying
     *         the previous variable data
     *
     * @n @b Example:
     * @code
     * ...
     * // a function that will be called to remove any resources allocated
     * // for the variable
     * extern "C" MafReturnT usernameCleanup(const char *varname, void *data) {
     *    assert(strcmp(varname, MafSessionVariableUsername) == 0);
     *    free(data);
     *    return MafOk;
     * }
     *
     * MafMgmtSpiSessionHandle_1T session;
     * MafMgmtSpiSession *s_mgmt = ...;
     *
     * // retrieve the current active session
     * if (s_mgmt->setActive(&session) != MafOk) {
     *    ...
     * }
     *
     * // then set the variable
     * MafMgmtSpiSessionVariable_1T variable;
     * variable.name = MafSessionVariableUsername;
     * variable.data = strdup("dracula");
     * variable.destructor = usernameCleanup;
     *
     * if (s_mgmt->setVariable(session, &variable) != MafOk) {
     *    ...
     * }
     * @endcode
     */
    MafReturnT (*setVariable)(MafMgmtSpiSessionHandle_1T session,
                              MafMgmtSpiSessionVariable_1T *variable);

    /**
     * Retrieves a session variable using its identifier.
     *
     * @param[in] session A handle to the session
     * @param[in] varname The variable's identifier
     * @param[out] data A pointer to a void* that will receive the variable's
     *             data
     * @return MafOk if the data could successfully be retrieved, MafNotExist if
     *         the variable doesn't exist
     *
     * @n @b Example:
     * @code
     * ...
     * MafMgmtSpiSessionHandle_1T session;
     * MafMgmtSpiSession *s_mgmt = ...;
     *
     * s_mgmt->getActive(&session);
     *
     * const char *username;
     *
     * if (s_mgmt->getVariable(session,
     *                         MafSessionVariableUsername,
     *                         (void**)&username) != MafOk) {
     *    // Variable not found
     *    username = "unknown user";
     * }
     * @endcode
     */
    MafReturnT (*getVariable)(MafMgmtSpiSessionHandle_1T session,
                              const char *varname,
                              void **data);
} MafMgmtSpiSession_1T;

#endif
