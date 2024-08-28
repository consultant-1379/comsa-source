#ifndef ComMwSpiTrace_1_h_
#define ComMwSpiTrace_1_h_

#include <stdint.h>
#include <ComMgmtSpiInterface_1.h>

/**
 * Trace interface.
 *
 * @file ComMwSpiTrace_1.h
 *
 * The trace service which can be used to write low level implementation centric information,
 * primarily suited for developers and field engineers often engaged in debugging.
 */


/**
 * Trace interface.
 */
typedef struct ComMwSpiTrace_1 {
    /**
       * Common interface description.
     * The "base class" for this interface. Contains component name, interface name and interface version.
     */
    ComMgmtSpiInterface_1T base;
    /**
     * A trace write request containing a trace message has been received.
     * The actual write operation towards the trace log may or may not be included in the function.
     *
     * @param[in] group Group id for the trace data.
     * The group parameter must be used to group
     * certain recordings according to any criteria.
     * In some situations only a subset of the tracing can be enabled, otherwise
     * the disturbances to the system will be too great.
     * The smallest subset is the tracing belonging to the same group.
     * How to select which groups that shall be enabled/disabled is MW SA specific.
     *
     * @param[in] traceMessageStr Null-terminated string, containing the trace message.
     *
     * @return No return value.
     */
    void (*traceWrite) (uint32_t group, const char* traceMessageStr);
    /**
     * A trace write request containing a trace enter message has been received.
     * The actual write operation towards the trace log may or may not be included in the function.
     *
     * @param[in] group Group id for the trace data.
     * The group parameter must be used to group
     * certain recordings according to any criteria.
     * In some situations only a subset of the tracing can be enabled, otherwise
     * the disturbances to the system will be too great.
     * The smallest subset is the tracing belonging to the same group.
     * How to select which groups that shall be enabled/disabled is MW SA specific.
     *
     * @param[in] funcName Name of the entered function, a null-terminated string.
     *
     * @param[in] traceMessageStr Null-terminated string, containing the trace message.
     *
     * @return No return value.
     */
    void (*traceWriteEnter) (uint32_t group, const char* funcName, const char* traceMessageStr);
    /**
     * A trace write request containing a trace exit message has been received.
     * The actual write operation towards the trace log may or may not be included in the function.
     *
     * @param[in] group Group id for the trace data.
     * The group parameter must be used to group
     * certain recordings according to any criteria.
     * In some situations only a subset of the tracing can be enabled, otherwise
     * the disturbances to the system will be too great.
     * The smallest subset is the tracing belonging to the same group.
     * How to select which groups that shall be enabled/disabled is MW SA specific.
     *
     * @param[in] funcName Name of the exited function, a null-terminated string.
     *
     * @param[in] exitCode Exit code from the function.
     *
     * @param[in] traceMessageStr Null-terminated string, containing the trace message.
     *
     * @return Not applicable.
     */
    void (*traceWriteExit) (uint32_t group, const char* funcName, uint32_t exitCode, const char* traceMessageStr);
} ComMwSpiTrace_1T;

#endif
