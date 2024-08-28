#ifndef MafOamSpiTransactionMaster_2_h
#define MafOamSpiTransactionMaster_2_h

#include <MafOamSpiTransaction_2.h>

#ifndef __cpluplus
#include <stdbool.h>
#endif

/**
 * Transaction master.
 *
 * @file MafOamSpiTransactionMaster_2.h
 * @ingroup MafOamSpi
 *
 * The MafOamSpiTransactionMaster interface provides the access
 * to the transaction functionality needed by the one that
 * initiates the transaction. The interface can be fetched from
 * MafMgmtSpiInterfacePortal using the identity
 * MafOamSpiTransactionMaster_1Id definition that is provided by
 * MafOamSpiServiceIdentities_1.h.
 *
 * @b Parameters: <br>
 * All functions, except newTransaction, takes as one of the
 * parameters a transaction handle that represents the context
 * in which the operation takes place.
 *
 * @b Return: <br>
 * A return value other than MafOk is considered a failure.
 * At failure, a descriptive text is set in the thread context,
 * see MafMgmtSpiThreadContext.h.
 */


/**
 * Transaction master interface.
 */
typedef struct MafOamSpiTransactionMaster_2 {
    /**
     * Mafmon interface description.
     * The "base class" for this interface contains the
     * component name, interface name, and interface version.
     */
    MafMgmtSpiInterface_1T base;
    /**
     * Start a new transaction.
     *
     * @param[in] policy Lock policy for the transaction.
     *
     * @param[in] timeout The transaction timeout in seconds.
     * If the transaction has not been used for the duration
     * specified by timeout, then the transaction will be aborted.
     * The timer is started when the transaction is first used in an
     * MO SPI call. The timer will not expire during
     * outstanding MO SPI calls. The max value for timeout is 86400.
     * The Transaction Service will remember the aborted transaction
     * for another 3600 seconds or until someone tries to use the
     * timed out transaction, in which case MafTimeOut will be
     * replied. After that the transaction will be completely
     * forgotten.
     *
     * @param[out] result Location referenced by the result
     * is set to contain the transaction handle.
     *
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*newTransaction)(MafLockPolicyT policy,
                                 unsigned int timeout,
                                 MafOamSpiTransactionHandleT *result);
    /**
     * Commits the transaction.
     *
     * @param[in] txhandle Transaction handle identifying the transaction.
     *
     * @return MafOk, or @n
     * MafCommitFailed if one or more participants failed to commit, or @n
     * MafPrepareFailed if a participant failed to prepare, or @n
     * MafValidationFailed if the validation failed.@n
     * MafNotExist if the transaction does not exist or no
     * participants are registered, or @n
     * one of the other MafReturnT return codes.
     *
     * @b Details: <br>
     * 1)
     * Validate will be called on all validators. If validation does
     * not pass then abort will be called on all perticipants and
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
     * 4)
     * At last, finish is called on all participants, giving them an
     * opportunity to clean up.
     *
     * @b Example @b excluding @b error @b handling:
     *
     * @code
     * ... include all needed interface files ...
     *
     * MafOamSpiTransactionalResurce_2T * resIf1,resIf2;
     * MafOamSpiManagedObject_2T * moIf1,moIf2;
     * MafOamSpiTransactionMaster_2T *master;
     * portal->getInterface(SomeResource_1Id, &resIf1);
     * portal->getInterface(SomeManagedObject_1Id, &moIf1);
     * portal->getInterface(SomeOtherResource_2Id, &resIf2);
     * portal->getInterface(SomeOtherManagedObject_1Id, &moIf2);
     * portal->getInterface(MafOamSpiTransactionMaster_2Id,&master);
     *
     * MafOamSpiTransactionhandleT tx;
     * master->newTransaction(MafLockReadWrite,tx);
     * resIf1->join(tx);
     * resIf2->join(tx);
     * ... prepare some attributes then ...
     *
     * moIf1->set(tx,someAttribute);
     * moIf2->set(tx,someOtherAttribute);
     * master->commit();
     * @endcode
     */
    MafReturnT (*commit)(MafOamSpiTransactionHandleT txHandle);
    /**
     * Aborts the transaction by calling abort on all participants.
     * The initiator typically calls abort as a response to a ManagedObject
     * replying with an error code to some function call done prior to commit.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @return MafOk, or @n
     * MafNotExist if the transaction does not exist, or @n
     * one of the other MafReturnT return codes.
     */
    MafReturnT (*abort) (MafOamSpiTransactionHandleT txHandle);

    /**
     * explicitPrepare can be used by the transaction initiator to
     * gain more control over the 2 phase commit protocol, when mapping
     * from a Mw database to, for example, an Object implementer in
     * COM.
     * prepare is normally called on all participants in the transaction
     * as a part of the 2 phase commit protocol. After all participants
     * has responded MafOk on the prepare, then commit is called on all
     * participants.
     *
     * explicit prepare gives the transaction initiator the oportunity
     * to act on an error from the prepare call. For example to feedback
     * return this fact to a MW database to which the transaction is mapped.
     *
     * @param[in] txhandle Transaction handle identifying the transaction.
     *
     * @return MafOk, or @n
     * MafPrepareFailed if one or more participants failed to prepare, or @n
     * MafNotExist if the transaction does not exist or no participants are registered, or @n
     * one of the other relevant MafReturnT return codes.
     *
     * @b Details: <br>
     * If explicit prepare has been successfully called then the only
     * possible allowed call is commit or abort.
     * And if commit is called, then prepare will
     * not be called on the participants as it has already been called in
     * the explicitPrepare call. If the explicit prepare fails then
     * the transaction will be automatically aborted.
     *
     */
    MafReturnT (*explicitPrepare)(MafOamSpiTransactionHandleT txHandle);

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
     * @return MafOk, or @n
     * MafNotExist if the transaction does not exist, or @n
     * MafInvalidArgument if a value is null, @n
     * or one of the other MafReturnT return codes.
     */
    MafReturnT (*isRegistered)( MafOamSpiTransactionHandleT txHandle,
                                MafOamSpiTransactionalResource_2T * participant,
                                bool *result);

    /**
     * The transaction master will call validate on all validators.
     *
     * @param[in] txHandle Transaction handle identifying the
     * transaction.
     *
     * @param[out] result true if the validation succeeded, false if
     * it failed.
     * @return MafOk, or @n
     * MafNotExist if the transaction does not exist, or @n
     * MafInvalidArgument if a value is null, @n
     * or one of the other MafReturnT return codes.
     */
    MafReturnT (*validate)(MafOamSpiTransactionHandleT txHandle,
                           bool *result);

} MafOamSpiTransactionMaster_2T;

#endif
