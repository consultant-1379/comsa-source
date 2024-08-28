/**
 * @file trace.c
 *
 * Modify: xdonngu 2013-12-12 run cppcheck 1.62 and fix errors and warnings
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
//include <sys/syscall.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>

#include "trace.h"

static const char *prefix_name[] = {
    "EM", "AL", "CR", "ER", 
    "WA", "NO", "IN", "TR", 
    "T1", "T2", "T3", "T4", 
    "T5", "T6", "T7", "T8", 
    ">>", "<<" };

static pthread_mutex_t log_write_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Local function declarations
 */
static void  _log_printf (const char *file, int line, int priority, int category, const char *format, va_list ap);
int64_t get_log_size( void );
void rotate_log( void );

/*
 * Global variables
 */
struct log_state_t gLog =
{
    LOG_LEVEL_INFO,             /* Everything but DEBUG logs */
    -1,                         /* All tags */
    LOG_MODE_STDERR
};

/*
 * Local variables
 */
static char* log_ident = NULL;

static char log_fname[255];
static int log_file_size = 10485760;
static FILE *log_file_fp = 0;

/**
 * Checks if the given libray name is loaded in the process memory
 *
 * @param probeName     probe library name
 * @return 0 if library is found and something else when it's not found
 */
int isTraceCCProbeLoaded(const char *probeName)
{
    char command[60];
    sprintf(command, "cat /proc/%d/maps | grep %s", getpid(), probeName);
    return system(command);
}


/**
 * Initialize the logger.
 *
 * @param ident     service name
 */
void
log_init (const char *ident)
{
    assert (ident != NULL);
    int str_size = strlen(ident)+1;
    str_size = (str_size > 6 ? str_size : 6);
    log_ident = (char *)malloc(str_size * sizeof(char));
    strcpy (log_ident, ident);
}

/**
 * Send output to log file.
 *
 * @param logfile   path to the log file
 */
void
log_to_file(const char* logfile)
{
    if (gLog.mode & LOG_MODE_FILE) {
	if (log_file_fp != 0)
	{
	        fclose(log_file_fp);
	}
        gLog.mode &= ~LOG_MODE_FILE;
    }

    if (logfile) {
        gLog.mode |= LOG_MODE_FILE;

        int str_size = strlen(logfile);
        strncpy( log_fname, logfile, str_size );

        log_file_fp = fopen ( log_fname, "a+");
        if (log_file_fp == 0) {
            fprintf (stderr, "Can't open logfile '%s': %s.\n", log_fname, strerror (errno));
        }

        if( getenv( "TRACE_LOG_FILE_SIZE" ) ) {
            log_file_size = atoi( getenv( "TRACE_LOG_FILE_SIZE" ) );
        }
    }
}

/** 
 * Checks the file size of the log.
 * 
 * @return The file size of log_fname.
 */
int64_t get_log_size( void )
{
    struct stat file_status;
    
    if( stat( log_fname, &file_status ) != 0 ) {
        fprintf (stderr, "Couldn't stat logfile '%s': %s.\n", log_fname, strerror (errno));
    }

    return (int64_t)file_status.st_size;
}

/** 
 * Rotate's the log file.
 */
void rotate_log( void )
{
    char old_log_fname[255];

    fclose(log_file_fp);

    strncpy( old_log_fname, log_fname, strlen(log_fname) );
    old_log_fname[strlen(log_fname)] = '\0';
    strncat( old_log_fname, ".1", 2 );
    rename( log_fname, old_log_fname );

    log_file_fp = fopen ( log_fname, "a+");
    if (log_file_fp == 0) {
        fprintf (stderr, "Can't open logfile '%s': %s.\n", log_fname, strerror (errno));
    }
}

/**
 * Internal function called from TRACE macros.
 *
 * @param file
 * @param line
 * @param priority
 * @param format
 */
void
log_printf(const char *file, int line, int priority, int category, const char *format, ...)
{
    va_list ap;
    va_start (ap, format);
    _log_printf (file, line, priority, category, format, ap);
    va_end(ap);
}

void
enter_leave_log_printf(const char *file, int line, int priority, int category, const char *format, ...)
{
    if(trace_flag == 1)
    {
        va_list ap;
        va_start (ap, format);
        _log_printf (file, line, priority, category, format, ap);
        va_end(ap);
    }
}

/**
 *
 *
 * @param file
 * @param line
 * @param priority
 * @param format
 * @param ap
 */
static void
_log_printf (const char *file, int line, int level, int category, const char *format, va_list ap)
{
    char newstring[4096];
    char log_string[4096];
    char char_time[512];
    struct timeval tv;
    struct tm time_info;
    int num_char = 0;
    int i = 0;

    /* lock mutex */
    pthread_mutex_lock( &log_write_mutex );

    /* Rotate the log if the file size is over 10M */
    if( gLog.mode & LOG_MODE_FILE && log_file_fp != 0 && log_file_size < get_log_size() ) {
        /*fprintf(stderr, "Rotating log\n" );*/
        rotate_log();
    }

    if (((gLog.mode & LOG_MODE_FILE) || (gLog.mode & LOG_MODE_STDERR) || (gLog.mode & LOG_MODE_STDOUT)) &&
        (gLog.mode & LOG_MODE_TIMESTAMP)) {
        gettimeofday (&tv, NULL);
        time_t t = time(0);
        struct tm *now = localtime(&t);

        char usec[10];
        sprintf(usec, "%06li", (long int)tv.tv_usec);
        char usec_str[4];
        memcpy(usec_str, usec, 3);
        usec_str[3]= '\0';

        unsigned int absTzOffset = (unsigned int)((now->tm_gmtoff>0 ? now->tm_gmtoff : -now->tm_gmtoff)/60);
        strftime (char_time, sizeof (char_time), "%Y-%m-%dT%H:%M:%S",
                  localtime_r (&tv.tv_sec, &time_info));
        num_char = sprintf (newstring, "%s.%s%c%02d:%02d ", char_time,usec_str,
        		(unsigned char)((now->tm_gmtoff >= 0)?'+':'-'),
        		(unsigned char)(absTzOffset / 60),// offset in hours
        		(unsigned char)(absTzOffset % 60));// offset in minutes
    }

    /* Remove the path to the file. */
    char *fname = strrchr( file, '/' );
    if( !fname ) {
        fname = (char*)file;
    } else {
        fname++;
    }

    if ( (level == LOG_LEVEL_DEBUG) || (gLog.mode & LOG_MODE_FILELINE) ) {
        i = sprintf( &newstring[ num_char ], "[%-5s] %20s:%-4d ", 
                log_ident, fname, line );
        num_char += i;
        sprintf(&newstring[ num_char ], "%s : %s",
                prefix_name[ level + category ], format);
    } else {
        i = sprintf( &newstring[num_char], "[%-5s] ", 
                log_ident );
        num_char += i;
        sprintf( &newstring[num_char], "%s : %s", 
                prefix_name[ level + category ], format);
    }
    vsprintf (log_string, newstring, ap);

    /*
     * Output the log data
     */
    if (gLog.mode & LOG_MODE_FILE && log_file_fp != 0) {
        fprintf (log_file_fp, "%s\n", log_string);
        fflush (log_file_fp);
    }
    if (gLog.mode & LOG_MODE_STDERR) {
        fprintf (stderr, "%s\n", log_string);
        fflush (stderr);
    }
    if (gLog.mode & LOG_MODE_STDOUT) {
        printf("%s\n", log_string);
    }

    if (gLog.mode & LOG_MODE_SYSLOG || ( level != LOG_LEVEL_DEBUG && level != LOG_LEVEL_INFO ) ) {
        syslog (LOG_USER*8 + level, "%s", &log_string[num_char]);
    }

    /* unlock mutex */
    pthread_mutex_unlock( &log_write_mutex );
}

/**
 * Set new trace state from string.
 * Examples:
 *
 * log_control("-#l");          // set max trace level (DEBUG)
 * log_control("-#f,s");        // trace output to file and syslog
 * log_control("-#l:6,t:2");    // set trace level = 6, tags = 2
 *
 * @param control   test string
 */
void
log_control_from_string(const char* string, struct log_state_t* old)
{
    if (old)
        *old = gLog;

    const char* scan;
    if (string && *string == '-') {
        if (*++string == '#')
            string++;
    }

    /* Update state */
    for (scan = string; scan != NULL; scan = strchr(scan, ',')) {
        if (*scan == ',')
            scan++;

        switch (*scan++) {
        case 'l':
            gLog.level = LOG_LEVEL_DEBUG;
            if (*scan == ':') {
                gLog.level = atoi(++scan);
            }
            break;
        case 't':
            gLog.tags = -1;
            if (*scan == ':') {
                gLog.tags = atoi(++scan);
            }
            break;
        case 's':
            gLog.mode |= LOG_MODE_SYSLOG;
            break;
        case 'o':
            gLog.mode |= LOG_MODE_STDERR;
            break;
        case 'p':
            gLog.mode |= LOG_MODE_STDOUT;
            break;
        case 'i':
            gLog.mode |= LOG_MODE_FILELINE;
            break;
        case 'f':
            log_to_file( getenv( "TRACE_LOG_FILE" ) );
            break;
        }
    }
}

/**
 * Set new trace state.
 *
 * @param state     new trace state
 * @param state     output, old trace state
 */
void
log_control(const struct log_state_t* state, struct log_state_t* old)
{
    if (old)
        *old = gLog;
    gLog = *state;
}


#ifdef TRY_MAIN
int
foo(int arg)
{
    ENTER();

    struct log_state_t old_state;
    log_control_from_string("l", &old_state); /* Set max trace level (DEBUG) */

    int a = arg +7;
    TRACE2("a=%d", a);      /*  */
    if (a > 20)
        LOG_PRINTF(LOG_LEVEL_ERROR, "a too big (a=%d)", a);

    log_control(&old_state, 0); /* Return to previous trace level */

    LEAVE();
    return a;
}

int
main(int argc, const char* argv[])
{
    if (argc > 1)
        log_control_from_string(argv[1], 0);

    gLog.mode |= LOG_MODE_FILELINE;
    gLog.mode |= LOG_MODE_TIMESTAMP;

    ENTER();
    int a = 7;
    int b = 20;

    log_init("TEST");           /* Set service name */

    LOG_PRINTF(LOG_LEVEL_INFO, "Test of error trace");
    TRACE1("Test av TRACE1, arg=%d", a);
    foo(b);

    LEAVE();
    return 0;
}

#endif


/* End of file */
