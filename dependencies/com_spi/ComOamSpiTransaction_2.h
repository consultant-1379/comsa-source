#ifndef ComOamSpiTransaction_2_h
#define ComOamSpiTransaction_2_h
#include <ComOamSpiTransaction_1.h>
#include <ComOamSpiTransactionalResource_2.h>
#include <ComMgmtSpiInterface_1.h>

/**
 * Transaction interface.
 *
 * @file ComOamSpiTransaction_2.h
 *
 * The ComOamSpiTransaction interface provides access to the
 * transaction functionality required by the participants in a
 * transaction. A participant implements
 * the ComOamSpiTransactionalResource interface.
 *
 * The ComOamSpiTransaction interface can be fetched from ComMgmtSpiInterfacePortal
 * using the identity ComOamSpiTransaction_1Id definition that
 * is provided by ComOamSpiServiceIdentities_1.h.
 *
 * @b Parameters <br>
 * All functions takes as one of the parameters a transaction
 * handle that represents the context in which the operation
 * takes place.
 *
 * @b Return <br>
 * A return value other than ComOk is considered a failure.
 * At failure, a descriptive text is set in the thread context,
 * see ComMgmtSpiThreadContext.h.
 */

/**
 * Locking policy that the transaction must use.
 *
 * @b ComLockWrite <br>
 * No lock is acquired on MOs for read operations and the last committed data
 * is returned. This is also true if the lock is already held by an ongoing
 * Read/Write transaction.
 *
 * @b ComLockReadWrite <br>
 * The lock is acquired on all MOs that are modified and the lock is acquired
 * on all MOs that are accessed.
 */


/**
 * Transaction interface.
 */
typedef struct ComOamSpiTransaction_2 {
    /**
     * Common interface description.
     * The "base class" for this interface contains the component
     * name, interface name, and interface version.
     */
    ComMgmtSpiInterface_1T base;
    /**
     * Register a participant in the transaction.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] resource Pointer to the interface instance
     * that will participate.
     *
     * @return ComOk, or @n
     * ComNotExist if the transaction does not exist, or @n
     * ComInvalidArgument if resource is null, or @n
     * one of the other ComReturnT return codes.
     *
     * <br>The function is typically called in the
     * TransactionalResource::join function.
     *
     * @b Example:
     * @code
     * TransactionalResource_1T * myTxResource; // instance set at component initiation
     * TransactionInterface_1T * txIf; // instance probably set at component initiation
     * ComReturnT myJoinImpl(ComOamSpiTransactionHandleT txHandle){
     *     ...
     *     if (!txIf->registerParticipant(txHandle, myTxResource))
     *         { ...error handling... }
     *     return ComOk;
     * }
     * @endcode
     */
    ComReturnT (*registerParticipant)(ComOamSpiTransactionHandleT txHandle,
                                      ComOamSpiTransactionalResource_2T * resource);
    /**
     * Set a pointer to something in the local thread and associate
     * it with TransactionHandle and an instance of a
     * TransactionalResource_1 interface. It is typically used in the
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
     * @return ComOk, or @n
     * ComNotExist if the resource is not registered in the transaction
     * or the transaction does not exist, or @n
     * ComInvalidArgument if the resource or context is null, or @n
     * one of the other ComReturnT return codes.
     *
     *
    * <br> @b Example:
    * @code
    * ComOamSpiTransaction_1T * txIf; //probably set during component initiation
    * ComOamSpiTransactionalResource_1T *myTxResIf; // set during component initiation
    * ComReturnT myJoin(TransactionHandleT txHandle){
    *     if(!txIf->registerParticipant( txHandle, myTxResIf)){
    *         // error handling
    *     }
    *     //here is a good place to create a local context
    *   txIf->setContext( txHandle,
    *                   myTxResIf,
    *                   createMyContext(txhandle) );
    *        // some more to do
    *     return ComOk;
    * }
    * @endcode
    *
     */
    ComReturnT (*setContext)(ComOamSpiTransactionHandleT txHandle,
                             ComOamSpiTransactionalResource_2T * resource,
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
     * @return ComOk, or @n
     * ComNotExist if the context has not been set, or the resource is
     * not registered, or the transaction does not exist, or @n
     * ComInvalidArgument if resource or context is null, or @n
     * one of the other ComReturnT return codes.
     */
    ComReturnT (*getContext)( ComOamSpiTransactionHandleT txHandle,
                              ComOamSpiTransactionalResource_2T * resource,
                              void ** context);
    /**
     * Retrieve the lock policy for the transaction.
     *
     * @param[in] txhandle Transaction handle.
     *
     * @param[out] result Location referenced by the result is set to
     * contain the lock policy.
     *
     * @return ComOk, or @n
     * ComNotExist if the transaction does not exist, or @n
     * ComInvalidArgument if result is null, or @n
     * one of the other ComReturnT return codes.
     */
    ComReturnT (*getLockPolicy)(ComOamSpiTransactionHandleT txHandle,
                                ComLockPolicyT * result);
} ComOamSpiTransaction_2T;

#endif
