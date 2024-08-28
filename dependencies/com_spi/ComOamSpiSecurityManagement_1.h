#ifndef _ComOamSpiSecurityManagement_1_h_
#define _ComOamSpiSecurityManagement_1_h_

#include <ComMgmtSpiInterface_1.h>

#include <stdbool.h>
#include <stdint.h>

/**
 * @file ComOamSpiSecurityManagement_1.h
 *
 * Security Management. Contains functions and data definitions for
 * accessing and using Security Management.
 */

/**
 * Type to store session specific id.
 */
typedef uint64_t  ComSmSessionHandleT;

/**
 * Specifies the operation type the user is authorized for.
 */
typedef enum ComSmOperationType {
    ComSmOperationTypeGet = 1,
    ComSmOperationTypeSet = 2,
    ComSmOperationTypeCreate = 3,
    ComSmOperationTypeDelete = 4,
    ComSmOperationTypeAction = 5,
    ComSmOperationTypeNavigate = 6
} ComSmOperationTypeT;

/**
 * Security Management.
 */
typedef struct ComOamSpiSecurityManagement_1 {
    /**
     * Common interface description. The "base class" for this interface
     * contains the component name, interface name, and interface version.
     */
    ComMgmtSpiInterface_1T base;

    /**
     * Answers the question: is the user allowed an operation on a specific target.
     *
     * @param [in] handle The identifier of the current session.
     *
     * @param [in] target The target of the operation that the user is
     * authorized for. Target must contain the characters "A-Z", "a-z",
     * "0-9", "_", "." and ",".
     * The target can be expressed in the following formats:
     * @li DN, that is full distinguished name in 3GPP format, beginning with
     * the string "ManagedElement".
     * @n DN.attribute, where attribute is the name of the MO attribute of the
     * MO identified by the DN.
     * @n DN.action, where action is the name of the action of
     * the MO identified by the DN.
     * @n Examples of valid DN targets are "ManagedElement=1" and
     * "ManagedElement=1,Fm=1.userLabel".
     * @li String, where the string can be matched to a rule that is
     * not a rule for a DN. Note that the string must not start
     * with the string "ManagedElement" unless it is a DN.
     *
     * @param [in] type The operation type to authorize the user for.
     *
     * @param [out] answer True if the user is authorized, false if not.
     *
     * @return ComOk if all went well, otherwise one of the COM Error Codes.
     */
    ComReturnT (*isAuthorized)(ComSmSessionHandleT handle, const char* target, ComSmOperationTypeT type, bool* answer);

    /**
     * Creates a new handle specific instance of SmSession,
     * retrieves the roles for the user using Access Management and
     * the rules for the roles using the Managed Object interface.
     * Returns handle as output parameter.
     *
     * @param [in] user The logged-in user.
     *
     * @param [out] handle The identifier of the current session.
     *
     * @return ComOk if all went well, otherwise one of the COM Error Codes.
     */
    ComReturnT (*newSession)(const char* user, ComSmSessionHandleT* handle);

    /**
     * Deletes a handle specific instance of SmSession.
     *
     * @param [in] handle The identifier of the current session.
     *
     * @return ComOk if all went well, otherwise one of the COM Error Codes.
     */
    ComReturnT (*deleteSession)(ComSmSessionHandleT handle);
} ComOamSpiSecurityManagement_1T;

#endif // _ComOamSpiSecurityManagement_1_h_

