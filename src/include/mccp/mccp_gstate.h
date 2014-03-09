#ifndef __MCCP_GSTATE_H__
#define __MCCP_GSTATE_H__





/**
 * @file	mccp_gstate.h
 */





typedef enum {
  MCCP_GLOBAL_STATE_UNKNOWN = 0,
  MCCP_GLOBAL_STATE_INITIALIZING,
  MCCP_GLOBAL_STATE_INITIALIZED,
  MCCP_GLOBAL_STATE_STARTING,
  MCCP_GLOBAL_STATE_STARTED,
  MCCP_GLOBAL_STATE_REQUEST_SHUTDOWN,
  MCCP_GLOBAL_STATE_ACCEPT_SHUTDOWN,
  MCCP_GLOBAL_STATE_SHUTTINGDOWN,
  MCCP_GLOBAL_STATE_SHUTDOWN,
  MCCP_GLOBAL_STATE_FINALIZING,
  MCCP_GLOBAL_STATE_FINALIZED
} mccp_global_state_t;
#define IS_VALID_MCCP_GLOBAL_STATE(s)                                \
  ((((int)(s) > (int)MCCP_GLOBAL_STATE_UNKNOWN) &&                   \
    ((int)(s) <= (int)MCCP_GLOBAL_STATE_FINALIZED)) ? true : false)
#define IS_MCCP_GLOBAL_STATE_SHUTDOWN(s)                                     \
  (((int)(s) >= (int)MCCP_GLOBAL_STATE_ACCEPT_SHUTDOWN) ? true : false)


typedef enum {
  SHUTDOWN_UNKNOWN = 0,
  SHUTDOWN_RIGHT_NOW,
  SHUTDOWN_GRACEFULLY
} shutdown_grace_level_t;
#define IS_VALID_SHUTDOWN(s) \
  (((int)(s) > (int)SHUTDOWN_UNKNOWN) &&                \
   ((int)(s) <= (int)SHUTDOWN_GRACEFULLY)) ? true : false





/**
 * Set the global state.
 *
 *	@param[in]	s	A global state.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_INVALID_STATE_TRANSITION
 *	Failed, invalid global state.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t	mccp_global_state_set(mccp_global_state_t s);


/**
 * Get the global state.
 *
 *	@param[out]	sptr	A pointer to a global state.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t	mccp_global_state_get(mccp_global_state_t *sptr);


/**
 * Wait for change of the global state.
 *
 *	@param[in]	s_wait_for	A desired global state.
 *	@param[out]	cur_sptr	A pointer to a global state after
 *	the change.
 *	@param[out]	cur_gptr	A pointer to a shutdown grace level
 *	after the change.
 *	@param[in]	nsec		A timeout (in nsec).
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_NOT_OPERATIONAL	Failed, will be not
 *	operational in anytime.
 *	@retval MCCP_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval MCCP_RESULT_TIMEDOUT		Failed, timedout.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the current global state is more further than the
 *	\b s_wait_for, it always returns \b MCCP_RESULT_OK.
 *	@details If the global state is advanced while waiting more
 *	further than the \b s_wait_for, it also returns
 *	MCCP_RESULT_OK, EXCLUDE the case if the current global
 *	state changes to shutdown state. In this case, it returns
 *	the \b MCCP_RESULT_NOT_OPERATIONAL and the caller must
 *	check the \b cur_gptr to prepare/invoke for appropriate
 *	shutdown sequence.
 */
mccp_result_t
mccp_global_state_wait_for(mccp_global_state_t s_wait_for,
                           mccp_global_state_t *cur_sptr,
                           shutdown_grace_level_t *cur_gptr,
                           mccp_chrono_t nsec);

/**
 * Wait for change of the global state to shutdown state.
 *
 *	@param[out]	cur_gptr	A pointer to a shutdown grace level
 *	after the change.
 *	@param[in]	nsec		A timeout (in nsec).
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_NOT_OPERATIONAL	Failed, will be not
 *	operational in anytime.
 *	@retval MCCP_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval MCCP_RESULT_TIMEDOUT		Failed, timedout.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 *
 *	@details This is equivalent to calling;
 *	mccp_global_state_wait_for(\b MCCP_GLOBAL_STATE_REQUEST_SHUTDOWN,
 *	\b NULL, \b cur_gptr, \b nsec).
 *
 *	@details After calling this procedure and it returns \b
 *	MCCP_RESULT_OK, (one of) the caller must call
 *	mccp_global_state_set(\b MCCP_GLOBAL_STATE_ACCEPT_SHUTDOWN) or caller of
 *	the mccp_global_state_request_shutdown() blocks forever.
 */
mccp_result_t
mccp_global_state_wait_for_shutdown_request(shutdown_grace_level_t *cur_gptr,
    mccp_chrono_t nsec);


/**
 * Request shudtwon.
 *
 *	@param[in]	l	A shutdown grace level.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_INVALID_STATE_TRANSITION
 *	Failed, invalid global state.
 *	@retval MCCP_RESULT_NOT_OPERATIONAL	Failed, will be not
 *	operational in anytime.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 *
 *	@details Calling this procedure blocks the caller until the
 *	global state shanges to the \b MCCP_GLOBAL_STATE_ACCEPT_SHUTDOWN.
 */
mccp_result_t
mccp_global_state_request_shutdown(shutdown_grace_level_t l);


/**
 * Clean an internal state the global state tracker up after the
 * cancellation of the caller threads.
 */
void	mccp_global_state_cancel_janitor(void);


/**
 * Not documented on purposely.
 *
 *	@details Use this only in the unit tests.
 */
void	mccp_global_state_reset(void);





#endif /* ! __MCCP_GSTATE_H__ */
