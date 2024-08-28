#ifndef ComOamSpiAccessManagement_1_h
#define ComOamSpiAccessManagement_1_h

#include <ComMgmtSpiInterface_1.h>

/**
 * @file ComMwSpiAccessManagement_1.h
 *
 * Access Management.
 */

/**
 * Type of the role data.
 */
#ifndef ComMwSpiAccessManagementRoleT_typedef
#define ComMwSpiAccessManagementRoleT_typedef
typedef const char* ComMwSpiAccessManagementRoleT;
#endif

/**
 * An interface for accessing authentication data.
 */
typedef struct ComMwSpiAccessManagement_1 {
    /**
     * Common interface description. The "base class" for this interface
     * contains the component name, interface name, and interface version.
     */
    ComMgmtSpiInterface_1T base;

    /**
     * Returns the roles for the user. Memory of the roles are allocated by
     * the Support Agent and freeRoles must be called when not in use anymore.
     *
     * @param [in] user The user roles requested for.
     *
     * @param [out] roles The roles for the user. The list of the roles
     * is a null-terminated vector where each role is a ComMwSpiAccessManagementRoleT.
     *
     * @return ComOk, or ComNotExist if the user is not existing, or
     *         ComFailure if an internal error occurred.
     */
    ComReturnT (*getRoles)(const char* user, ComMwSpiAccessManagementRoleT** roles);

    /**
     * Frees any allocated memory for the roles.
     *
     * @param [in] roles The list of the roles.
     *
     * @return ComOk, or ComFailure if an internal error occurred.
     */
    ComReturnT (*freeRoles)(ComMwSpiAccessManagementRoleT* roles);
} ComMwSpiAccessManagement_1T;

#endif // ComMwSpiAccessManagement_1_h

