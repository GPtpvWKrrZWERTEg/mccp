#ifndef __MCCP_BBQ_H__
#define __MCCP_BBQ_H__





/**
 * @file mccp_bbq.h
 */





#include <mccp/mccp_cbuffer.h>





#ifndef __MCCP_BBQ_T_DEFINED__
typedef mccp_cbuffer_t mccp_bbq_t;
#endif /* ! __MCCP_BBQ_T_DEFINED__ */


/**
 * @deprecated Existing just for a backward compatibility.
 */
#define MCCP_BOUND_BLOCK_Q_DECL(name, type) mccp_bbq_t





/**
 * Create a bounded blocking queue.
 *
 *     @param[out] bbqptr         A pointer to a queue to be created.
 *     @param[in]  type           A type of a value of the queue.
 *     @param[in]  maxelem        A maximum # of the value the queue holds.
 *     @param[in]  proc           A value free up function (\b NULL allowed).
 *
 *     @retval MCCP_RESULT_OK               Succeeded.
 *     @retval MCCP_RESULT_NO_MEMORY        Failed, no memory.
 *     @retval MCCP_RESULT_ANY_FAILURES     Failed.
 */
#define mccp_bbq_create(bbqptr, type, length, proc)        \
  mccp_cbuffer_create((bbqptr), type, (length), (proc))


/**
 * Shutdown a bounded blocking queue.
 *
 *    @param[in]  bbqptr    A pointer to a queue to be shutdown.
 *    @param[in]  free_values  If \b true, all the values
 *    remaining in the queue are freed if the value free up
 *    function given by the calling of the mccp_cbuffer_create()
 *    is not \b NULL.
 */
#define mccp_bbq_shutdown(bbqptr, free_values)       \
  mccp_cbuffer_shutdown((bbqptr), (free_values))


/**
 * Destroy a bounded blocking queue.
 *
 *    @param[in]  bbqptr    A pointer to a queue to be destroyed.
 *    @param[in]  free_values  If \b true, all the values
 *    remaining in the queue are freed if the value free up
 *    function given by the calling of the mccp_cbuffer_create()
 *    is not \b NULL.
 *
 *    @details if \b bbq is operational, shutdown it.
 */
#define mccp_bbq_destroy(bbqptr, free_values)       \
  mccp_cbuffer_destroy((bbqptr), (free_values))


/**
 * Clear a bounded blocking queue.
 *
 *     @param[in]  bbqptr       A pointer to a queue
 *     @param[in]  free_values  If \b true, all the values
 *     remaining in the queue are freed if the value free up
 *     function given by the calling of the mccp_cbuffer_create()
 *     is not \b NULL.
 *
 *     @retval MCCP_RESULT_OK                Succeeded.
 *     @retval MCCP_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval MCCP_RESULT_ANY_FAILURES      Failed.
 */
#define mccp_bbq_clear(bbqptr, free_values)  \
  mccp_cbuffer_clear((bbqptr), (free_values))





/**
 * Put a value into a bounded blocking queue.
 *
 *     @param[in]  bbqptr     A pointer to a queue.
 *     @param[in]  valptr     A pointer to a value.
 *     @param[in]  type       A type of the value.
 *     @param[in]  nsec       A wait time (in nsec).
 *
 *     @retval MCCP_RESULT_OK                Succeeded.
 *     @retval MCCP_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval MCCP_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval MCCP_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval MCCP_RESULT_ANY_FAILURES      Failed.
 */
#define mccp_bbq_put(bbqptr, valptr, type, nsec)        \
  mccp_cbuffer_put((bbqptr), (valptr), type, (nsec))


/**
 * Get a value from a bounded blocking queue.
 *
 *     @param[in]  bbqptr     A pointer to a queue.
 *     @param[out] valptr     A pointer to a value.
 *     @param[in]  type       A type of the value.
 *     @param[in]  nsec       A wait time (in nsec).
 *
 *     @retval MCCP_RESULT_OK                Succeeded.
 *     @retval MCCP_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval MCCP_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval MCCP_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval MCCP_RESULT_ANY_FAILURES      Failed.
 */
#define mccp_bbq_get(bbqptr, valptr, type, nsec)   \
  mccp_cbuffer_get((bbqptr), (valptr), type, (nsec))


/**
 * Peek the first value from a bounded blocking queue.
 *
 *     @param[in]  bbqptr     A pointer to a queue.
 *     @param[out] valptr     A pointer to a value.
 *     @param[in]  type       A type of the value.
 *     @param[in]  nsec       A wait time (in nsec).
 *
 *     @retval MCCP_RESULT_OK                Succeeded.
 *     @retval MCCP_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval MCCP_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval MCCP_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval MCCP_RESULT_ANY_FAILURES      Failed.
 */
#define mccp_bbq_peek(bbqptr, valptr, type, nsec)            \
  mccp_cbuffer_peek((bbqptr), (valptr), type, (nsec))





/**
 * Get a # of values in a bounded blocking queue.
 *	@param[in]   bbqptr    A pointer to a queue.
 *
 *	@retval	>=0	A # of values in the queue.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval MCCP_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
#define mccp_bbq_size(bbqptr)                \
  mccp_cbuffer_size((bbqptr))


/**
 * Get the remaining capacity of bounded blocking queue.
 *	@param[in]   bbqptr    A pointer to a queue.
 *
 *	@retval	>=0	The remaining capacity of the queue.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval MCCP_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
#define mccp_bbq_remaining_capacity(bbqptr)        \
  mccp_cbuffer_remaining_capacity((bbqptr))


/**
 * Get the maximum capacity of bounded blocking queue.
 *	@param[in]   bbqptr    A pointer to a queue.
 *
 *	@retval	>=0	The maximum capacity of the queue.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval MCCP_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
#define mccp_bbq_max_capacity(bbqptr)        \
  mccp_cbuffer_max_capacity((bbqptr))





/**
 * Returns \b true if the bounded blocking queue is full.
 *
 *    @param[in]   bbqptr   A pointer to a queue.
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval MCCP_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
#define mccp_bbq_is_full(bbqptr, retptr)     \
  mccp_cbuffer_is_full((bbqptr), (retptr))


/**
 * Returns \b true if the bounded blocking queue is empty.
 *
 *    @param[in]   bbqptr   A pointer to a queue.
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval MCCP_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
#define mccp_bbq_is_empty(bbqptr, retptr)    \
  mccp_cbuffer_is_empty((bbqptr), (retptr))


/**
 * Returns \b true if the bounded blocking queue is operational.
 *
 *    @param[in]   cbptr    A pointer to a queue.
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
#define mccp_bbq_is_operational(bbqptr, retptr)    \
  mccp_cbuffer_is_operational((bbqptr), (retptr))


/**
 * Cleanup an internal state of a circular buffer after thread
 * cancellation.
 *	@param[in]	cbptr	A pointer to a circular buffer
 */
#define mccp_bbq_cancel_janitor(cbptr)       \
    mccp_cbuffer_cancel_janitor((cbptr))






#endif  /* ! __MCCP_BBQ_H__ */
