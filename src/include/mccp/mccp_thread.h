#ifndef __MCCP_THREAD_H__
#define __MCCP_THREAD_H__





/**
 *	@file	mccp_thread.h
 */





#if SIZEOF_PTHREAD_T == SIZEOF_INT64_T
#define MCCP_INVALID_THREAD	(pthread_t)~0LL
#elif SIZEOF_PTHREAD_T == SIZEOF_INT
#define MCCP_INVALID_THREAD	(pthread_t)~0
#endif /* SIZEOF_PTHREAD_T == SIZEOF_INT64_T ... */





typedef struct mccp_thread_record *	mccp_thread_t;


/**
 * @details The signature of thread finalization functions.
 *
 * @details In these functions <em> \b DO \b NOT </em> destroy/\b
 * free() anything, but just "make it sure" the threads are ready to
 * quit without any side effects that will affect futher operation
 * (e.g.: releasing the global lock, closing file descriptors, etc.).
 *
 * @details If \b is_canceled is \b true, it indicates that the thread
 * is canceled.
 */
typedef void
(*mccp_thread_finalize_proc_t)(const mccp_thread_t *selfptr,
                               bool is_canceled,
                               void *arg);


/**
 * @details The signature of thread free up functions.
 *
 * @details In thees functions, just free up any other dynamic
 * allocated objects which are needed to be freed up.
 *
 * @details And <em> \b DO \b NOT </em> free() the \b selfptr itself.
 */
typedef void
(*mccp_thread_freeup_proc_t)(const mccp_thread_t *selfptr,
                             void *arg);


/**
 * @details The signature of thread main workhorses.
 */
typedef mccp_result_t
(*mccp_thread_main_proc_t)(const mccp_thread_t *selfptr,
                           void *arg);





__BEGIN_DECLS


/**
 * Create a thread.
 *
 *	@param[in,out]	thdptr		A pointer to a thread.
 *	@param[in]	mainproc	A pointer to a main procedure.
 *	(\b NULL not allowed)
 *	@param[in]	finalproc	A pointer to a finalize procedure.
 *	(\b NULL allowed)
 *	@param[in]	freeproc	A pointer to a freeup procedure.
 *	(\b NULL allowed)
 *	@param[in]	name		A name of a thread to be created.
 *	@param[in]	arg		An auxiliary argumnet for the
 *	\b mainproc. (\b NULL allowed)
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b thdptr is \b NULL, it allocates a memory area
 *	and the allocated area is always free'd by calling \b
 *	mccp_thread_destroy(). Otherwise the pointer given is used
 *	as is.
 */
mccp_result_t
mccp_thread_create(mccp_thread_t *thdptr,
                   mccp_thread_main_proc_t mainproc,
                   mccp_thread_finalize_proc_t finalproc,
                   mccp_thread_freeup_proc_t freeproc,
                   const char *name,
                   void *arg);


/**
 * Start a thread.
 *
 *	@param[in]	thdptr		A pointer to a thread.
 *	@param[in]	autodelete	If \b true, the is deleted
 *	automatically when finished.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval MCCP_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ALREADY_EXISTS	Failed, already exists.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_thread_start(const mccp_thread_t *thdptr,
                  bool autodelete);


/**
 * Cancel a thread.
 *
 *	@param[in]	thdptr	A pointer to a thread.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval MCCP_RESULT_NOT_OWNER	Failed, not the owner.
 *	@retval MCCP_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_thread_cancel(const mccp_thread_t *thdptr);


/**
 * Wait for a thread finishes.
 *
 *	@param[in]  thdptr	A pointer to a thread.
 *	@param[in]  nsec	Wait time (nanosec).
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_NOT_OPERATIONAL	Failed, will be not
 *	operational in anytime.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval MCCP_RESULT_NOT_OWNER	Failed, not the owner.
 *	@retval MCCP_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval MCCP_RESULT_TIMEDOUT		Failed, timedout.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_thread_wait(const mccp_thread_t *thdptr,
                 mccp_chrono_t nsec);


/**
 * Destroy thread.
 *
 *     @param[in] thdptr         A pointer to a thread.
 *
 *    @details if thread is running, cancel it.
 */
void
mccp_thread_destroy(mccp_thread_t *thdptr);


/**
 * Let the \b mccp_thread_destroy() call free(3) for a thread
 * itself even the thread is not allocated by \b mccp_thread_create().
 *
 *	@param[in] thdptr	A pointer to a thread.
 *
 *	@retval MCCP_RESULT_OK               Succeeded.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval MCCP_RESULT_INVALID_ARGS     Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES     Failed.
 */
mccp_result_t
mccp_thread_free_when_destroy(mccp_thread_t *thdptr);


/**
 * Get a pthread id of a thread.
 *
 *	@param[in]	thdptr	A pointer to a thread.
 *	@param[out]	tidptr	A pointer to a thread id.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval MCCP_RESULT_NOT_STARTED	Failed, thread not started.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_thread_get_pthread_id(const mccp_thread_t *thdptr,
                           pthread_t *tidptr);


/**
 * Set a result code to a thread.
 *
 *	@param[in]	thdptr	A pointer to a thread.
 *	@param[in]	code	A result code to set.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_thread_set_result_code(const mccp_thread_t *thdptr,
                            mccp_result_t code);


/**
 * Get a result code of a thread.
 *
 *	@param[in]	thdptr	A pointer to a thread.
 *	@param[out]	codeptr	A pointer to a result code.
 *	@param[in]	nsec	Wait time (nanosec).
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval MCCP_RESULT_TIMEDOUT		Failed, timedout.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_thread_get_result_code(const mccp_thread_t *thdptr,
                            mccp_result_t *codeptr,
                            mccp_chrono_t nsec);


/**
 * Returns \b true if the thread is cancelled.
 *
 *	@param[in]	thdptr	A pointer to a thread.
 *	@param[out]	retptr	A pointer to a result.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval MCCP_RESULT_INVALID_ARGS     Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES     Failed.
 */
mccp_result_t
mccp_thread_is_canceled(const mccp_thread_t *thdptr,
                        bool *retptr);


/**
 * Returns \b true if the thread is valid (not destroyed).
 *
 *	@param[in]	thdptr	A pointer to a thread.
 *	@param[out]	retptr	A pointer to a result.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_thread_is_valid(const mccp_thread_t *thdptr,
                     bool *retptr);


/**
 * Preparation for fork(2).
 *
 *	@details We believe we don't need this, but in case.
 *
 *	@details This function is provided for the pthread_atfork(3)'s
 *	last argument. If we have to, functions for the first and
 *	second arguments for the pthread_atfork(3) would be provided
 *	as well, in later.
 */
void
mccp_thread_atfork_child(const mccp_thread_t *thdptr);


/**
 * Initialize the thread module.
 *
 *	@details No need to call this explicitly UNTIL you have to
 *	write any modules that could be required ordered
 *	initialization.
 */
void
mccp_thread_module_initialize(void);


/**
 * Finalize the thread module.
 *
 *	@details No need to call this explicitly UNTIL you have to
 *	write any modules that could be required ordered
 *	finalization.
 */
void
mccp_thread_module_finalize(void);


__END_DECLS





#endif /* ! __MCCP_THREAD_H__ */
