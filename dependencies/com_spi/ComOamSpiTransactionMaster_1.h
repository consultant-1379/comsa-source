#ifndef ComOamSpiTransactionMaster_1_h
#define ComOamSpiTransactionMaster_1_h

#include <ComOamSpiTransaction_1.h>

#ifndef __cpluplus
#include <stdbool.h>
#endif

/**
 * Transaction master.
 *
 * @file ComOamSpiTransactionMaster_1.h
 *
 * The ComOamSpiTransactionMaster interface provides the access
 * to the transaction functionality needed by the one that
 * initiates the transaction. The interface can be fetched from
 * ComMgmtSpiInterfacePortal using the identity
 * ComOamSpiTransactionMaster_1Id definition that is provided by
 * ComOamSpiServiceIdentities_1.h.
 *
 * @b Parameters: <br>
 * All functions, except newTransaction, takes as one of the
 * parameters a transaction handle that represents the context
 * in which the operation takes place.
 *
 * @b Return: <br>
 * A return value other than ComOk is considered a failure.
 * At failure, a descriptive text is set in the thread context,
 * see ComMgmtSpiThreadContext.h.
 */


/**
 * Transaction master interface.
 */
typedef struct ComOamSpiTransactionMaster_1 {
    /**
     * Common interface description.
     * The "base class" for this interface contains the
     * component name, interface name, and interface version.
     */
    ComMgmtSpiInterface_1T base;
    /**
     * Start a new transaction.
     *
     * @param[in] policy Lock policy for the transaction.
     *
     * @param[in] timeout The transaction timeout in seconds.
     * If the transaction has not been used for the duration
     * specified by timeout, then the transaction will be aborted.
     * The max value for timeout is 86400.
     * The Transaction Service will remember the aborted
     * transaction for another 3600 seconds or until someone
     * tries to use the timed out transaction, in which case
     * ComTimeOut will be replied. After that the transaction
     * will be completely forgotten.
     *
     * @param[out] result Location referenced by the result
     * is set to contain the transaction handle.
     *
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*newTransaction)(ComLockPolicyT policy,
                                 unsigned int timeout,
                                 ComOamSpiTransactionHandleT *result);
    /**
     * Commits the transaction.
     *
     * @param[in] txhandle Transaction handle identifying the transaction.
     *
     * @return ComOk, or @n
     * ComCommitFailed if one or more participants failed to commit, or @n
     * ComPrepareFailed if a participant failed to prepare, or @n
     * ComValidationFailed if the validation failed.@n
     * ComNotExist if the transaction does not exist or no
     * participants are registered, or @n
     * one of the other ComReturnT return codes.
     *
     * @b Details: <br>
     *
     * 1) Validate will be called on all validators. If validation does
     * not pass then abort will be called on all participants and
     * the commit will return an error code.@n
     *
     * 2) If validate succeeds then Prepare will be called on the
     * participants. If a participant responds with an error code
     * then prepare will not be called on the remaining participants
     * and abort will be called on all participants.@n
     *
     * 3) If prepare succeeded for participants succeed then commit
     * will always be called on all participants. If one or more
     * participants commit fails then the potential resulting
     * inconsistency must be resolved by a human.@n
     *
     *
     * 4) At last, finish is called on all participants, giving them an
     * opportunity to clean up.
     *
     * @b Example @b excluding @b error @b handling:
     *
     * @code
     * ... include all needed interface files ...
     *
     * ComOamSpiTransactionalResurce_1T * resIf1,resIf2;
     * ComOamSpiManagedObject_1T * moIf1,moIf2;
     * ComOamSpiTransactionMaster_1T *master;
     * portal->getInterface(SomeResource_1Id, &resIf1);
     * portal->getInterface(SomeManagedObject_1Id, &moIf1);
     * portal->getInterface(SomeOtherResource_2Id, &resIf2);
     * portal->getInterface(SomeOtherManagedObject_1Id, &moIf2);
     * portal->getInterface(ComOamSpiTransactionMaster_1Id,&master);
     *
     * ComOamSpiTransactionhandleT tx;
     * master->newTransaction(ComLockReadWrite,tx);
     * resIf1->join(tx);
     * resIf2->join(tx);
     * ... prepare some attributes then ...
     *
     * moIf1->set(tx,someAttribute);
     * moIf2->set(tx,someOtherAttribute);
     * master->commit();
     * @endcode
     */
    ComReturnT (*commit)(ComOamSpiTransactionHandleT txHandle);
    /**
     * Aborts the transaction by calling abort on all participants.
     * The initiator typically calls abort as a response to a ManagedObject
     * replying with an error code to some function call done prior to commit.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @return ComOk, or @n
     * ComNotExist if the transaction does not exist, or @n
     * one of the other ComReturnT return codes.
     */
    ComReturnT (*abort) (ComOamSpiTransactionHandleT txHandle);
    /**
     * Answers the question if the participant is registered in the transaction.
     *
     * @param[in] txHandle Transaction handle identifying the transaction.
     *
     * @param[in] participant Participant to enquire about.
     *
     * @param[out] result Location referenced by the result is set to
     * true if the participant is registered, otherwise it is set to false.
     *
     * @return ComOk, or @n
     * ComNotExist if the transaction does not exist, or @n
     * ComInvalidArgument if a value is null, @n
     * or one of the other ComReturnT return codes.
     */
    ComReturnT (*isRegistered)( ComOamSpiTransactionHandleT txHandle,
                                ComOamSpiTransactionalResource_1T * participant,
                                bool *result);
} ComOamSpiTransactionMaster_1T;

#endif
