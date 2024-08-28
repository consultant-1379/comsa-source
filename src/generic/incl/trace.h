/******************************************************************************
 *   Copyright (C) 2010 by Ericsson AB
 *   S - 125 26  STOCKHOLM
 *   SWEDEN, tel int + 46 10 719 0000
 *
 *   The copyright to the computer program herein is the property of
 *   Ericsson AB. The program may be used and/or copied only with the
 *   written permission from Ericsson AB, or in accordance with the terms
 *   and conditions stipulated in the agreement/contract under which the
 *   program has been supplied.
 *
 *   All rights reserved.
 *
 * 
 *   File:   trace.h
 * 
 *   Author: efaiami
 *
 *   Date:   2011-03-14
 * 
 *****************************************************************************/

#include <syslog.h>
#ifndef TRACE_H_DEFINED
#define TRACE_H_DEFINED

extern int trace_flag;

/*
 * Log levels, compliant with syslog and SA Forum Log spec.
 */
#define LOG_LEVEL_EMERG     LOG_EMERG
#define LOG_LEVEL_ALERT     LOG_ALERT
#define LOG_LEVEL_CRIT      LOG_CRIT
#define LOG_LEVEL_ERROR     LOG_ERR
#define LOG_LEVEL_WARNING   LOG_WARNING
#define LOG_LEVEL_SECURITY  LOG_WARNING // openais specific
#define LOG_LEVEL_NOTICE    LOG_NOTICE
#define LOG_LEVEL_INFO      LOG_INFO
#define LOG_LEVEL_DEBUG     LOG_DEBUG

/*
 * Trace tags, used by trace macros, uses 32 bits => 32 different tags
 */
#define TRACE_TAG_LOG     (1<<0)
#define TRACE_TAG_ENTER   (1<<1)
#define TRACE_TAG_LEAVE   (1<<2)
#define TRACE_TAG_TRACE1  (1<<3)
#define TRACE_TAG_TRACE2  (1<<4)
#define TRACE_TAG_TRACE3  (1<<5)
#define TRACE_TAG_TRACE4  (1<<6)
#define TRACE_TAG_TRACE5  (1<<7)
#define TRACE_TAG_TRACE6  (1<<8)
#define TRACE_TAG_TRACE7  (1<<9)
#define TRACE_TAG_TRACE8  (1<<10)

/*
 * Trace modes
 */
#define LOG_MODE_TIMESTAMP  2
#define LOG_MODE_FILE       4
#define LOG_MODE_SYSLOG     8
#define LOG_MODE_STDERR     16
#define LOG_MODE_FILELINE   32
#define LOG_MODE_STDOUT     64

/**
 * Com_SA Internal Trace
 */

#define _TRACE_FLAG

/*
 * Trace functions
 */
#ifdef __cplusplus
/*
 * If this stuff is to be used from within C++, it must be declared
 * C-linked structures and methods.
 */
extern "C" {
#endif

/*
 * Global trace variable
 */
struct log_state_t {
    int level;
    int tags;
    int mode;
};

extern struct log_state_t gLog;

void    log_init (const char *ident);
void    log_to_file(const char* logfile);
void    log_control(const struct log_state_t*, struct log_state_t* old);
void    log_control_from_string(const char* string, struct log_state_t* old);

void    log_printf(const char *file, int line, int priority, int category, const char *format, ...);
void    enter_leave_log_printf(const char *file, int line, int priority, int category, const char *format, ...);

/**
 * Checks if the given libray name is loaded in the process memory
 *
 * @param probeName     probe library name
 * @return 0 if library is found and something else when it's not found
 */
int isTraceCCProbeLoaded(const char *probeName);

#ifdef __cplusplus
}
#endif

/*
 * Trace macros
 */
#ifdef _lint

#define LOG_PRINTF(lvl, format, args...)  /* void */
#define LOG_DPRINTF(format, args...)      /* void */
#define ENTER()                           /* void */
#define ENTER_ARGS(format, args...)       /* void */
#define LEAVE()                           /* void */
#define TRACE1(format, args...)           /* void */
#define TRACE2(format, args...)           /* void */
#define TRACE3(format, args...)           /* void */
#define TRACE4(format, args...)           /* void */
#define TRACE5(format, args...)           /* void */
#define TRACE6(format, args...)           /* void */
#define TRACE7(format, args...)           /* void */
#define TRACE8(format, args...)           /* void */

#else

#define LOG_PRINTF(lvl, format, args...) do { \
    if ((lvl) <= gLog.level)    { \
        log_printf(__FILE__, __LINE__, lvl, 0, format, ##args);  \
    } \
} while(0)

#define LOG_DPRINTF(format, args...) do { \
    if (LOG_LEVEL_DEBUG <= gLog.level)    { \
        log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 0, format, ##args);  \
    } \
} while(0)

#ifdef _TRACE_FLAG
#define ENTER() do { \
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
    } \
} while(0)
#else
#define ENTER()
#endif

#define ENTER_ARGS(format, args...) do { \
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_ENTER & gLog.tags)) { \
        log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s: " format, __FUNCTION__, ##args); \
    } \
} while(0)

#ifdef _TRACE_FLAG
#define LEAVE() do { \
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_LEAVE & gLog.tags)) { \
        enter_leave_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
    } \
} while(0)
#else
#define LEAVE()
#endif

#define TRACE1(format, args...) do { \
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_TRACE1 & gLog.tags)) { \
        log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 1, format, ##args);  \
    } \
} while(0)

#define TRACE2(format, args...) do { \
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_TRACE2 & gLog.tags)) { \
        log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 2, format, ##args);  \
    } \
} while(0)

#define TRACE3(format, args...) do { \
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_TRACE3 & gLog.tags)) { \
        log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 3, format, ##args);  \
    } \
} while(0)

#define TRACE4(format, args...) do { \
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_TRACE4 & gLog.tags)) { \
        log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 4, format, ##args);  \
    } \
} while(0)

#define TRACE5(format, args...) do { \
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_TRACE5 & gLog.tags)) { \
        log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 5, format, ##args);  \
    } \
} while(0)

#define TRACE6(format, args...) do { \
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_TRACE6 & gLog.tags)) { \
        log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 6, format, ##args);  \
    } \
} while(0)

#define TRACE7(format, args...) do { \
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_TRACE7 & gLog.tags)) { \
        log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 7, format, ##args);  \
    } \
} while(0)

#define TRACE8(format, args...) do { \
    if ((LOG_LEVEL_DEBUG <= gLog.level) && (TRACE_TAG_TRACE8 & gLog.tags)) { \
        log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 8, format, ##args);  \
    } \
} while(0)

#endif /* _lint */

#endif /* TRACE_H_DEFINED */

/* End of file */
