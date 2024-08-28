#ifndef MafOamSpiManagedObject_3_1_h
#define MafOamSpiManagedObject_3_1_h
#include <MafOamSpiManagedObject_3.h>
#include <MafOamSpiTransaction_1.h>
#include <stdint.h>

#ifndef __cpluplus
#include <stdbool.h>
#endif

/**
 * Managed object interface.
 *
 * @file MafOamSpiManagedObject_3_1.h
 * @ingroup MafOamSpi
 *
 * Contains optional extension functions and data definitions
 * for accessing the MOs.
 *
 * The interface is optional, and can be implemented to improve
 * performance for certain applications.
 *
 * Users of the interface must declare a dependency to this
 * interface in the optional dependency array in the
 * MafMgmtSpiComponent_2 struct for their component.
 *
 * The Identity definitions for the MafOamSpiManagedObject_3_1
 * interface can be found in MafOamSpiServiceIdentities_1.h
 */

typedef struct MafOamSpiManagedObject_3_1 {
    /**
     * Common interface description. The "base class" for this
     * interface contains the component name, interface name, and
     * interface version.
     */
    MafMgmtSpiInterface_1T base;

    /**
    * This is an optional extension to MafOamSpiManagedObject_3
    *
    * Sets values for a list of attributes.
    *
    * @param[in] txHandle Transaction handle.
    *
    * @param[in] dn MO distinguished name in 3GPP format.
    *
    * @param[in] The first attribute in a null-terminated array of
    *       attributes Values to set. Any existing value is
    *       completely replaced by the new value. For example, if
    *       the value is a multi valued attribute then all values
    *       in the existing set is removed and replaced by the new
    *       set of values.
    *
    * @return ComOk, or one of the other ComReturnT return codes.
    */
    MafReturnT (*setMoAttributes)(MafOamSpiTransactionHandleT txHandle,
                                  const char * dn,
                                  MafMoNamedAttributeValueContainer_3T ** attributes);



} MafOamSpiManagedObject_3_1T;

#endif
