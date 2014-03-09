#ifndef __MCCP_THREAD_INTERNAL_H__
#define __MCCP_THREAD_INTERNAL_H__





typedef struct mccp_thread_record {
  volatile pthread_t m_pthd;
  void *m_arg;

  char m_name[16];	/* max 16 characters. */

  pid_t m_creator_pid;

  mccp_thread_main_proc_t m_main_proc;
  mccp_thread_finalize_proc_t m_final_proc;
  mccp_thread_freeup_proc_t m_freeup_proc;

  mccp_mutex_t m_op_lock;
  mccp_mutex_t m_wait_lock;
  mccp_mutex_t m_cancel_lock;

  mccp_cond_t m_wait_cond;
  mccp_cond_t m_startup_cond;

  mccp_result_t m_result_code;

  volatile bool m_is_started;
  volatile bool m_is_activated;
  volatile bool m_is_canceled;
  volatile bool m_is_finalized;
  volatile bool m_is_destroying;

  bool m_do_autodelete;
} mccp_thread_record;





#endif /* ! __MCCP_THREAD_INTERNAL_H__ */
