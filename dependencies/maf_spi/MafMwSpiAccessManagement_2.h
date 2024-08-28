#ifndef MafOamSpiAccessManagement_2_h
#define MafOamSpiAccessManagement_2_h

#include <MafMgmtSpiInterface_1.h>

/**
 * @file MafMwSpiAccessManagement_2.h
 *
 * Access Management.
 */


/**
 * Type of the role data.
 */
#ifndef MafMwSpiAccessManagementRoleT_typedef
#define MafMwSpiAccessManagementRoleT_typedef
typedef const char* MafMwSpiAccessManagementRoleT;
#endif

/**
 * An interface for accessing authentication data.
 */
typedef struct MafMwSpiAccessManagement_2 {
    /**
     * Common interface description. The "base class" for this interface
     * contains the component name, interface name, and interface version.
     */
    MafMgmtSpiInterface_1T base;

    /**
     * Returns the roles for the user. Memory of the roles are allocated by
     * the Support Agent and freeRoles must be called when not in use anymore.
     *
     * @param [in] method The transport method selected for the call of this function.
     *
     * @param [in] user The user roles requested for.
     *
     * @param [out] roles The roles for the user. The list of the roles
     * is a null-terminated vector where each role is a MafMwSpiAccessManagementRoleT.
     *
     * @return MafOk, or MafNotExist if the user is not existing, or
     *         MafFailure if an internal error occurred.
     */
    MafReturnT (*getRoles)(const char* method, const char* user, MafMwSpiAccessManagementRoleT** roles);

    /**
     * Frees any allocated memory for the roles.
     *
     * @param [in] method The transport method selected for the call of this function.
     *
     * @param [in] roles The list of the roles.
     *
     * @return MafOk, or MafFailure if an internal error occurred.
     */
    MafReturnT (*freeRoles)(const char* method, MafMwSpiAccessManagementRoleT* roles);
} MafMwSpiAccessManagement_2T;

#endif // MafMwSpiAccessManagement_2_h

