#ifndef __PIPLELINE_WORKER_C__
#define __PIPLELINE_WORKER_C__





/*
 * Woeker implementaion.
 */


typedef mccp_result_t (*worker_main_proc_t)(mccp_pipeline_worker_t w);


typedef struct mccp_pipeline_worker_record {
  mccp_thread_record m_thd;  /* must be placed at the head. */
  mccp_pipeline_stage_t *m_sptr;
  /* ref. to the parent container. */
  size_t m_idx;			/* A worker index in the m_sptr */
  worker_main_proc_t m_proc;
  bool m_is_started;

  uint8_t m_buf[0];		/* A buffer for the batch, must be >=
                                 * (*m_sptr)->m_batch_buffer_size (in
                                 * bytes) and placed at the tail of
                                 * the record. */
} mccp_pipeline_worker_record;





static inline void	s_worker_pause(mccp_pipeline_worker_t w,
                                   mccp_pipeline_stage_t ps);





#define WORKER_LOOP(OPS)                                                \
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;                         \
  if (w != NULL) {                                                      \
    mccp_pipeline_stage_t *sptr = w->m_sptr;                            \
    if (sptr != NULL && *sptr != NULL) {                                \
      void *evbuf = (void *)(w->m_buf);                                 \
      size_t max_n_evs = (*sptr)->m_max_batch;                          \
      size_t idx = w->m_idx;                                            \
      mccp_result_t st = 0;                                             \
      while ((*sptr)->m_do_loop == true &&                              \
             ((st > 0) ||                                               \
              (st == 0 && (*sptr)->m_sg_lvl == SHUTDOWN_UNKNOWN))) {    \
        if ((*sptr)->m_pause_requested == false) {                      \
          { OPS }                                                       \
        } else {                                                        \
          s_worker_pause(w, *sptr);                                     \
        }                                                               \
      }                                                                 \
      if (((*sptr)->m_sg_lvl == SHUTDOWN_RIGHT_NOW ||                   \
           (*sptr)->m_sg_lvl == SHUTDOWN_GRACEFULLY) &&                 \
          st > 0) {                                                     \
        st = MCCP_RESULT_OK;                                            \
      }                                                                 \
      ret = st;                                                         \
    } else {                                                            \
      ret = MCCP_RESULT_INVALID_ARGS;                                   \
    }                                                                   \
  } else {                                                              \
    ret = MCCP_RESULT_INVALID_ARGS;                                     \
  }                                                                     \
  return ret;


/*
 * The st indicates how the main loop iterates:
 *
 *	st <  0:	... stop iteration immediately.
 *	st == 0:	... If the graceful shutdown is requested,
 *			    stop iterateion.
 *	st >  0:	... If the immediate shutdown is requested,
 *			    stop iterateion. Otherwise conrinues the
 *			    iteration.
 */


static mccp_result_t
s_worker_f_m_t(mccp_pipeline_worker_t w) {
  WORKER_LOOP
  (
    if ((st = ((*sptr)->m_fetch_proc)(sptr, idx, evbuf,
                                      max_n_evs)) > 0) {
      if ((st = ((*sptr)->m_main_proc)(sptr, idx, evbuf,
                                       (size_t)st)) > 0) {
        if ((st = ((*sptr)->m_throw_proc)(sptr, idx, evbuf,
                                          (size_t)st)) > 0) {
          continue;
      } else {
          if (st < 0) {
            break;
          }
        }
      } else {
        if (st < 0) {
          break;
        }
      }
    } else {
      if (st < 0) {
        break;
      }
    }
  )
}


static mccp_result_t
s_worker_f_m(mccp_pipeline_worker_t w) {
  WORKER_LOOP
  (
    if ((st = ((*sptr)->m_fetch_proc)(sptr, idx, evbuf,
                                      max_n_evs)) > 0) {
      if ((st = ((*sptr)->m_main_proc)(sptr, idx, evbuf,
                                       (size_t)st)) > 0) {
        continue;
      } else {
        if (st < 0) {
          break;
        }
      }
    } else {
      if (st < 0) {
        break;
      }
    }
  )
}


static mccp_result_t
s_worker_m_t(mccp_pipeline_worker_t w) {
  WORKER_LOOP
  (
    if ((st = ((*sptr)->m_main_proc)(sptr, idx, evbuf,
                                     max_n_evs)) > 0) {
      if ((st = ((*sptr)->m_throw_proc)(sptr, idx, evbuf,
                                        (size_t)st)) > 0) {
        continue;
      } else {
        if (st < 0) {
          break;
        }
      }
    } else {
      if (st < 0) {
        break;
      }
    }
  )
}


static mccp_result_t
s_worker_m(mccp_pipeline_worker_t w) {
  WORKER_LOOP
  (
    if ((st = ((*sptr)->m_main_proc)(sptr, idx, evbuf,
                                     max_n_evs)) > 0) {
      continue;
    } else {
      if (st < 0) {
        break;
      }
    }
  )
}


static inline worker_main_proc_t
s_find_worker_proc(mccp_pipeline_stage_fetch_proc_t fetch_proc,
                   mccp_pipeline_stage_main_proc_t main_proc,
                   mccp_pipeline_stage_throw_proc_t throw_proc) {
  worker_main_proc_t ret = NULL;

  if (fetch_proc != NULL && main_proc != NULL && throw_proc != NULL) {
    ret = s_worker_f_m_t;
  } else if (fetch_proc != NULL && main_proc != NULL && throw_proc == NULL) {
    ret = s_worker_f_m;
  } else if (fetch_proc == NULL && main_proc != NULL && throw_proc != NULL) {
    ret = s_worker_m_t;
  } else if (fetch_proc == NULL && main_proc != NULL && throw_proc == NULL) {
    ret = s_worker_m;
  }

  return ret;
}


static mccp_result_t
s_worker_main(const mccp_thread_t *tptr, void *arg) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  (void)arg;

  if (tptr != NULL) {
    mccp_pipeline_worker_t w = (mccp_pipeline_worker_t)*tptr;
    if (w != NULL) {
      if (w->m_proc != NULL) {
        mccp_global_state_t s;
        shutdown_grace_level_t l;

        /*
         * Wait for the gala opening.
         */
        mccp_msg_debug(10, "waiting for the gala opening...\n");
        ret = mccp_global_state_wait_for(MCCP_GLOBAL_STATE_STARTED,
                                         &s, &l, -1LL);
        if (ret == MCCP_RESULT_OK) {
          if (s == MCCP_GLOBAL_STATE_STARTED) {
            w->m_is_started = true;
            mccp_msg_debug(10, "gala opening.\n");
            /*
             * Do the main loop.
             */
            ret = (w->m_proc)(w);
          } else {
            mccp_exit_fatal("must not happen.\n");
          }
        } else {
          /*
           * Means we are just about to be shutted down before the gala
           * opening.
           */
          mccp_perror(ret, "mccp_global_state_wait_for()");
          if (ret == MCCP_RESULT_NOT_OPERATIONAL) {
            if (IS_MCCP_GLOBAL_STATE_SHUTDOWN(s) == false) {
              mccp_exit_fatal("must not happen.\n");
            }
          }
        }
      } else {
        ret = MCCP_RESULT_INVALID_ARGS;
      }
    } else {
      ret = MCCP_RESULT_INVALID_ARGS;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


static void
s_worker_finalize(const mccp_thread_t *tptr, bool is_canceled, void *arg) {
  (void)arg;

  if (tptr != NULL) {
    mccp_pipeline_worker_t w = (mccp_pipeline_worker_t)*tptr;
    if (w != NULL) {
      mccp_pipeline_stage_t *sptr = w->m_sptr;

      if (sptr != NULL && *sptr != NULL) {

        s_final_lock_stage(*sptr);
        {
          if (is_canceled == true) {
            if (w->m_is_started == false) {
              /*
               * Means it could be canceled while waiting for the gala
               * opening.
               */
              mccp_global_state_cancel_janitor();
            }
            (*sptr)->m_n_canceled_workers++;

            s_pause_unlock_stage(*sptr);
          }
          (*sptr)->m_n_shutdown_workers++;
        }
        s_final_unlock_stage(*sptr);

      }
    }
  }
}


static void
s_worker_freeup(const mccp_thread_t *tptr, void *arg) {
  (void)arg;

  if (tptr != NULL) {
    mccp_pipeline_worker_t w = (mccp_pipeline_worker_t)*tptr;
    if (w != NULL) {
      /*
       * Note: there is nothing to do here for now. This exists only
       * for the future use.
       */
    }
  }
}


static inline mccp_result_t
s_worker_create(mccp_pipeline_worker_t *wptr,
                mccp_pipeline_stage_t *sptr,
                size_t idx,
                worker_main_proc_t proc) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (wptr != NULL &&
      sptr != NULL && *sptr != NULL &&
      IS_VALID_STRING((*sptr)->m_name) == true) {
    mccp_pipeline_worker_t w;

    *wptr = NULL;
    /*
     * Allocate a worker.
     */
    w = (mccp_pipeline_worker_t)malloc(sizeof(*w) +
                                       (*sptr)->m_batch_buffer_size);
    if (w != NULL) {
      char buf[16];
      snprintf(buf, sizeof(buf), "%s:%d", (*sptr)->m_name, (int)idx);
      if ((ret = mccp_thread_create((mccp_thread_t *)&w,
                                    s_worker_main,
                                    s_worker_finalize,
                                    s_worker_freeup,
                                    buf,
                                    NULL)) == MCCP_RESULT_OK) {
        w->m_sptr = sptr;
        w->m_idx = idx;
        w->m_proc = proc;
        w->m_is_started = false;
        (void)memset((void *)(w->m_buf), 0, (*sptr)->m_batch_buffer_size);
        /*
         * Make the object destroyable via mccp_thread_destroy() so
         * we don't have to worry about not to provide our own worker
         * destructor.
         */
        (void)mccp_thread_free_when_destroy((mccp_thread_t *)&w);
        *wptr = w;
      } else {
        free((void *)w);
      }
    } else {
      ret = MCCP_RESULT_NO_MEMORY;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline mccp_result_t
s_worker_start(mccp_pipeline_worker_t *wptr) {
  return mccp_thread_start((mccp_thread_t *)wptr, false);
}


static inline mccp_result_t
s_worker_cancel(mccp_pipeline_worker_t *wptr) {
  return mccp_thread_cancel((mccp_thread_t *)wptr);
}


static inline mccp_result_t
s_worker_wait(mccp_pipeline_worker_t *wptr, mccp_chrono_t nsec) {
  return mccp_thread_wait((mccp_thread_t *)wptr, nsec);
}


static inline void
s_worker_destroy(mccp_pipeline_worker_t *wptr) {
  mccp_thread_destroy((mccp_thread_t *)wptr);
}





static inline void
s_worker_pause(mccp_pipeline_worker_t w,
               mccp_pipeline_stage_t ps) {
  if (w != NULL && ps != NULL) {
    mccp_result_t st;
    bool is_master = false;

    /*
     * Note that the master stage lock (ps->m_lock) is locked by the
     * pauser at this moment.
     */
    if ((st = mccp_barrier_wait(&(ps->m_pause_barrier), &is_master)) ==
        MCCP_RESULT_OK) {

      /*
       * The barrier synchronization done. All the workers are very
       * here now.
       */

      s_pause_lock_stage(ps);
      {
        if (is_master == true) {
          ps->m_status = STAGE_STATE_PAUSED;
          (void)s_pause_notify_stage(ps);
        }

      recheck:
        if (ps->m_pause_requested == true) {
          st = s_resume_cond_wait_stage(ps, -1LL);
          if (st == MCCP_RESULT_OK) {
            goto recheck;
          } else {
            mccp_perror(st, "s_resume_cond_wait_stage()");
            mccp_msg_error("a worker resume error.\n");
          }
        }
      }
      s_pause_unlock_stage(ps);

    } else {
      mccp_perror(st, "mccp_barrier_wait()");
      mccp_msg_error("a worker barrier error.\n");
    }
  }
}





#endif /* __PIPLELINE_WORKER_C__ */
