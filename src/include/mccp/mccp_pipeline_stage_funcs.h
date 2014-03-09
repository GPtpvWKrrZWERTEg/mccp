#ifndef __MCCP_PIPELINE_STAGE_FUNCS_H__
#define __MCCP_PIPELINE_STAGE_FUNCS_H__





/**
 *	@file	mccp_pipeline_stage_funcs.h
 */





#include <mccp/mccp_gstate.h>





#ifndef PIPELINE_STAGE_T_DECLARED
typedef struct mccp_pipeline_stage_record *	mccp_pipeline_stage_t;
#define PIPELINE_STAGE_T_DECLARED
#endif /* PIPELINE_STAGE_T_DECLARED */





/**
 * The signature of pipeline stage setup functions.
 *
 *	@param[in] sptr A pointer to the pipeline stage where this
 *	proc belongs to.
 *
 *	@retval	MCCP_RESULT_OK		Succeeded.
 *	@retval <0				Failed.
 *
 * @details A pipeline stage setup function is invoked once per the
 * pipeline stage, in order to setup (e.g. find the next hop of the
 * pipeline and get it as a pointer of the mccp_pipeline_stage_t)
 * the pipeline before the main stage task loop is started.
 */
typedef mccp_result_t
(*mccp_pipeline_stage_setup_proc_t)(const mccp_pipeline_stage_t *sptr);


/**
 * The signature of pipeline stage fetch functions.
 *
 *	@param[in] sptr A pointer to the pipeline stage where this
 *	proc belongs to.
 *	@param[in] idx An index # of a worker in the stage.
 *	@param[out] evbuf A buffer to store the fetched events into.
 *	@oaram[in] max_n_evs A maximum # of the events to fetch.
 *
 *	@retval	>=0	# of events fetched.
 *	@retval <0	Failed.
 *
 * @details A pipeline stage fetch function is invoked once per each
 * main stage loop iteration, in order to fetch the events to be dealt
 * with a worker specified by the \b idx.
 */
typedef mccp_result_t
(*mccp_pipeline_stage_fetch_proc_t)(const mccp_pipeline_stage_t *sptr,
                                    size_t idx,
                                    void *evbuf,
                                    size_t max_n_evs);


/**
 * The signature of pipeline stage main functions.
 *
 *	@param[in] sptr A pointer to the pipeline stage where this
 *	proc belongs to.
 *	@param[in] idx An index # of a worker in the stage.
 *	@param[inout] evbuf A buffer of the events to be dealt.
 *	@param[in] n_evs A # of events in the \b evbuf.
 *
 *	@retval	>=0	# of events dealt.
 *	@retval <0	Failed.
 *
 * @details A pipeline stage main function is invoked once per each
 * main stage loop iteration. This is the main workhorse of the pipline stage.
 *
 * @details Events in the \b evbuf is dealt with the worker specified
 * by the \b idx.
 *
 * @details The \b evbuf is used as input/output bidirectionally and
 * the events dealt are required to be packed and alligned
 * successively from the head of the \b evbuf.
 */
typedef mccp_result_t
(*mccp_pipeline_stage_main_proc_t)(const mccp_pipeline_stage_t *sptr,
                                   size_t idx,
                                   void *evbuf,
                                   size_t n_evs);


/**
 * The signature of pipeline stage throw functions.
 *
 *	@param[in] sptr A pointer to the pipeline stage where this
 *	proc belongs to.
 *	@param[in] idx An index # of a worker in the stage.
 *	@param[in] evbuf A buffer of the events to be thrown.
 *	@param[in] n_evs A # of events in the \b evbuf.
 *
 *	@retval	>=0	# of events thrown.
 *	@retval <0	Failed.
 *
 * @details A pipeline stage throw function is invoked once per each
 * main stage loop iteration, in order to throw the delat events to
 * the next stage of the pipeline.
 */
typedef mccp_result_t
(*mccp_pipeline_stage_throw_proc_t)(const mccp_pipeline_stage_t *sptr,
                                    size_t idx,
                                    void *evbuf,
                                    size_t n_evs);


/**
 * The signature of piplline stage shutdown functions.
 *
 *	@param[in] sptr A pointer to the pipeline stage where this
 *	proc belongs to.
 *	@param[in] l A shutdown graceful level.
 *
 *	@retval	MCCP_RESULT_OK		Succeeded.
 *	@retval <0				Failed.
 *
 * @details A pipeline stage shutdown function is invoked when the
 * stage is required to shutdown.
 */
typedef mccp_result_t
(*mccp_pipeline_stage_shutdown_proc_t)(const mccp_pipeline_stage_t *sptr,
                                       shutdown_grace_level_t l);


/**
 * The signature of piplline stage finalization functions.
 *
 *	@param[in] sptr A pointer to the pipeline stage where this
 *	proc belongs to.
 *	@param[in] is_canceled \b true if the stage is canceled.
 *
 * @details A pipeline stage finalization function is invoked after the
 * stage is shutted down or canceled, in order to release any temporally
 * allocated/acquired resource (exclude resources allocated at
 * initialization).
 */
typedef void
(*mccp_pipeline_stage_finalize_proc_t)(const mccp_pipeline_stage_t *sptr,
                                       bool is_canceled);


/**
 * The signature of piplline stage free-up functions.
 *
 *	@param[in] sptr A pointer to the pipeline stage where this
 *	proc belongs to.
 *
 * @details A pipeline stage free-up function is invoked when the
 * stage is destroyed, in order to free all the allocated resources
 * (both in constructor and the setup function) up. Must not free the
 * \b sptr itself up in this function.
 */
typedef void
(*mccp_pipeline_stage_freeup_proc_t)(const mccp_pipeline_stage_t *sptr);





/**
 * The signature of pipeline stage schedule functions.
 *
 *	@param[in] sptr A pointer to the pipeline stage where this
 *	proc belongs to.
 *	@param[in] evbuf A buffer of the events to be thrown.
 *	@param[in] n_evs A # of events in the \b evbuf.
 *
 *	@retval	>=0	# of events thrown.
 *	@retval <0	Failed.
 *
 * @details A pipeline stage schedule function is invoked when any
 * batches are submitted by calling \b mccp_pipeline_stage_submit()
 * which is mainly called in other stages' throw function.
 */
typedef mccp_result_t
(*mccp_pipeline_stage_sched_proc_t)(const mccp_pipeline_stage_t *sptr,
                                    void *evbuf,
                                    size_t n_evs);


/**
 * The signature of pipeline stage maintenance functions.
 *
 *	@param[in] sptr A pointer to the pipeline stage where this
 *	proc belongs to.
 *	@param[in] arg	An argument.
 *
 *	@retval	MCCP_RESULT_OK		Succeeded.
 *	@retval <0				Failed.
 *
 * @details A pipeline stage maintenance function is invoked when the
 * \b mccp_pipeline_stage_maintenance() is called, in order to
 * "maintain" the stage (e.g. update any objects which are shared
 * amonng the workers.) The function is called only for a single
 * worker at a time, which gurantees the atomicity of the
 * "maintenance".
 */
typedef mccp_result_t
(*mccp_pipeline_stage_maintenance_proc_t)(
  const mccp_pipeline_stage_t *sptr, void *arg);





#endif /* ! __MCCP_PIPELINE_STAGE_FUNCS_H__ */
