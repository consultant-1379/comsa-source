#ifndef ComOamSpiManagedObject_1_h
#define ComOamSpiManagedObject_1_h
#include <ComOamSpiTransaction_1.h>
#include <ComOamSpiModelRepository_1.h>
#include <stdint.h>

/**
 * Managed object interface.
 *
 * @file ComOamSpiManagedObject_1.h
 *
 * Contains functions and data definitions for accessing the MOs.
 *
 * @date 2011-01-25 ehskaja, clarified text for setMoAttribute and action
 * @date 2011-05-23 ehskaja, clarified text for newMoIterator
 * and nextMo.
 *
 */



/**
 * Defines a void pointer type for generic MO iterator.
 */
typedef void * ComOamSpiMoIteratorHandleT;

/**
 * Forward declaration.
 */
struct ComMoAttributeValueStructMember;
struct ComMoAttributeValueContainer;

/**
 * A container for one attribute value.
 */
typedef struct ComMoAttributeValue {

    union {
        /**
         * An 8-bit integer.
         */
        int8_t   i8;
        /**
         * A 16-bit integer.
         */
        int16_t  i16;
        /**
         * A 32-bit integer.
         */
        int32_t  i32;
        /**
         * A 64-bit integer.
         */
        int64_t  i64;
        /**
         * An 8-bit unsigned integer.
         */
        uint8_t  u8;
        /**
         * A 16-bit unsigned integer.
         */
        uint16_t u16;
        /**
         * A 32-bit unsigned integer.
         */
        uint32_t u32;
        /**
         * A 64-bit unsigned integer.
         */
        uint64_t u64;
        /**
         * A string value.
         */
        char *   theString;
        /**
         * An enumeration.
         */
        int16_t theEnum;
        /**
         * A boolean.
         */
        bool     theBool;
        /**
         * A distinguished name (dn).
         */
        char *   moRef;

        /**
         * A pointer to the first member of the struct.
         */
        struct ComMoAttributeValueStructMember * structMember;
    } value;
} ComMoAttributeValueT;

/**
 * Used for the struct members. A struct contains one or more
 * members.
 */
typedef struct ComMoAttributeValueStructMember {

    /**
     * Name of the member.
     */
    char * memberName;

    /**
     * Value of the member.
     */
    struct ComMoAttributeValueContainer * memberValue;

    /**
     * Pointer to the next struct member, or null if
     * there are no more structs.
     */
    struct ComMoAttributeValueStructMember *next;

} ComMoAttributeValueStructMemberT;

/**
 * A container for an attribute including the type and the value.
 * It can contain a number of attribute values
 * having the same attribute type.
 */
typedef struct ComMoAttributeValueContainer {
    /**
     * Type of the attributes in the container.
     */
    ComOamSpiMoAttributeTypeT type;
    /**
     * Number of values in the container.
     */
    unsigned int nrOfValues;
    /**
     * Pointer to the first element in the array.
     */
    struct ComMoAttributeValue *values;
} ComMoAttributeValueContainerT;

/**
 * Managed object interface.
 * This interface implements the model modification operations for
 * managed objects. The ManagedObject interface can be fetched from
 * ComMgmtSpiInterfacePortal using the identity in the
 * ComOamSpiService_1.h header file. The users of
 * ComOamSpiManagedObject may consider using ComOamSpiCmRouter
 * that automatically routes the calls to the component that
 * handles the requested part of the model. The ComOamSpiCmRouter
 * is provided in ComOamSpiCmRouter_1.h, and its identity is specified in
 * ComOamSpiService_1.h.
 *
 * The implementation of the ComOamSpiManagedObject_1 interface
 * works in close cooperation with the implementation of the
 * ComOamSpiTransactionalResource_1 interface.
 *
 * The implementation of ComOamSpiManagedObject_1 must ensure the
 * transactional ACID properties, see ComOamSpiTransactionalResource_1.h.
 *
 * It is understood that some middlewares may not support transactions
 * in a way that allows the ACID properties to be always ensured. If
 * so, the implementation must do the best it can to reduce the risk
 * of failure. However, the implementation must at all times ensure
 * safe read, that is, handle the versioning of the data so that
 * the changes done in a transaction are visible in this and only this
 * transaction until it is committed. For example, if a value is set
 * to 5 in a transaction, then when reading the value in the same
 * transaction it must read 5. In a similar way, the objects created
 * must be visible and traversable in the transaction when they have
 * been created. The objects deleted in a transaction must not be
 * visible or traversable in the transaction when they have been deleted.
 *
 * @b Parameters: @n
 * All functions takes as one of the parameters a transaction handle
 * that represents the context in which the operation takes place.
 * The character strings are null-terminated.
 *
 * Memory for out values is allocated by the implementer and freed
 * by the implementer when the transaction finishes (that is, when
 * the finish function is called on the TransactionalResource_1
 * interface).
 *
 * @b Return: @n
 * A return value other than ComOk is considered a failure.
 * The user of the interface has to decide what strategy to apply for
 * different types of failures.
 *
 * Applications applying batch operations may benefit from
 * failing the complete batch on the first failure and abort the transaction.
 *
 * Applications operating on behalf of an operator issuing operations one at
 * the time, may benefit from taking a more forgiving attitude, allowing the
 * operator to try a fix the problem without aborting the transaction.
 *
 * At failure, a descriptive text is set in
 * the thread context, see ComMgmtSpiThreadContext.h.
 */
typedef struct ComOamSpiManagedObject_1 {
    /**
     * Common interface description.
     * The "base class" for this interface contains the component
     * name, interface name, and interface version.
     */
    ComMgmtSpiInterface_1T base;
    /**
     * Sets an attribute value.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] dn MO distinguished name in 3GPP format.
     *
     * @param[in] attributeName Attribute name.
     *
     * @param[in] attributeValue Value to set. Any existing value is
     *       completely replaced by the new value. For example, if
     *       the value is a multi valued attribute then all values
     *       in the existing set is removed and replaced by the new
     *       set of values. Furthermore, if the number of values in
     *       the attributeValue container is 0 then it means that
     *       the attribute is unset. The type of \p attributeValue
     *       must not be set to \c ComOamSpiMoAttributeType_DERIVED or
     *       \c ComOamSpiMoAttributeType_VOID. The underlying basic
     *       type of derived types should be used when calling this function.
     *
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*setMoAttribute)(ComOamSpiTransactionHandleT txHandle,
                                 const char * dn,
                                 const char * attributeName,
                                 const ComMoAttributeValueContainerT * attributeValue);
    /**
     * Retrieves an attribute value. The values, arrays, and structs
     * returned by pointer values must be deallocated by the implementer of
     * this operation when the transaction is finished.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] dn MO distinguished name in 3GPP format for the MO
     * to get an attribute from.
     *
     * @param[in] attributeName Name of the attribute to get.
     *
     * @param[out] result At success, a pointer to a ComAttributeValueT
     * instance is stored at the location referenced by the result. If
     * the attribute exists but contains no values, then
     * ComMoAttributeValueContainerT::nrOfValues in the result is set to 0.
     * The type of \p result must not be set to
     * \c ComOamSpiMoAttributeType_DERIVED or \c ComOamSpiMoAttributeType_VOID.
     * The underlying basic type of derived types should be used when calling
     * this function.
     *
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getMoAttribute)(ComOamSpiTransactionHandleT txHandle,
                                 const char * dn,
                                 const char * attributeName,
                                 ComMoAttributeValueContainerT  ** result);
    /**
     * Creates an iterator for iterating over the direct children of the
     * MO specified by the dn. The scope of the iterator is limited by the
     * className parameter. The purpose of this iterator is mainly for
     * supporting those with knowledge of model meta information. For
     * example, a netconf component may use it when scanning the model
     * instance for its structure prior to fetching its contents.
     *
     * Note that its currently the responsibility of the implementation
     * to know which instances exists of the class under the dn, even
     * if the class is a root and the parent is located in another MIB.
     *
     * When using ComOamSpiCmRouter the call will be routed to the
     * implementation that is responsible for the className.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] dn Parent distinguished name of the children to
     * iterate over. The dn is in 3GPP format.
     * An empty string means that the iterator will iterate over
     * the top level MOs in the MO tree that this implementation
     * of ComOamSpiManagedObject handles. That is, iterate over the
     * roots of the specified className.
     *
     * @param[in] className Class to limit the iteration to.
     *
     * @param[out] result At success, an iterator handle is stored
     * at the location referenced by the result. NOTE! The value of
     * the iterator handle must not be reused until the
     * ComOamSpiTransactionalResource finish function has been
     * called.
     *
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*newMoIterator)(ComOamSpiTransactionHandleT txHandle,
                                const char * dn,
                                const char * className,
                                ComOamSpiMoIteratorHandleT *result);
    /**
     * Retrieves the next MO.
     *
     * Memory allocated by the implementer of this operation must be
     * deallocated by the implementer when the transaction is finished.
     *
     * Note that its currently the responsibility of the implementation
     * to know which instances exists of the class under the dn, even
     * if the class is a root and the parent is located in another MIB.
     *
     * When using ComOamSpiCmRouter the call will be routed to the
     * implementation that is responsible for the className
     * specified in the newMoIterator call.
     *
     * @param[in] itHandle MO iterator handle.
     *
     * @param[out] result At success, a pointer to the value of
     * the naming attribute is stored at the location referenced
     * by the result, or null if the iterator has no more values.
     *
     * @return ComOk, or one of the other ComReturnT return codes.
     *
     * @n @b Example:
     * @code
     * ...
     * ComOamSpiManagedObject_1T * totalModel;
     * portal->getInterface(ComCmRouter_1Id,(ComMgmtSpiInterface_1**)&totalModel);
     * ComOamSpiTransactionMaster_1T * master;
     * portal->getInterface(ComOamSpiTransactionMaster_1Id,master);
     * ComOamSpiTransactionHandleT tx;
     * master->newTransaction(ComLockReadWrite,tx);
     * ComMoIteratorHandleT it;
     * totalModel->newMoIterator(tx, "ManagedElement=1,Fm=1","FmAlarmType",&it);
     * char * key = 0;
     * ComReturnT ret = ComOk;
     * while(ret == ComOk && key == 0){
     *     ret = totalModel->nextMo(it, &key);
     *     if(ret == ComOk){
     *         ... do something with key ...
     *     }
     *     else{
     *         ... handle error code ...
     *     }
     * };
     * ...
     * @endcode
     */
    ComReturnT (*nextMo)(ComOamSpiMoIteratorHandleT itHandle,
                         char **result);
    /**
     * Creates an instance of the className identified by the dn.
     * The implementer must be able to handle the situation that
     * the mandatory attributes are set later in the same transaction.
     * Each MO class must have a unique key attribute whose name and value
     * must be specified at creation time.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] dn MO distinguished name in 3GPP format of the
     * parent MO.
     *
     * @param[in] className Class to create an instance of.
     *
     * @param[in] keyAttributeName Name of the key attribute
     *
     * @param[in] keyAttributeValue Value of the key attribute
     *
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*createMo)(ComOamSpiTransactionHandleT txHandle,
                           const char * parentDn,
                           const char * className,
                           const char * keyAttributeName,
                           const char * keyAttributevalue);
    /**
     * Recursively deletes the subtree in which the dn is the root.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] dn MO distinguished name in 3GPP format.
     *
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*deleteMo)(ComOamSpiTransactionHandleT txHandle,
                           const char * dn);

    /**
     * Executes an action. The action may be executed immediately or when
     * the transaction commits. In either case, COM expects the action to return
     * without delay.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] dn The MO distinguished name in 3GPP format.
     *
     * @param[in] name The name of the action.
     *
     * @param[in] parameters The first parameter in a
     *       null-terminated array of action parameters. The
     *       implementation must interpret the values in the same
     *       order as they appear in the definition of the action
     *       parameters in the Model Repository.
     *
     * @param[out] result At success, a pointer to a
     *                   ComMoAttributeValueContainerT
     *                   instance is stored at the location
     *                   referenced by result. The result instance
     *                   must exist during the transaction (until
     *                   commit or abort). The actual action may be
     *                   executed when the action call is made, or
     *                   later, e.g. pre or post, prepare or commit,
     *                   depending on the implementation. So the
     *                   actual execution result can only be
     *                   returned if the action is executed
     *                   immediately. A container must always be
     *                   returned in the result, even if it contains
     *                   no values, in which case the type member of
     *                   \p result must be set to ComOamSpiMoAttributeType_VOID.
     *
     * @return ComOk, or one of the other ComReturnT return codes. A failure will abort the transaction.
     */
    ComReturnT (*action)(ComOamSpiTransactionHandleT txHandle,
                         const char * dn,
                         const char * name,
                         ComMoAttributeValueContainerT **parameters,
                         ComMoAttributeValueContainerT **result);

} ComOamSpiManagedObject_1T;

#endif
