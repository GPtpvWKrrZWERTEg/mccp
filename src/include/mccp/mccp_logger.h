#ifndef __MCCP_LOGGER_H__
#define __MCCP_LOGGER_H__





/**
 * @file	mccp_logger.h
 */





typedef enum {
  MCCP_LOG_LEVEL_UNKNOWN = 0,
  MCCP_LOG_LEVEL_DEBUG,
  MCCP_LOG_LEVEL_TRACE,
  MCCP_LOG_LEVEL_INFO,
  MCCP_LOG_LEVEL_NOTICE,
  MCCP_LOG_LEVEL_WARNING,
  MCCP_LOG_LEVEL_ERROR,
  MCCP_LOG_LEVEL_FATAL
} mccp_log_level_t;


typedef enum {
  MCCP_LOG_EMIT_TO_UNKNOWN = 0,
  MCCP_LOG_EMIT_TO_FILE,
  MCCP_LOG_EMIT_TO_SYSLOG
} mccp_log_destination_t;





/**
 * Initialize the logger.
 *
 *	@param[in]	dst	Where to log;
 *	\b MCCP_LOG_EMIT_TO_UNKNOWN: stderr,
 *	\b MCCP_LOG_EMIT_TO_FILE: Any regular file,
 *	\b MCCP_LOG_EMIT_TO_SYSLOG: syslog
 *	@param[in]	arg	For \b MCCP_LOG_EMIT_TO_FILE: a file name,
 *	for \b MCCP_LOG_EMIT_TO_SYSLOG: An identifier for syslog.
 *	@param[in]	multi_process	If the \b dst is
 *	\b MCCP_LOG_EMIT_TO_FILE, use \b true if the application shares
 *	the log file between child processes.
 *	@param[in]	emit_date	Use \b true if date is needed in each
 *	line header.
 *	@param[in]	debug_level	A debug level.
 *	@param[in]	trace_flags	A trace flags.
 *
 *	@retval	MCCP_RESULT_OK			Succeeded.
 *	@retval	MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_log_initialize(mccp_log_destination_t dst,
                    const char *arg,
                    bool multi_process,
                    bool emit_date,
                    uint64_t debug_level,
                    uint64_t trace_flags);


/**
 * Re-initialize the logger.
 *
 *	@details Calling this function implies 1) close opened log
 *	file. 2) re-open the log file, convenient for the log rotation.
 *
 *	@retval	MCCP_RESULT_OK			Succeeded.
 *	@retval	MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t	mccp_log_reinitialize(void);


/**
 * Finalize the logger.
 */
void	mccp_log_finalize(void);


/**
 * Set the debug level.
 *
 *	@param[in]	lvl	A debug level.
 */
void	mccp_log_set_debug_level(uint64_t lvl);


/**
 * Get the debug level.
 *
 *	@returns	The debug level.
 */
uint64_t	mccp_log_get_debug_level(void);


/**
 * Set the trace flags.
 *
 *	@param[in]	flags	An arbitrary interger value.
 */
void	mccp_log_set_trace_flags(uint64_t flags);


/**
 * Unset the trace flags.
 *
 *	@param[in]	flags	An arbitrary interger value.
 */
void	mccp_log_unset_trace_flags(uint64_t flags);


/**
 * Get the trace flags.
 *
 *	@returns The trace flags.
 */
uint64_t	mccp_log_get_trace_flags(void);


/**
 * Check where the log is emitted to.
 *
 *	@returns The log destination.
 */
mccp_log_destination_t
mccp_log_get_destination(void);


/**
 * The main logging workhorse: not intended for direct use.
 */
void	mccp_log_emit(mccp_log_level_t log_level,
                    uint64_t debug_level,
                    const char *file,
                    int line,
                    const char *func,
                    const char *fmt, ...)
__attr_format_printf__(6, 7);





#ifdef __GNUC__
#define __PROC__	__PRETTY_FUNCTION__
#else
#define	__PROC__	__func__
#endif /* __GNUC__ */


/**
 * Emit a debug message to the log.
 *
 *	@param[in]	level	A debug level (int).
 */
#define mccp_msg_debug(level, ...)                              \
  mccp_log_emit(MCCP_LOG_LEVEL_DEBUG, (uint64_t)(level),        \
                __FILE__, __LINE__, __PROC__, __VA_ARGS__)


/**
 * Emit a trace message to the log.
 *
 *	@param[in]	flags	An arbitrary interger value.
 *	@param[in]	detail	A boolean for detailed trace logging.
 */
#define mccp_msg_trace(flags, ...)                              \
  mccp_log_emit(MCCP_LOG_LEVEL_TRACE, (uint64_t)(flags),        \
                __FILE__, __LINE__, __PROC__, __VA_ARGS__)


/**
 * Emit an informative message to the log.
 */
#define mccp_msg_info(...)                                    \
  mccp_log_emit(MCCP_LOG_LEVEL_INFO, 0LL, __FILE__, __LINE__, \
                __PROC__, __VA_ARGS__)


/**
 * Emit a notice message to the log.
 */
#define mccp_msg_notice(...)                                    \
  mccp_log_emit(MCCP_LOG_LEVEL_NOTICE, 0LL, __FILE__, __LINE__, \
                __PROC__, __VA_ARGS__)


/**
 * Emit a warning message to the log.
 */
#define mccp_msg_warning(...)                                    \
  mccp_log_emit(MCCP_LOG_LEVEL_WARNING, 0LL, __FILE__, __LINE__, \
                __PROC__, __VA_ARGS__)


/**
 * Emit an error message to the log.
 */
#define mccp_msg_error(...)                                    \
  mccp_log_emit(MCCP_LOG_LEVEL_ERROR, 0LL, __FILE__, __LINE__, \
                __PROC__, __VA_ARGS__)


/**
 * Emit a fatal message to the log.
 */
#define mccp_msg_fatal(...)                                    \
  mccp_log_emit(MCCP_LOG_LEVEL_FATAL, 0LL, __FILE__, __LINE__, \
                __PROC__, __VA_ARGS__)


/**
 * Emit an arbitarary message to the log.
 */
#define mccp_msg(...)                                            \
  mccp_log_emit(MCCP_LOG_LEVEL_UNKNOWN, 0LL, __FILE__, __LINE__, \
                __PROC__, __VA_ARGS__)


/**
 * The minimum level debug emitter.
 */
#define mccp_dprint(...)                        \
  mccp_msg_debug(1LL, __VA_ARGS__)


/**
 * Emit a readable error message for the errornous result.
 *
 *	@param[in]	s	A result code (mccp_result_t)
 *	@param[in]	msg	A description.
 */
#define mccp_perror(s, msg)                                             \
  do {                                                                  \
    if ((s) == MCCP_RESULT_POSIX_API_ERROR) {                           \
      if (IS_VALID_STRING(msg) == true) {                               \
        mccp_msg_error("MCCP_RESULT_POSIX_API_ERROR: %s: %s.\n",        \
                       (msg), strerror(errno));                         \
      } else {                                                          \
        mccp_msg_error("MCCP_RESULT_POSIX_API_ERROR: %s.\n",            \
                       strerror(errno));                                \
      }                                                                 \
    } else {                                                            \
      if (IS_VALID_STRING(msg) == true) {                               \
        mccp_msg_error("%s: %s.\n", (msg), mccp_error_get_string((s))); \
      } else {                                                          \
        mccp_msg_error("%s.\n", mccp_error_get_string((s)));            \
      }                                                                 \
    }                                                                   \
  } while (0)


/**
 * Emit an error message and exit.
 *
 *	@param[in]	ecode	An exit code (int)
 */
#define mccp_exit_error(ecode, ...)            \
  do {                                       \
    mccp_msg_error(__VA_ARGS__);               \
    exit(ecode);                               \
  } while (0)


/**
 * Emit a fatal message and abort.
 */
#define mccp_exit_fatal(...)                        \
  do {                                              \
    mccp_msg_fatal(__VA_ARGS__);                    \
    abort();                                        \
  } while (0)





#endif /* ! __MCCP_LOGGER_H__ */
