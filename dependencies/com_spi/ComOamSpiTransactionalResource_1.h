#ifndef ComOamSpiTransactionalResource_1_h
#define ComOamSpiTransactionalResource_1_h

#include <ComMgmtSpiCommon.h>
#include <ComMgmtSpiInterface_1.h>

/**
 * Transactional resource.
 *
 * @file ComOamSpiTransactionalResource_1.h
 *
 * The ComOamSpiTransactionalResource interface is implemented
 * by components that need to take part (be a participant) in a
 * transaction. The transaction protocol is a typical two phase
 * commit protocol. The implementer must at all times ensure
 * the ACID properties for the data it handles in the transaction,
 * that is:
 *
 * Atomicity:
 * Either all changes or no changes are applied.
 *
 * Consistency:
 * The database contents must at all times be consistent.
 *
 * Isolation:
 * Data in the intermediate states is not accessible by the
 * other transactions.
 *
 * Durability:
 * Once the transaction has succeeded the changes are permanent.
 *
 * The interface represents the transaction control part. The
 * actual modification may be done through some other interface,
 * for example ComOamSpiManagedObject, that the component also
 * provides. Both interfaces are implemented in the context of
 * one component that is responsible for the total functionality
 * of these interfaces together, with the transaction handle as
 * the binding and identifying context.
 *
 * The interface can be fetched from ComMgmtSpiInterfacePortal
 * using the identity provided by the implementer in an include
 * file that contains:
 *
 * @param componentName "componentName"
 * @param interfaceName "ComOamSpiTransactionalResource"
 * @param interfaceVersion "1"
 *
 *
 */

/**
 * The control interface for the transaction participants.
 */
typedef struct ComOamSpiTransactionalResource_1 {
    /**
     * Common interface description.
     * The "base class" for this interface contains the
     * component name, interface name, and interface version.
     */
    ComMgmtSpiInterface_1T base;
    /**
     * The resource joins the transaction by registering itself
     * in the transaction, or returning an error code.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @return ComOk, or one of the other ComReturnT return codes.
     *
     */
    ComReturnT (*join) (ComOamSpiTransactionHandleT txHandle);
    /**
     * The participant must prepare before the participant can commit. If prepare returns
     * ComOk then the participant must be able to execute the commit as one atomic operation.
     *
     *
     * @param[in] txHandle Transaction handle.
     *
     * @return ComOk, or one of the other ComReturnT return codes.
     *
     */
    ComReturnT (*prepare) (ComOamSpiTransactionHandleT txHandle);
    /**
     * The participant shall commit all changes.
     * This call shall succeed if the prepare call returned ComOk,
     * but for example
     * if there is a communication error with the external database,
     * an error code is forwarded to the caller.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*commit) (ComOamSpiTransactionHandleT txHandle);
    /**
     * The participant shall abort the transaction.
     * The transaction is officially ended when this call
     * has completed.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*abort) (ComOamSpiTransactionHandleT txHandle);
    /**
     * Allows the participant to delay cleaning up until the
     * complete transaction is officially done.
     *
     * The implementer of this operation must ensure that all
     * memory that has been allocated during the transaction
     * is properly deallocated.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*finish) (ComOamSpiTransactionHandleT txHandle);
} ComOamSpiTransactionalResource_1T;
#endif
