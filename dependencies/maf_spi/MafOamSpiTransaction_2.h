#ifndef MafOamSpiTransaction_2_h
#define MafOamSpiTransaction_2_h
#include <MafOamSpiTransaction_1.h>
#include <MafOamSpiTransactionalResource_2.h>
#include <MafMgmtSpiInterface_1.h>

/**
 * Transaction interface.
 *
 * @file MafOamSpiTransaction_2.h
 * @ingroup MafOamSpi
 *
 * The MafOamSpiTransaction interface provides access to the
 * transaction functionality required by the participants in a
 * transaction. A participant implements
 * the MafOamSpiTransactionalResource interface.
 *
 * The MafOamSpiTransaction interface can be fetched from MafMgmtSpiInterfacePortal
 * using the identity MafOamSpiTransaction_1Id definition that
 * is provided by MafOamSpiServiceIdentities_1.h.
 *
 * @b Parameters <br>
 * All functions takes as one of the parameters a transaction
 * handle that represents the context in which the operation
 * takes place.
 *
 * @b Return <br>
 * A return value other than MafOk is considered a failure.
 * At failure, a descriptive text is set in the thread context,
 * see MafMgmtSpiThreadContext.h.
 */

/**
 * Locking policy that the transaction must use.
 *
 * @b MafLockWrite <br>
 * No lock is acquired on MOs for read operations and the last committed data
 * is returned. This is also true if the lock is already held by an ongoing
 * Read/Write transaction.
 *
 * @b MafLockReadWrite <br>
 * The lock is acquired on all MOs that are modified and the lock is acquired
 * on all MOs that are accessed.
 */


/**
 * Transaction interface.
 */
typedef struct MafOamSpiTransaction_2 {
    /**
     * Common interface description.
     * The "base class" for this interface contains the component
     * name, interface name, and interface version.
     */
    MafMgmtSpiInterface_1T base;
    /**
     * Register a participant in the transaction.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] resource Pointer to the interface instance
     * that will participate.
     *
     * @return MafOk, or @n
     * MafNotExist if the transaction does not exist, or @n
     * MafInvalidArgument if resource is null, or @n
     * one of the other MafReturnT return codes.
     *
     * <br>The function is typically called in the
     * TransactionalResource::join function.
     *
     * @b Example:
     * @code
     * TransactionalResource_1T * myTxResource; // instance set at component initiation
     * TransactionInterface_1T * txIf; // instance probably set at component initiation
     * MafReturnT myJoinImpl(MafOamSpiTransactionHandleT txHandle){
     *     ...
     *     if (!txIf->registerParticipant(txHandle, myTxResource))
     *         { ...error handling... }
     *     return MafOk;
     * }
     * @endcode
     */
    MafReturnT (*registerParticipant)(MafOamSpiTransactionHandleT txHandle,
                                      MafOamSpiTransactionalResource_2T * resource);
    /**
     * Set a pointer to something in the local thread and associate
     * it with TransactionHandle and an instance of a
     * TransactionalResource_1 interface. It is typcally used in the
     * beginning of a transaction to store some context that is
     * needed during the transaction.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] resource Instance of the TransactionResource
     * interface
     *
     * @param[in] context Pointer to data that is needed to be available
     * during the whole transaction.
     *
     * @return MafOk, or @n
     * MafNotExist if the resource is not registered in the transaction
     * or the transaction does not exist, or @n
     * MafInvalidArgument if the resource or context is null, or @n
     * one of the other MafReturnT return codes.
     *
     *
    * <br> @b Example:
    * @code
    * MafOamSpiTransaction_1T * txIf; //probably set during component initiation
    * MafOamSpiTransactionalResource_1T *myTxResIf; // set during component initiation
    * MafReturnT myJoin(TransactionHandleT txHandle){
    *     if(!txIf->registerParticipant( txHandle, myTxResIf)){
    *         // error handling
    *     }
    *     //here is a good place to create a local context
    *   txIf->setContext( txHandle,
    *                   myTxResIf,
    *                   createMyContext(txhandle) );
    *        // some more to do
    *     return MafOk;
    * }
    * @endcode
    *
     */
    MafReturnT (*setContext)(MafOamSpiTransactionHandleT txHandle,
                             MafOamSpiTransactionalResource_2T * resource,
                             void * context);
    /**
     * Get the local transaction context. The context is typically
     * needed both in TransactionalResource and in the ManagedObject
     * interface.
     *
     * @param[in] txHandle Transaction handle associated with the context.
     *
     * @param[in] resource Interface associated with the context.
     *
     * @param[out] result Location referenced by the result is set
     * to contain the context pointer that was previously set in the
     * setContext call.
     *
     * @return MafOk, or @n
     * MafNotExist if the context has not been set, or the resource is
     * not registered, or the transaction does not exist, or @n
     * MafInvalidArgument if resource or context is null, or @n
     * one of the other MafReturnT return codes.
     */
    MafReturnT (*getContext)( MafOamSpiTransactionHandleT txHandle,
                              MafOamSpiTransactionalResource_2T * resource,
                              void ** context);
    /**
     * Retrieve the lock policy for the transaction.
     *
     * @param[in] txhandle Transaction handle.
     *
     * @param[out] result Location referenced by the result is set to
     * contain the lock policy.
     *
     * @return MafOk, or @n
     * MafNotExist if the transaction does not exist, or @n
     * MafInvalidArgument if result is null, or @n
     * one of the other MafReturnT return codes.
     */
    MafReturnT (*getLockPolicy)(MafOamSpiTransactionHandleT txHandle,
                                MafLockPolicyT * result);
} MafOamSpiTransaction_2T;

#endif
