
#ifndef MafOamSpiRegisterObjectImplementer_1_h_
#define MafOamSpiRegisterObjectImplementer_1_h_

#include <MafMgmtSpiInterface_1.h>

/**
 * Oam Register Object implementer interface
 *
 * @file MafOamSpiRegisterObjectImplementer_1.h
 * @ingroup MafOamSpi
 *
 * Contains functions for register and deregister an object implementer
 * to a OAM Support Agent (OamSa).
 *
 */

/**
 * Defines filter options when registering the object implementer (OI).
 */
typedef enum MafOamSpiRegisterObjectImplementerFilter {
    /**
     * Only the indicated instance. This option is typically used
     * When the OI is an implementer for a runtime object.
     */
    MAF_OAM_REGISTER_OI_INSTANCE = 0,
    /**
     * The indicated instance and the complete subtree below. This option
     * is typically used when the OI is a validator for operations
     * performed on persistent objects.
     */
    MAF_OAM_REGISTER_OI_SUBTREE = 1
} MafOamSpiRegisterObjectImplementerFilterT;

/**
 * OamSa register object implementer interface.
 * This interface implements the operations needed to register and
 * unregister an object implementer in the OamSa.
 *
 * The OamSa will forward Managed Object operation upcalls from the middleware
 * to the registered object implementers (OIs).
 *
 * An OI can represent one or more non persistent runtime
 * objects where it supplies the actual attribute values. In this case the OamSa will
 * get an upcall from the Mw and forward that call to the Object implementer. Only
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
 * by the MafOamSpiCmRouterService, routing the call to the apropriate destination,
 * for example the OamSa.
 */

typedef struct MafOamSpiRegisterObjectImplementer_1 {

    MafMgmtSpiInterface_1T base;

    /**
     * Register as object implementer for a specific MO class.
     *
     * @param managedObjectInterface the ManagedObject interface
     * identity that the SA shall use when forwarding.
     * @param transactionalResource the TransactionalResource interface
     * identity that the SA shall use when forwarding.
     * @param mocPath the complete path from the root and down to
     * the MOC the OI will represent. Example: /Me/ApplX/SomeMoc
     * The path is needed for the OI to resolve ambiguous MOC names.
     * @return MafOk if the  the operation succeeded and the OamSa is
     * responsible for this mocPath, MafNotExist if the OamSa doesn't
     * handle this mocPath, MafAlreadyExist if the object is already
     * registered, otherwise one of the other
     * MafReturnT error codes
     */
    MafReturnT (*registerClass)(MafMgmtSpiInterface_1T managedObjectInterfaceId,
                                MafMgmtSpiInterface_1T transactionalResourceId,
                                const char * mocPath);

    /**
     * Register for a particular Mo instance or subtree of MO instances
     * that the Sa will forward calls to from the middleware.
     *
     * @param managedObjectInterface the ManagedObject interface id
     * @param transactionalResource the TransactionalResource interface id.
     * @param dn the dn in 3GPP format
     * @param filter the filter
     * @return MafOk if the operation succeeded and the OamSa is
     * responsible for this dn, MafNotExist if the OamSa doesn't
     * handle this dn, otherwise one of the other
     * MafReturnT error codes
     */
    MafReturnT (*registerDn)(MafMgmtSpiInterface_1T managedObjectInterfaceId,
                             MafMgmtSpiInterface_1T transactionalResourceId,
                             const char * dn,
                             MafOamSpiRegisterObjectImplementerFilterT filter);


    /**
     * @param managedObjectInterface the ManagedObject interface id
     * @param transactionalResource the TransactionalResource interface id.
     * @param mocPath the complete path from the root and down to
     * the MOC the OI will represent. Example: /Me/ApplX/SomeMoc
     * will be handled by this OI
     * @return MafOk if the operation succeeded, MafNotExist if the class is not
     * registered, otherwise one of the other
     * MafReturnT error codes
     */
    MafReturnT (*unregisterClass)(MafMgmtSpiInterface_1T managedObjectInterfaceId,
                                  MafMgmtSpiInterface_1T transactionalResourceId,
                                  const char * mocPath);

    /**
     * @param managedObjectInterface the ManagedObject interface id
     * @param transactionalResource the TransactionalResource interface id.
     * @param dn the dn
     * @return MafOk if the operation succeeded, otherwise one of the other
     * MafReturnT error codes
     */
    MafReturnT (*unregisterDn)(MafMgmtSpiInterface_1T managedObjectInterfaceId,
                               MafMgmtSpiInterface_1T transactionalResourceId,
                               const char * dn);
} MafOamSpiRegisterObjectImplementer_1T;

#endif

