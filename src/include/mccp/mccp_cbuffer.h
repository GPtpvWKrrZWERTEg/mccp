#ifndef __MCCP_CBUFFER_H__
#define __MCCP_CBUFFER_H__





/**
 * @file mccp_cbuffer.h
 */





#ifndef __MCCP_CBUFFER_T_DEFINED__
typedef struct mccp_cbuffer_record *	mccp_cbuffer_t;
#endif /* ! __MCCP_CBUFFER_T_DEFINED__ */


/**
 * @details The signature of value free up functions called when
 * destroying a circular buffer.
 */
typedef void	(*mccp_cbuffer_value_freeup_proc_t)(void **valptr);





mccp_result_t
mccp_cbuffer_create_with_size(mccp_cbuffer_t *cbptr,
                              size_t elemsize,
                              int64_t maxelems,
                              mccp_cbuffer_value_freeup_proc_t proc);
/**
 * Create a circular buffer.
 *
 *     @param[in,out]	cbptr	A pointer to a circular buffer to be created.
 *     @param[in]	type	Type of the element.
 *     @param[in]	maxelems	# of maximum elements.
 *     @param[in]	proc	A value free up function (\b NULL allowed).
 *
 *     @retval MCCP_RESULT_OK               Succeeded.
 *     @retval MCCP_RESULT_NO_MEMORY        Failed, no memory.
 *     @retval MCCP_RESULT_ANY_FAILURES     Failed.
 */
#define mccp_cbuffer_create(cbptr, type, maxelems, proc)              \
  mccp_cbuffer_create_with_size((cbptr), sizeof(type), (maxelems), (proc))


/**
 * Shutdown a circular buffer.
 *
 *    @param[in]	cbptr	A pointer to a circular buffer to be shutdown.
 *    @param[in]	free_values	If \b true, all the values remaining
 *    in the buffer are freed if the value free up function given by
 *    the calling of the mccp_cbuffer_create() is not \b NULL.
 */
void
mccp_cbuffer_shutdown(mccp_cbuffer_t *cbptr,
                      bool free_values);


/**
 * Destroy a circular buffer.
 *
 *    @param[in]  cbptr    A pointer to a circular buffer to be destroyed.
 *    @param[in]  free_values  If \b true, all the values
 *    remaining in the buffer are freed if the value free up
 *    function given by the calling of the mccp_cbuffer_create()
 *    is not \b NULL.
 *
 *    @details if \b cbuf is operational, shutdown it.
 */
void
mccp_cbuffer_destroy(mccp_cbuffer_t *cbptr,
                     bool free_values);


/**
 * Clear a circular buffer.
 *
 *     @param[in]  cbptr        A pointer to a circular buffer
 *     @param[in]  free_values  If \b true, all the values
 *     remaining in the buffer are freed if the value free up
 *     function given by the calling of the mccp_cbuffer_create()
 *     is not \b NULL.
 *
 *     @retval MCCP_RESULT_OK                Succeeded.
 *     @retval MCCP_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval MCCP_RESULT_ANY_FAILURES      Failed.
 */
mccp_result_t
mccp_cbuffer_clear(mccp_cbuffer_t *cbptr,
                   bool free_values);





mccp_result_t
mccp_cbuffer_put_with_size(mccp_cbuffer_t *cbptr,
                           void **valptr,
                           size_t valsz,
                           mccp_chrono_t nsec);
/**
 * Put an element at the tail of a circular buffer.
 *
 *     @param[in]  cbptr      A pointer to a circular buffer
 *     @param[in]  valptr     A pointer to an element.
 *     @param[in]  type       Type of a element.
 *     @param[in]  nsec       Wait time (nanosec).
 *
 *     @retval MCCP_RESULT_OK                Succeeded.
 *     @retval MCCP_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval MCCP_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval MCCP_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval MCCP_RESULT_ANY_FAILURES      Failed.
 */
#define mccp_cbuffer_put(cbptr, valptr, type, nsec)                  \
  mccp_cbuffer_put_with_size((cbptr), (void **)(valptr), sizeof(type), \
                                (nsec))


mccp_result_t
mccp_cbuffer_get_with_size(mccp_cbuffer_t *cbptr,
                           void **valptr,
                           size_t valsz,
                           mccp_chrono_t nsec);
/**
 * Get the head element in a circular buffer.
 *
 *     @param[in]  cbptr      A pointer to a circular buffer
 *     @param[out] valptr     A pointer to a element.
 *     @param[in]  type       Type of a element.
 *     @param[in]  nsec       Wait time (nanosec).
 *
 *     @retval MCCP_RESULT_OK                Succeeded.
 *     @retval MCCP_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval MCCP_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval MCCP_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval MCCP_RESULT_ANY_FAILURES      Failed.
 */
#define mccp_cbuffer_get(cbptr, valptr, type, nsec)                  \
  mccp_cbuffer_get_with_size((cbptr), (void **)(valptr), sizeof(type), \
                                (nsec))


mccp_result_t
mccp_cbuffer_peek_with_size(mccp_cbuffer_t *cbptr,
                            void **valptr,
                            size_t valsz,
                            mccp_chrono_t nsec);
/**
 * Peek the head element of a circular buffer.
 *
 *     @param[in]  cbptr      A pointer to a circular buffer
 *     @param[out] valptr     A pointer to a element.
 *     @param[in]  type       Type of a element.
 *     @param[in]  nsec       Wait time (nanosec).
 *
 *     @retval MCCP_RESULT_OK                Succeeded.
 *     @retval MCCP_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval MCCP_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval MCCP_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval MCCP_RESULT_ANY_FAILURES      Failed.
 */
#define mccp_cbuffer_peek(cbptr, valptr, type, nsec)                 \
  mccp_cbuffer_peek_with_size((cbptr), (void **)(valptr), sizeof(type), \
                                 (nsec))





/**
 * Get a # of elements in a circular buffer.
 *	@param[in]   cbptr    A pointer to a circular buffer
 *
 *	@retval	>=0	A # of elements in the circular buffer.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval MCCP_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_cbuffer_size(mccp_cbuffer_t *cbptr);


/**
 * Get the remaining capacity of a circular buffer.
 *	@param[in]   cbptr    A pointer to a circular buffer
 *
 *	@retval	>=0	The remaining capacity of the circular buffer.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval MCCP_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_cbuffer_remaining_capacity(mccp_cbuffer_t *cbptr);


/**
 * Get the maximum capacity of a circular buffer.
 *	@param[in]   cbptr    A pointer to a circular buffer
 *
 *	@retval	>=0	The maximum capacity of the circular buffer.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval MCCP_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_cbuffer_max_capacity(mccp_cbuffer_t *cbptr);





/**
 * Returns \b true if the circular buffer is full.
 *
 *    @param[in]   cbptr    A pointer to a circular buffer.
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval MCCP_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_cbuffer_is_full(mccp_cbuffer_t *cbptr, bool *retptr);


/**
 * Returns \b true if the circular buffer is empty.
 *
 *    @param[in]   cbptr    A pointer to a circular buffer
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval MCCP_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_cbuffer_is_empty(mccp_cbuffer_t *cbptr, bool *retptr);


/**
 * Returns \b true if the circular buffer is operational.
 *
 *    @param[in]   cbptr    A pointer to a circular buffer
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_cbuffer_is_operational(mccp_cbuffer_t *cbptr, bool *retptr);


/**
 * Cleanup an internal state of a circular buffer after thread
 * cancellation.
 *	@param[in]	cbptr	A pointer to a circular buffer
 */
void
mccp_cbuffer_cancel_janitor(mccp_cbuffer_t *cbptr);





#endif  /* ! __MCCP_CBUFFER_H__ */
