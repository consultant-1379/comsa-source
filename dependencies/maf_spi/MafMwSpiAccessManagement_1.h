#ifndef MafOamSpiAccessManagement_1_h
#define MafOamSpiAccessManagement_1_h

#include <MafMgmtSpiInterface_1.h>

/**
 * @file MafMwSpiAccessManagement_1.h
 * @ingroup MafMwSpi
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
typedef struct MafMwSpiAccessManagement_1 {
    /**
     * Common interface description. The "base class" for this interface
     * contains the component name, interface name, and interface version.
     */
    MafMgmtSpiInterface_1T base;

    /**
     * Returns the roles for the user. Memory of the roles are allocated by
     * the Support Agent and freeRoles must be called when not in use anymore.
     *
     * @param [in] user The user roles requested for.
     *
     * @param [out] roles The roles for the user. The list of the roles
     * is a null-terminated vector where each role is a MafMwSpiAccessManagementRoleT.
     *
     * @return MafOk, or MafNotExist if the user is not existing, or
     *         MafFailure if an internal error occurred.
     */
    MafReturnT (*getRoles)(const char* user, MafMwSpiAccessManagementRoleT** roles);

    /**
     * Frees any allocated memory for the roles.
     *
     * @param [in] roles The list of the roles.
     *
     * @return MafOk, or MafFailure if an internal error occurred.
     */
    MafReturnT (*freeRoles)(MafMwSpiAccessManagementRoleT* roles);
} MafMwSpiAccessManagement_1T;

#endif // MafMwSpiAccessManagement_1_h

