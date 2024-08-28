#ifndef MafMgmtSpiThreadContext_h
#define MafMgmtSpiThreadContext_h
#include <MafMgmtSpiInterface_1.h>
/**
 * Describes thread context
 *
 * @file MafMgmtSpiThreadContext.h
 * @ingroup MafMgmtSpi
 *
 * MafMgmtSpiThreadContext provides access to the thread context and may be
 * used by MAF components that provides an SPI. This SPI contains functions
 * for managing a local thread context used for error text and other data
 * related to the thread.
 *
 * If a thread is created a new thread context for the thread will be created
 * when the MafMgmtSpiThreadContext SPI is first used.
 *
 * If a function returns an error code from an SPI call, then a descriptive error
 * string may be set in the thread context before returning from the function.
 *
 * @b Example:
 * \code
 * MafReturn func(){
 *     if(failed){
 *         mafMgmtSpiThreadContextSetErrorText("X failed beacuse Y");
 *         return MafFailure;
 *     }
 * }
 * @endcode
 *
 * If it is known that someone already has filled in information, it is possible
 * to add the new information in each step backwards in the call chain.
 *
 * @code
 * MafReturn func(){
 *     if(failed){
 *         mafMgmtSpiThreadContextPrependErrorText("X failed beacuse Y : %s",
 *                            mafMgmtSpiThreadContextGetErrorText());
 *         return MafFailure;
 *     }
 * }
 * @endcode
 *
 * The implementation of the functions is provided in mafThreadContext.so.
 */

/**
 * Definition of the buffer size.
 */
#define MafMgmtSpiThreadContextBufferSize 4096

/**
 * Contains data that is thread local.
 * The context will be created when the SPI is first called in a new thread
 * and will exist during the lifetime of the thread.
 * On first call of any SPI function, thread context will be initialized with
 * error text "\0" and opaque with 0. The struct instance will be deallocated
 * when the thread terminates.
 */
typedef struct MafMgmtSpiThreadContext_1 {
    /**
     * The error text must be copied to this buffer before returned
     * by mafMgmtSpiThreadContextGetErrorText ().
     */
    char  errorTextGetBuf[MafMgmtSpiThreadContextBufferSize];
    /**
     * The error text must be copied to this buffer by mafMgmtSpiTheadContextSetErrorText().
     */
    char  errorTextSetBuf[MafMgmtSpiThreadContextBufferSize];
    /**
     * Opaque must be used by the thread owner to hold thread-specific data. The user setting the opaque
     * is responsible for the creation and the deletion of the data.
     */
    void* opaque;
} MafMgmtSpiThreadContext;

/**
 * Creates a thread local instance of MafMgmtSpiThreadContext.
 * It will initialize errorText with "\0" and opaque with 0.
 *
 * @return MafOk, or MafFailure if the thread context is not initialized.
 *
 * @deprecated This function is no longer required. ThreadContext service can be used
 * without explicit initialization.
 */
MafReturnT mafMgmtSpiThreadContextInit();

/**
 * Gets the thread context.
 *
 * @param[in] threadContext Pointer to the thread local instance of context.
 * MafMgmtSpiThreadContext, or 0 if there is no context.
 *
 * @return MafOk, or MafFailure if thread context initialization failed.
 */
MafReturnT mafMgmtSpiThreadContextGet(MafMgmtSpiThreadContext** threadContext);

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
 * @return MafOk, or MafFailure if the string is too long or thread context initialization failed.
 */
MafReturnT mafMgmtSpiThreadContextPrependErrorText(const char * format, ...);

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
 * @return MafOk, or MafFailure if the string is too long or thread context initialization failed.
 */
MafReturnT mafMgmtSpiThreadContextSetErrorText(const char * format, ...);

/**
 * Gets the error text from the thread context. The return
 * string is copied from the internal set buffer to the internal get buffer
 * before returned.
 *
 * @return Pointer to the error text, or 0 if the thread context initialization failed
 * or the text is too long.
 */
char * mafMgmtSpiThreadContextGetErrorText();

#endif
