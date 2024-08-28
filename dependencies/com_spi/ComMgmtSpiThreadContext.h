#ifndef ComMgmtSpiThreadContext_h
#define ComMgmtSpiThreadContext_h
#include <ComMgmtSpiInterface_1.h>
/**
 * Describes thread context
 *
 * @file ComMgmtSpiThreadContext.h
 *
 *
 * ComMgmtSpiThreadContext provides access to the thread context and may be
 * used by COM components that provides an SPI. This SPI contains functions
 * for managing a local thread context used for error text and other data
 * related to the thread.
 *
 * If a thread is created a new thread context for the thread will be created
 * when the ComMgmtSpiThreadContext SPI is first used.
 *
 * If a function returns an error code from an SPI call, then a descriptive error
 * string may be set in the thread context before returning from the function.
 *
 * @b Example:
 * \code
 * ComReturn func(){
 *     if(failed){
 *         comMgmtSpiThreadContextSetErrorText("X failed beacuse Y");
 *         return ComFailure;
 *     }
 * }
 * @endcode
 *
 * If it is known that someone already has filled in information, it is possible
 * to add the new information in each step backwards in the call chain.
 *
 * @code
 * ComReturn func(){
 *     if(failed){
 *         comMgmtSpiThreadContextPrependErrorText("X failed beacuse Y : %s",
 *                            comMgmtSpiThreadContextGetErrorText());
 *         return ComFailure;
 *     }
 * }
 * @endcode
 *
 * The implementation of the functions is provided in comThreadContext.so.
 */

/**
 * Definition of the buffer size.
 */
#define ComMgmtSpiThreadContextBufferSize 4096

/**
 * Contains data that is thread local.
 * The context will be created when the SPI is first called in a new thread
 * and will exist during the lifetime of the thread.
 * On first call of any SPI function, thread context will be initialized with
 * error text "\0" and opaque with 0. The struct instance will be deallocated
 * when the thread terminates.
 */
typedef struct ComMgmtSpiThreadContext_1 {
    /**
     * The error text must be copied to this buffer before returned
     * by comMgmtSpiThreadContextGetErrorText ().
     */
    char  errorTextGetBuf[ComMgmtSpiThreadContextBufferSize];
    /**
     * The error text must be copied to this buffer by comMgmtSpiTheadContextSetErrorText().
     */
    char  errorTextSetBuf[ComMgmtSpiThreadContextBufferSize];
    /**
     * Opaque must be used by the thread owner to hold thread-specific data. The user setting the opaque
     * is responsible for the creation and the deletion of the data.
     */
    void* opaque;
} ComMgmtSpiThreadContext;

/**
 * Creates a thread local instance of ComMgmtSpiThreadContext.
 * It will initialize errorText with "\0" and opaque with 0.
 *
 * @return ComOk, or ComFailure if the thread context is not initialized.
 *
 * @deprecated This function is no longer required. ThreadContext service can be used
 * without explicit initialization.
 */
ComReturnT comMgmtSpiThreadContextInit();

/**
 * Gets the thread context.
 *
 * @param[in] threadContext Pointer to the thread local instance of context.
 * ComMgmtSpiThreadContext, or 0 if there is no context.
 *
 * @return ComOk, or ComFailure if thread context initialization failed.
 */
ComReturnT comMgmtSpiThreadContextGet(ComMgmtSpiThreadContext** threadContext);

/**
 * Can be used to add the error text before
 * an unsuccessful function returns. The function will fetch the thread context
 * and safely prepend the text in the thread context struct.
 *
 * @param[in] format C string that contains the text to be written to the buffer.
 * It can optionally contain embedded format tags that are substituted by the values
 * specified in the subsequent argument list. It works in the same way as printf.
 *
 * @param ... [in] Additional argument list depending on the format string.
 * It works in the same way as printf. If a string argument is null then
 * it is replaced by string "(null)".
 *
 * @return ComOk, or ComFailure if the string is too long or thread context initialization failed.
 */
ComReturnT comMgmtSpiThreadContextPrependErrorText(const char * format, ...);

/**
 * Can be used to set the error text before an unsuccessful
 * function returns. The function will fetch the thread context and safely fill
 * in the buffer (errorTextSetBuf) in the thread context struct.
 * If the thread context has not been initialized then the function will report
 * this information to stdout.
 *
 * @param[in] format C string that contains the text to be written to the buffer.
 * It can optionally contain embedded format tags that are substituted by the values
 * specified in the subsequent argument list. It works in the same way as printf.
 *
 * @param ... [in] Additional argument list depending on the format string.
 * It works in the same way as printf. If a string argument is null then
 * it is replaced by string "(null)".
 *
 * @return ComOk, or ComFailure if the string is too long or thread context initialization failed.
 */
ComReturnT comMgmtSpiThreadContextSetErrorText(const char * format, ...);

/**
 * Gets the error text from the thread context. The return
 * string is copied from the internal set buffer to the internal get buffer
 * before returned.
 *
 * @return Pointer to the error text, or 0 if the thread context initialization failed
 * or the text is too long.
 */
char * comMgmtSpiThreadContextGetErrorText();

#endif
