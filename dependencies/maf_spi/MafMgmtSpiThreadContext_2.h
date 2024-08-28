
#ifndef MafMgmtSpiThreadContext_2_h
#define MafMgmtSpiThreadContext_2_h

/**
 * @file MafMgmtSpiThreadContext_2.h
 * @ingroup MafMgmtSpi
 *
 * The ThreadContext version 2 interface provides thread based storage of data
 * that can be used as a context across function calls. The context consists
 * of string based messages and generic user data.
 *
 * ThreadContext SPI may be used to pass string based information across
 * function calls within a thread. When this interface is used a
 * condition resulting in a failed function call, for example when an SPI
 * function returns \c MafFailure, can be complemented with an error message
 * detailing the cause of the condition, which would otherwise only be handled
 * as a generic error return code and local log entry.
 *
 * ThreadContext also allows storage of a user data reference for each thread
 * which can be used to simplify client state handling.
 *
 */

#include <stdint.h>                  /* uint64_t               */
#include "MafMgmtSpiCommon.h"        /* MafReturnT             */
#include "MafMgmtSpiInterface_1.h"   /* MafMgmtSpiInterface_1T */

/**
 * Generic ThreadContext handle type.
 *
 * @note ThreadContext users should only use this type for parameters to
 * functions in the ThreadContext SPI.
 */
#ifndef ThreadContextHandle_2_typedef
#define ThreadContextHandle_2_typedef
typedef struct ThreadContextHandle_2 {
    /** Handle value. */
    uint64_t value;
} ThreadContextHandle_2T;
#endif
/**
 * Message category type. Message categories are a high level classification
 * of messages and used in the message handling functions. Messages added with
 * a category will only be accessible using the same category.
 *
 * Messages of any category should contain enough information and context to be
 * as self contained as possible.
 *
 * The enum defines a monotonically inreasing number of values
 * starting with the number zero.
 */
#ifndef ThreadContextMsgCategory_2_typedef
#define ThreadContextMsgCategory_2_typedef
typedef enum ThreadContextMsgCategory_2 {

    /**
     * The log category should be used for messages suitable for internal
     * exposure such as log streams. Messages added using this category can
     * be more technical in nature.
     */
    ThreadContextMsgLog_2 = 0,

    /**
     * The NBI category should be used for messages suitable for external
     * exposure on the northbound interface (NBI). This means that the
     * message content within this category should be carefully considered.
     * For example, NBI exposure of some information could be considered
     * a security issue.
     *
     * Since NBI category messages could be presented to an NBI end-user,
     * it is important that the message information and format is expressed
     * with NBI usability in mind.
     */
    ThreadContextMsgNbi_2 = 1

} ThreadContextMsgCategory_2T;
#endif
/**
 * The ThreadContext version 2 interface. This type makes up the function
 * interface of the ThreadContxt SPI.
 *
 * When calling the functions provided in this structure the caller must be
 * aware that the calls will use the current thread as an
 * implicit API handle. All state changes from calls to these
 * functions will be handled in the context of current thread and will
 * not be accessible from using this interface in another thread.
 *
 * When using messages stored by this service it is generally a good idea to
 * clear the messages afterwards to avoid excessive message storage and avoid
 * multiple usage of the same message, unless that is the intention.
 */
typedef struct MafMgmtSpiThreadContext_2 {

    /**
     * Interface identification information.
     */
    MafMgmtSpiInterface_1T base;

    /**
     * Adds a new null terminated string message to the specified category.
     * This function will store a copy of \p message as the last message of
     * the specified \p category. Messages added to the same category will
     * be retrieved in the same order as they were added.
     *
     * Adding messages to a category will not affect iterator handles in use
     * for the category, but the new messages will be available as the last
     * message of the iteration.
     *
     * Generally, information in a message should not depend on information in
     * other messages within the same category since there is no guarantee the
     * messages will be used together after retrieval.
     *
     * @param[in] category The category the added message belongs to.
     * @param[in] message The message to add.
     * @return MafOk if message was successfully added.
     */
    MafReturnT (*addMessage) (ThreadContextMsgCategory_2T category, const char* message);

    /**
     * Creates a new message iterator for messages of the specified category.
     * The handle set by this function will be valid as parameter to the
     * message iterating functions of this SPI. The content of the
     * iterator is a snapshot of all messages present when the
     * iterator is created.
     *
     * When an iterator handle is no longer needed, the handle should be
     * released by calling \c releaseHandle().
     *
     * @param[in] category Message category for the new iterator.
     * @param[out] handle Handle to the newly created iterator.
     * @return MafOk if if an iterator was successfully created. If MafOk is
     *         returned, \p iteratorHandle refers to the new iterator and can be
     *         used as parameter to message handling functions.
     */
    MafReturnT (*newMessageIterator) (ThreadContextMsgCategory_2T category, ThreadContextHandle_2T* handle);

    /**
     * Gets the next message in the iteration refered to by the provided
     * handle. After a message has been return by this function, the
     * iterator will be advanced and refer to the following message.
     *
     * When the iteration is complete as indicated by this function,
     * the iterator handle can no longer be used as parameter to iterator
     * functions. When this function indicate end of iteration,
     * \c releaseHandle() does not have to be called for the used iterator
     * handle.
     *
     * @param[in]  iteratorHandle iteratorHandle of the ongoing iteration.
     * @param[out] message If MafOk is returned, \p message points to the next
     *             current message in the iteration or NULL if the iteration has
     *             reached the end.
     * @return MafOk if \p next message could be returned successfully.
     */
    MafReturnT (*nextMessage)  (ThreadContextHandle_2T handle, const char** message);

    /**
     * Clear all messages for a specified category. After this call, the
     * message count for the category will be zero. Any iterator handle
     * in use for the cleared category will be invalidated and can no
     * longer be used for iteration.
     *
     * @param[in] category The category for the message count.
     * @return MafOk if all \p category messages could be cleared successfully.
     */
    MafReturnT (*clearMessages) (ThreadContextMsgCategory_2T category);

    /**
     * Gets the number of messages currently stored in the specified category.
     *
     * @param[in] category The category for the message count.
     * @param[out] count The number of messages available in \p category if the
     *                   call was successful.
     * @return MafOk if the message count could be returned successfully.
     */
    MafReturnT (*messageCount)  (ThreadContextMsgCategory_2T category, uint32_t* count);

    /**
     * Gets all available messages in a specified category. The messages will
     * be returned as a null terminated array of strings. The
     * contents of the array is a snapshot of the available messages
     * when the function is called.
     *
     * When the messages retrieved using this function is no longer needed,
     * the associated handle should be released by calling \c releaseHandle().
     * The handle may only be used as parameter to \c releaseHandle().
     *
     * @param[in] category The category of the messages to get.
     * @param[out] msgHandle The handle associated with the returned messages.
     * @param[out] messages If successful, messages will be set to a null
     *             terminated array of all message strings in \p category.
     * @return MafOk if messages was successfully retrieved.
     */
    MafReturnT (*getMessages)  (ThreadContextMsgCategory_2T category, ThreadContextHandle_2T* msgHandle, const char*** messages);

    /**
     * Release a ThreadContext handle. This function should be called when a
     * handle and its associated data is no longer needed. After the call,
     * \p handle will no longer be valid in other ThreadContext functions and
     * associated resources may have been deallocated.
     *
     * @note After calling this function, any pointers retrieved using this
     * handle must not be used.
     *
     * @param handle The handle to release.
     * @return MafOk if handle was released successfully.
     */
    MafReturnT (*releaseHandle) (ThreadContextHandle_2T handle);

} MafMgmtSpiThreadContext_2T;


#endif
