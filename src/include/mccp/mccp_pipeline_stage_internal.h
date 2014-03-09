#ifndef __MCCP_PIPELINE_STAGE_INTERNAL_H__
#define __MCCP_PIPELINE_STAGE_INTERNAL_H__





#include <mccp/mccp_pipeline_stage_funcs.h>





typedef struct mccp_pipeline_worker_record *	mccp_pipeline_worker_t;


typedef enum {
  STAGE_STATE_UNKNOWN = 0,
  STAGE_STATE_INITIALIZED,
  STAGE_STATE_SETUP,
  STAGE_STATE_STARTED,
  STAGE_STATE_PAUSED,
  STAGE_STATE_SINGLED,
  STAGE_STATE_CANCELED,
  STAGE_STATE_SHUTDOWN,
  STAGE_STATE_FINALIZED,
  STAGE_STATE_DESTROYING
} mccp_pipeline_stage_state_t;


typedef struct mccp_pipeline_stage_record {
  mccp_pipeline_stage_sched_proc_t m_sched_proc;
  mccp_pipeline_stage_maintenance_proc_t m_maintenance_proc;

  mccp_pipeline_stage_setup_proc_t m_setup_proc;
  mccp_pipeline_stage_fetch_proc_t m_fetch_proc;
  mccp_pipeline_stage_main_proc_t m_main_proc;
  mccp_pipeline_stage_throw_proc_t m_throw_proc;
  mccp_pipeline_stage_shutdown_proc_t m_shutdown_proc;
  mccp_pipeline_stage_finalize_proc_t m_final_proc;
  mccp_pipeline_stage_freeup_proc_t m_freeup_proc;

  const char *m_name;

  mccp_pipeline_worker_t *m_workers;
  size_t m_n_workers;

  size_t m_event_size;
  size_t m_max_batch;
  size_t m_batch_buffer_size;	/* == m_event_size * m_max_batch (in bytes.) */

  bool m_is_heap_allocd;

  mccp_mutex_t m_lock;
  mccp_cond_t m_cond;

  volatile bool m_do_loop;
  volatile shutdown_grace_level_t m_sg_lvl;

  volatile mccp_pipeline_stage_state_t m_status;

  mccp_mutex_t m_final_lock;
  size_t m_n_canceled_workers;
  size_t m_n_shutdown_workers;

  volatile bool m_pause_requested;
  mccp_barrier_t m_pause_barrier;
  mccp_mutex_t m_pause_lock;
  mccp_cond_t m_pause_cond;
  mccp_cond_t m_resume_cond;

} mccp_pipeline_stage_record;





#endif /* ! __MCCP_PIPELINE_STAGE_INTERNAL_H__ */
