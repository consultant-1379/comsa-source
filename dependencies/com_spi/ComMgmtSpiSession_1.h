#ifndef ComMgmtSpiSession_1_h
#define ComMgmtSpiSession_1_h

#include <stdint.h>
#include <ComMgmtSpiCommon.h>
#include <ComMgmtSpiInterface_1.h>
#include <ComMgmtSpiComponent_1.h>

/**
 * Session management.
 *
 * @file ComMgmtSpiSession_1.h
 *
 * Handles session creation, destruction, session variables and session
 * listeners. The interface can be fetched from ComMgmtSpiInterfacePortal using
 * the identity ComMgmtSpiSessionManagement_1Id definition that is provided by
 * ComMgmtSpiServiceIdentities_1.h.
 *
 */


/**
 * Name of the communication protocol
 * Data contains the values immediately;
 *   0 = unspecified
 *   1 = NETCONF
 *   2 = CLI
 */
#define ComSessionVariableProtocol_1 "ComSessionVariableProtocol_1"

/**
 * Name of logged in user
 * Data format is a pointer to a NULL-terminated string (char *)
 * NULL means "no user logged in"
 */
#define ComSessionVariableUsername_1 "ComSessionVariableUsername_1"


/**
 * Defines the session handle as an unsigned 64-bit integer.
 */
typedef uint64_t ComMgmtSpiSessionHandle_1T;

/**
 * Defines the callback handle as an unsigned 64-bit integer.
 */
typedef uint64_t ComMgmtSpiSessionCallbackHandle_1T;


/**
 * Callback interface for Sessions
 */
typedef struct ComMgmtSpiSessionCallback_1 {
    /**
     * A callback function for session created events. Default variables have
     * been set at this point.
     *
     * @param[in] handle A handle to the session
     */
    void (*sessionCreated)(ComMgmtSpiSessionHandle_1T handle);

    /**
     * A callback function for session destroyed events. Variables still exist
     * at this point.
     *
     * @param[in] handle A handle to the session
     */
    void (*sessionDestroyed)(ComMgmtSpiSessionHandle_1T handle);

} ComMgmtSpiSessionCallback_1T;


typedef struct ComMgmtSpiSessionVariable_1 {
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
    ComReturnT (*destructor)(const char *varname, void *vardata);
} ComMgmtSpiSessionVariable_1T;

/**
 * Session management interface.
 */
typedef struct ComMgmtSpiSession_1 {

    /**
     * Common interface description.
     * The "base class" for this interface contains the component
     * name, interface name, and interface version.
     */
    ComMgmtSpiInterface_1T base;

    /**
     * Creates a new session and binds it to the current thread, then
     * sets all the variables provided in the 'variables' parameter.
     * After the initial variables have been set, all registered
     * 'session created' callbacks will be invoked.
     *
     * If the thread already has a session bound, the operation will fail.
     *
     * The responsibility to close a session (using destroySession) is on the
     * one who created it.
     *
     * If a session already is bound it has to be either closed (using destroySession)
     * or unbound (setActive(0)) before initiating a new one.
     *
     * @param[in] num_vars The numbers of variables in 'variables' to set
     * @param[in] variables An array of variables that should be set during
     * creation of the session.
     * @param[out] handle A handle to the newly created session
     * @return ComOk if the operation succeeded, MafAlreadyExist if a session
     * has already been assigned to the thread.
     */
    ComReturnT (*initSession)(unsigned num_vars,
                              ComMgmtSpiSessionVariable_1T *variables,
                              ComMgmtSpiSessionHandle_1T *handle);

    /**
     * Retrieves the calling thread's active session.
     *
     * @param[out] handle A pointer to a handle that will receive the active
     * handle.
     * @return ComOk if successful, MafNotExist if no session has been bound
     */
    ComReturnT (*getActive)(ComMgmtSpiSessionHandle_1T *handle);

    /**
     * Associates the calling thread with a session, can be used in situations
     * where multiple sessions will share a thread.
     * Will override any previously bound session on this thread.
     *
     * @param[in] handle A handle to the new session. 0 will unbind the session,
     * but it won't be destroyed
     * @return ComOk if successful
     */
    ComReturnT (*setActive)(ComMgmtSpiSessionHandle_1T handle);

    /**
     * Destroys the calling thread's session. The session will be unbound from
     * the current thread and memory for the session will be freed.
     * Variables will get their destructors called.
     * Global 'session destroyed' callbacks will be invoked in sequence, the
     * function will not be able to return until all callbacks have been
     * notified.
     * Only the one who creates a session should destroy it.
     *
     * @return ComOk if all the internal memory was deallocated and all variable
     * destructors returned ComOk, or if no session has been associated to the
     * thread.
     */
    ComReturnT (*destroySession)();

    /**
     * Registers global callbacks to be invoked when a session is created or
     * destroyed. An invocation of sessionDestroyed might not necessarily
     * correlate to an earlier invocation of sessionCreated.
     *
     * @param[in] sessionCallbacks Callbacks that will be invoked when a session is
     *            created or destroyed, can be NULL
     * @param[out] handle A handle to the callbacks that can be used for
     *             unregistering.
     * @param ComOk if it successfully registered the callbacks
     *
     * @n @b Example:
     * @code
     * ...
     * static void sessionCreated(ComMgmtSpiSessionHandle_1T session) {
     *    ...
     * }
     *
     * static void sessionDestroyed(ComMgmtSpiSessionHandle_1T session) {
     *    ...
     * }
     *
     * ComMgmtSpiSessionCallbackHandle_1T callbacks;
     * ComMgmtSpiSessionCallback_1T sessionCallbacks = ...;
     * ComMgmtSpiSession *s_mgmt = ...;
     *
     * // register callbacks
     * ComReturnT ret = s_mgmt->registerCallbacks(sessionCallbacks,
     *                                            &callbacks);
     * if (ret == ComFailure) {
     *    ... handle error ...
     * }
     * ...
     *
     * // unregister callbacks when done
     * s_mgmt->unregisterCallbacks(callbacks);
     * @endcode
     */
    ComReturnT (*registerCallbacks)(
        ComMgmtSpiSessionCallback_1T sessionCallbacks,
        ComMgmtSpiSessionCallbackHandle_1T *handle);

    /**
     * Removes the global callbacks associated with the handle.
     *
     * @param[in] callbacks Handle to callbacks
     * @return ComOk if the callbacks were removed, ComNotExist if the callbacks
     * have already been unregistered or if the handle is invalid
     */
    ComReturnT (*unregisterCallbacks)(
        ComMgmtSpiSessionCallbackHandle_1T callbacks);

    /**
     * Creates or overwrites a variable in a session. If a variable by the same
     * name already exists, it will get its destructor called.
     * Variable names are prefixed by a namespace; "ComSessionVariable", for
     * example.
     * The format of the data is defined by the producer and should be
     * associated to the variable name.
     *
     * @param[in] session The session to set the variable in
     * @param[in] variable A struct containing the variable information
     * @return ComOk if the variable could be set or overwritten by destroying
     *         the previous variable data
     *
     * @n @b Example:
     * @code
     * ...
     * // a function that will be called to remove any resources allocated
     * // for the variable
     * extern "C" ComReturnT usernameCleanup(const char *varname, void *data) {
     *    assert(strcmp(varname, ComSessionVariableUsername) == 0);
     *    free(data);
     *    return ComOk;
     * }
     *
     * ComMgmtSpiSessionHandle_1T session;
     * ComMgmtSpiSession *s_mgmt = ...;
     *
     * // retrieve the current active session
     * if (s_mgmt->setActive(&session) != ComOk) {
     *    ...
     * }
     *
     * // then set the variable
     * ComMgmtSpiSessionVariable_1T variable;
     * variable.name = ComSessionVariableUsername;
     * variable.data = strdup("dracula");
     * variable.destructor = usernameCleanup;
     *
     * if (s_mgmt->setVariable(session, &variable) != ComOk) {
     *    ...
     * }
     * @endcode
     */
    ComReturnT (*setVariable)(ComMgmtSpiSessionHandle_1T session,
                              ComMgmtSpiSessionVariable_1T *variable);

    /**
     * Retrieves a session variable using its identifier.
     *
     * @param[in] session A handle to the session
     * @param[in] varname The variable's identifier
     * @param[out] data A pointer to a void* that will receive the variable's
     *             data
     * @return ComOk if the data could successfully be retrieved, ComNotExist if
     *         the variable doesn't exist
     *
     * @n @b Example:
     * @code
     * ...
     * ComMgmtSpiSessionHandle_1T session;
     * ComMgmtSpiSession *s_mgmt = ...;
     *
     * s_mgmt->getActive(&session);
     *
     * const char *username;
     *
     * if (s_mgmt->getVariable(session,
     *                         ComSessionVariableUsername,
     *                         (void**)&username) != ComOk) {
     *    // Variable not found
     *    username = "unknown user";
     * }
     * @endcode
     */
    ComReturnT (*getVariable)(ComMgmtSpiSessionHandle_1T session,
                              const char *varname,
                              void **data);
} ComMgmtSpiSession_1T;

#endif
