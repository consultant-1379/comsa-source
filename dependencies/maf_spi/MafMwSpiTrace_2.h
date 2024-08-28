#ifndef MafMwSpiTrace_2_h_
#define MafMwSpiTrace_2_h_

#include <stdint.h>
#include <stdarg.h>
#include <MafMgmtSpiInterface_1.h>
/**
 * Trace interface.
 *
 * @file MafMwSpiTrace_2.h
 * @ingroup MafMwSpi
 *
 * The trace service which can be used to write low level implementation
 * centric information,
 * primarily suited for developers and field engineers often engaged in
 * debugging.
 * In addition to information that normally is associated to tracing
 * information sent to logging interface is expected to be duplicated on this interface.
 * This is to get a fairly complete picture of the system.
 *
 * It is strongly recommended that an implementer of this interface provides
 * mechanisms to enable/disable traces on per domain and level in run time.
 * If LTTng is chosen as the underlying trace framework this is possible.
 *
 * It is strongly recommended that all components running in MAF's process
 * use this interface for tracing.
 */

/**
 * The MafMwSpiTraceLevelT enum describes the level of trace.
 *
 * This information corresponds to trace levels in LTTng.
 *
 * The levels go from 0 to 14. Higher numbers imply the most
 * verbosity (higher event throughput expected).
 *
 * Levels 0 through 6, and level 14, match syslog(3) levels
 * semantic. Levels 7 through 13 offer more fine-grained selection of
 * debug information.
 */
typedef enum {
    /*
     * system is unusable
     */
    MafMwSpiTraceEmerg = 0,
    /*
     * action must be taken immediately
     */
    MafMwSpiTraceAlert = 1,
    /*
     * critical conditions
     */
    MafMwSpiTraceCrit = 2,
    /*
     * error conditions
     */
    MafMwSpiTraceErr = 3,
    /*
     * warning conditions
     */
    MafMwSpiTraceWarning = 4,
    /*
     * normal, but significant, condition
     */
    MafMwSpiTraceNotice = 5,
    /*
     * informational message
     */
    MafMwSpiTraceInfo  = 6,
    /*
     * debug information with system-level scope (set of programs)
     * This level is not expected to be used by applications running on MAF
     * but it is included for easier mapping to LTTng.
     */
    MafMwSpiTraceDebugSystem = 7,
    /*
     * debug information with program-level scope (set of processes)
     */
    MafMwSpiTraceDebugProgram = 8,
    /*
    * debug information with process-level scope (set of modules)
    */
    MafMwSpiTraceDebugProcess = 9,
    /*
     * debug information with module (executable/library) scope (set of units)
     */
    MafMwSpiTraceDebugModule = 10,
    /*
     * debug information with compilation unit scope (set of functions)
     */
    MafMwSpiTraceDebugUnit = 11,
    /*
     * debug information with function-level scope
     */
    MafMwSpiTraceDebugFunction = 12,
    /*
     * debug information with line-level scope
     */
    MafMwSpiTraceDebugLine = 13,
    /*
     * debug-level lower than line
     */
    MafMwSpiTraceDebug = 14
} MafMwSpiTraceLevelT;


/**
 * Trace subsytem handle. The handle value is opaque to application and MAF,
 * It will be used by the SA to map handles to registered subsystem strings.
 */
typedef struct {
    uint64_t handle;
} MafMwSpiTraceSubsystemHandle_2T;



/**
 * The following mapping procedure to map the interface to LTTng is strongly recommended.
 *
 * Define com_ericsson_common_abc as the provider name where abc is the application.
 * i.e. for COM abc == com
 * Design one tracepoint event for each trace level.
 *
 * Example: for the trace provider com_ericsson_common_maf
 *          there will be the following tracepoint event names.
 *
 *          Emerg, Alert, ... Debug.
 *
 *          The trace level for the tracepoint event should be mapped to the
 *          corresponding level.
 *
 * The fields in these LTTng tracepoint events would be
 * subsystem, file name, line number, function name and the actual trace message.
 */

/**
 * Trace interface.
 */
typedef struct MafMwSpiTrace_2 {
    /**
     * Common interface description.
     * The "base class" for this interface.
     * Contains component name, interface name and interface version.
     */
    MafMgmtSpiInterface_1T base;

    /**
     * Registers a subsystem identifier in trace component. This done to ensure that subsystem identifiers
     * are unique within an application. The subsystem string shall be copied and stored by the component
     * implementing this interface.
     * The number of subsystems supported by an SA is SA specific but at least 256 is strongly recommended.
     * The SA shall enforce that the registered subsystem identifiers are unique.
     * It is strongly recommended that a susystem identifier is prefixed with an application specific string.
     * Examples: MafSvs, ComSchema.
     *
     * @param [in]  domain  the domain
     * @param [out] domainHandle  a handle representing the domain,
                    it will only be set to a valid value if registration was successful
     * @return     MafOk on success or MafFailure in case of failure.
     */
    MafReturnT (*registerSubsystem)(const char* subsystem, MafMwSpiTraceSubsystemHandle_2T* subsystemHandle);

    /**
     * A trace write request containing a trace message has been received.
     * The actual write operation towards the trace log may or may not be included in the function.
     * It is the implementers responsibility's to add additionally information like time stamps etc.
     * The processing of the fields; file, format and args should only be done if the tracing
     * is enabled for the level.
     *
     * @param [in] subsystem  the handle to a registered subsystem.
     *             The registered string value shall be in the trace output.
     *             Unknown values should result in "Unknown" in the subsystem field in the trace output.
     * @param [in] level  the level as specified above.
     * @param [in] file  the file where the trace was generated.
     *             This is the raw string as given by the system.
     *             The path prefix should be removed before outputting trace information.
     *             I.e normally the string will look something like this /a/b/c/foo.c
     *             then the output should be foo.c.
     * @param [in] line  the line number where the trace was generated.
     * @param [in] func  the function name where the trace was generated.
     * @param [in] format  a printf style format string.
     *             I.e a string similar to this "a=%s, b=%d"
     * @param [in] args  a va_list of arguments.
     *             It should be used together with the format string to construct the
     *             the actual message.
     *
     * @return No return value.
     */
    void (*traceWrite) (MafMwSpiTraceSubsystemHandle_2T subsystem, MafMwSpiTraceLevelT level, const char* file, uint32_t line, const char* func, const char* format, va_list args);
} MafMwSpiTrace_2T;

#endif
