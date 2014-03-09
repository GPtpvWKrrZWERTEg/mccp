#ifndef __MCCP_PIPELINE_STAGE_H__
#define __MCCP_PIPELINE_STAGE_H__





/**
 *	@file	mccp_pipeline_stage.h
 */





#include <mccp/mccp_pipeline_stage_funcs.h>





#ifndef PIPELINE_STAGE_T_DECLARED
typedef struct mccp_pipeline_stage_record *	mccp_pipeline_stage_t;
#define PIPELINE_STAGE_T_DECLARED
#endif /* PIPELINE_STAGE_T_DECLARED */





/**
 * Create a pipeline stage.
 *
 *	@param[in,out] sptr A pointer to a stage.
 *	@param[in] alloc_size A memory allocation size for this object
 *	(in bytes)
 *	@param[in] name A name of the stage.
 *	@param[in] n_workers A # of the workers.
 *	@param[in] event_size A size of an event. (in bytes.)
 *	@param[in] max_batch_size A # of the maximum processed events at
 *	a time in each worker.
 *	@param[in] sched_proc A schedule function.
 *	@param[in] maintenance_proc A maintenance function.
 *	@param[in] setup_proc A setup function.
 *	@param[in] fetch_proc A fetch function.
 *	@param[in] main_proc A main function.
 *	@param[in] throw_proc A throw function.
 *	@param[in] shutdown_proc A shutdown function.
 *	@param[in] final_proc A finalization function.
 *	@param[in] freeup_proc A free-up function.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval MCCP_RESULT_ALREADY_EXISTS	Failed, already exists.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b sptr is \b NULL, it allocates a memory area
 *	and the allocated area is always free'd by calling \b
 *	mccp_pipeline_stage_destroy(). In this case if the \b
 *	alloc_size is greater than the original object size, \b
 *	alloc_size bytes momory area is allocated. Otherwise if the \b
 *	sptr is not NULL the pointer given is used as is.
 */
mccp_result_t
mccp_pipeline_stage_create(mccp_pipeline_stage_t *sptr,
                           size_t alloc_size,
                           const char *name,
                           size_t n_workers,
                           size_t event_size,
                           size_t max_batch_size,
                           mccp_pipeline_stage_sched_proc_t sched_proc,
                           mccp_pipeline_stage_maintenance_proc_t
                           maintenance_proc,
                           mccp_pipeline_stage_setup_proc_t setup_proc,
                           mccp_pipeline_stage_fetch_proc_t fetch_proc,
                           mccp_pipeline_stage_main_proc_t main_proc,
                           mccp_pipeline_stage_throw_proc_t throw_proc,
                           mccp_pipeline_stage_shutdown_proc_t
                           shutdown_proc,
                           mccp_pipeline_stage_finalize_proc_t
                           final_proc,
                           mccp_pipeline_stage_freeup_proc_t
                           freeup_proc);


/**
 * Setup a pipeline stage.
 *
 *	@param[in] sptr A pointer to a stage.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_pipeline_stage_setup(const mccp_pipeline_stage_t *sptr);


/**
 * Start a pipeline stage.
 *
 *	@param[in] sptr A pointer to a stage.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval MCCP_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ALREADY_EXISTS	Failed, already exists.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_pipeline_stage_start(const mccp_pipeline_stage_t *sptr);


/**
 * Shutdown a pipeline stage.
 *
 *	@param[in] sptr A pointer to a stage.
 *	@param[in] l A shutdown graceful level.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval MCCP_RESULT_NOT_OWNER	Failed, not the owner.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_pipeline_stage_shutdown(const mccp_pipeline_stage_t *sptr,
                             shutdown_grace_level_t l);


/**
 * Cancel a pipeline stage.
 *
 *	@param[in] sptr A pointer to a stage.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval MCCP_RESULT_NOT_OWNER	Failed, not the owner.
 *	@retval MCCP_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_pipeline_stage_cancel(const mccp_pipeline_stage_t *sptr);


/**
 * Wait for a pipeline stage finishes.
 *
 *	@param[in]  sptrr	A pointer to a stage.
 *	@param[in]  nsec	Wait timeout (nano second).
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_NOT_OPERATIONAL	Failed, will be not
 *	operational in anytime.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval MCCP_RESULT_NOT_OWNER	Failed, not the owner.
 *	@retval MCCP_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval MCCP_RESULT_TIMEDOUT		Failed, timedout.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_pipeline_stage_wait(const mccp_pipeline_stage_t *sptr,
                         mccp_chrono_t nsec);


/**
 * Destroy a pipeline stage.
 *
 *	@param[in] sptr A pointer to a stage.
 *
 *	@details If the stage is not canceled/shutted down, the stage
 *	is canceled.
 */
void
mccp_pipeline_stage_destroy(mccp_pipeline_stage_t *sptr);


/**
 * Clean an internal state of a pipeline stage up after the
 * cancellation of the caller threads.
 *
 *	@param[in] sptr A pointer to a stage.
 */
void
mccp_pipeline_stage_cancel_janitor(const mccp_pipeline_stage_t *sptr);





/**
 * Pause a pipeline stage.
 *
 *	@param[in]  sptrr	A pointer to a stage.
 *	@param[in]  nsec	Timeout for pause completion (nano second).
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_NOT_OPERATIONAL	Failed, will be not
 *	operational in anytime.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval MCCP_RESULT_NOT_OWNER	Failed, not the owner.
 *	@retval MCCP_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval MCCP_RESULT_TIMEDOUT		Failed, timedout.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If a stage is paused, the stage is resumed by calling
 *	of the \b mccp_pipeline_stage_resume().
 *
 *	@details <em> Don't call this function in
 *	mccp_pipeline_stage_*_proc() since no one can't resume if
 *	all the workers were paused and If you did whole the stage
 *	must deaalock. </em>
 */
mccp_result_t
mccp_pipeline_stage_pause(const mccp_pipeline_stage_t *sptr,
                          mccp_chrono_t nsec);


/**
 * Resume a paused pipeline stage.
 *
 *	@param[in]  sptrr	A pointer to a stage.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval MCCP_RESULT_NOT_OWNER	Failed, not the owner.
 *	@retval MCCP_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_pipeline_stage_resume(const mccp_pipeline_stage_t *sptr);


/**
 * Execute a maintenance task of a pipeline stage.
 *
 *	@param[in]  sptr	A pointer to a stage.
 *	@param[in]  arg		An argument.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval MCCP_RESULT_NOT_OWNER	Failed, not the owner.
 *	@retval MCCP_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the maintenace function of the stage is not \b
 *	NULL, it is invoked with the \b arg. It is guaranteed that The
 *	maintenance function is calle only for a single worker and the
 *	caller of this API is not blocked since the maintenance
 *	function is executed in the worker's context.
 */
mccp_result_t
mccp_pipeline_stage_maintenance(const mccp_pipeline_stage_t *sptr,
                                void *arg);



/**
 * Find a pipeline stage by name.
 *
 *	@param[in] name A name of the pipline stage to find.
 *	@param[out] retptr A reference to the found stage pointer.
 *
 *	@retval MCCP_RESULT_OK		Succeeded.
 *	@retval MCCP_RESULT_NOT_FOUND	Failed, stage not found.
 *	@retval MCCP_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval MCCP_RESULT_ANY_FAILURES	Failed.
 */
mccp_result_t
mccp_pipeline_stage_find(const char *name,
                         mccp_pipeline_stage_t *retptr);


#endif /* __MCCP_PIPELINE_STAGE_H__ */
