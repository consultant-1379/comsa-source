/*
 * MafOamSpiRegisterObjectImplementer_3.h
 *
 *  Created on: Sep 1, 2014
 *      Author: xseemah
 */

#ifndef MafOamSpiRegisterObjectImplementer_3_h_
#define MafOamSpiRegisterObjectImplementer_3_h_

#include <MafMgmtSpiInterface_1.h>

/**
 * Oam Register Object implementer interface
 *
 * @file MafOamSpiRegisterObjectImplementer_3.h
 * @ingroup MafOamSpi
 *
 * Contains functions for register and de-register an object implementer
 * to a OAM Support Agent (OamSa).
 */

/**
 * OamSa register object implementer interface.
 * This interface implements the operations needed to register and
 * unregister an object implementer in the OamSa.
 *
 * The OamSa will forward Managed Object operation up-calls from the middle ware
 * to the registered object implementers (OIs).
 *
 * An OI can represent one or more non persistent runtime
 * objects where it supplies the actual attribute values. In this case the OamSa will
 * get an up-call from the Mw and forward that call to the Object implementer. Only
 * read operations can be performed on such an OI.
 *
 * An OI can also act as a validator for modifications performed on persistent
 * objects in the model. In this case the OamSa will forward the calls to the OI
 * before making the actual modifications in the model, allowing the OI to veto the
 * operation(s). The last operation that a validator can veto the actual commit of
 * the transaction. This must be done in the transaction prepare phase of the commit.
 * And it requires that the OamSa uses the explicitPrepare
 * transaction call, otherwise the validator veto will be mapped to the transaction
 * commit which may cause inconsistencies if more than one participant is involved
 * in the transaction
 *
 * All operations to the OI is performed via the MafOamSpiManagedObject interface
 * in a transactional context, handled by the MafOamSpiTransactionalResource interface.
 * Both interfaces are implemented by the OI. The transaction protocol is driven by the
 * MafOamSpiTransactionService via the MafOamSpiTransactionMaster interface.
 *
 * Mo operations performed by the OI to other parts of the model is done in the standard
 * way by performing the operations using the MafOamSpiManagedObject interface provided
 * by the MafOamSpiCmRouterService, routing the call to the appropriate destination,
 * for example the OamSa.
 *
 * Implementer of this Interface shall fetch the interface arrays of ManagedObject and
 * TransactionalResource of the Object Implementer using getComponentInterfaceArray
 * from MafMgmtSpiInterfacePortal_3_1 interface and then use the intended version which
 * the implementer supports.
 *
 * OI shall implement or provide the support from ManagedObject version 3 and
 * TransactionResource version 2. And This is the baseline for all OIs.
 *
 * @n @b Example:
 * @code
 * ...
 * MafMgmtSpiInterface_1T ***result;
 * portal->getComponentInterfaceArray(componentName, // OI componentName
 *                                    interface,     // MO or TxR
 *                                    result);
 *
 * if(*result)
 * {
 *     // SA found that MocPath/Dn is not valid
 *     // SA shall return MafNotExist to OI
 *     if(mocPath_Dn_invalid())
 *     {
 *         return MafNotExist;
 *     }
 *
 *     // SA has found the required version interfaces and
 *     // should use them and return MafOk to OI
 *     ...
 *     return MafOk;
 * }
 * else
 * {
 *     // SA has NOT found the required version interfaces and
 *     // SA shall return MafFailure to OI
 *	...
 *      return MafFailure;
 * }
 * ...
 * @endcode
 *
 */

typedef struct MafOamSpiRegisterObjectImplementer_3 {

    MafMgmtSpiInterface_1T base;

    /**
     * Register as object implementer for a specific MO class.
     *
     * @param componentName the Component Name of the Object Implementer
     * that the SA shall use to fetch the ManagedObject and
     * TransactionalResource interfaces from the Portal.
     * @param mocPath the complete path from the root and down to
     * the MOC the OI will represent. Example: /Me/ApplX/SomeMoc
     * The path is needed for the OI to resolve ambiguous MOC names.
     * @return MafOk if the  the operation succeeded and the OamSa is
     * responsible for this mocPath, MafNotExist if the OamSa doesn't
     * handle this mocPath, MafAlreadyExist if the object is already
     * registered, otherwise one of the other
     * MafReturnT error codes
     */
    MafReturnT (*registerClass)(const char * componentName,
                                const char * mocPath);

    /**
     * Register for a particular Mo instance
     * that the Sa will forward calls to from the middleware.
     *
     * @param componentName the Component Name of the Object Implementer
     * that the SA shall use to fetch the ManagedObject and
     * TransactionalResource interfaces from the Portal.
     * @param dn the dn in 3GPP format
     * @return MafOk if the operation succeeded and the OamSa is
     * responsible for this dn, MafNotExist if the OamSa doesn't
     * handle this dn, otherwise one of the other
     * MafReturnT error codes
     */
    MafReturnT (*registerDn)(const char * componentName,
                             const char * dn);


    /**
     * @param componentName the Component Name of the Object Implementer
     * @param mocPath the complete path from the root and down to
     * the MOC the OI will represent. Example: /Me/ApplX/SomeMoc
     * will be handled by this OI
     * @return MafOk if the operation succeeded, MafNotExist if the class is not
     * registered, otherwise one of the other
     * MafReturnT error codes
     */
    MafReturnT (*unregisterClass)(const char * componentName,
                                  const char * mocPath);

    /**
     * @param componentName the Component Name of the Object Implementer
     * @param dn the dn
     * @return MafOk if the operation succeeded, otherwise one of the other
     * MafReturnT error codes
     */
    MafReturnT (*unregisterDn)(const char * componentName,
                               const char * dn);

} MafOamSpiRegisterObjectImplementer_3T;

#endif