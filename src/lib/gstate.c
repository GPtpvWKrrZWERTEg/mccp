#include <mccp/mccp.h>





static pthread_once_t s_once = PTHREAD_ONCE_INIT;

static mccp_mutex_t s_lck = NULL;
static mccp_cond_t s_cond = NULL;
static mccp_global_state_t s_gs = MCCP_GLOBAL_STATE_UNKNOWN;
static shutdown_grace_level_t s_gl = SHUTDOWN_UNKNOWN;

static void s_ctors(void) __attr_constructor__(106);
static void s_dtors(void) __attr_destructor__(106);





static void
s_child_at_fork(void) {
  (void)mccp_mutex_reinitialize(&s_lck);
}


static void
s_once_proc(void) {
  mccp_result_t r;

  if ((r = mccp_mutex_create(&s_lck)) != MCCP_RESULT_OK) {
    mccp_perror(r, "mccp_mutex_create()");
    mccp_exit_fatal("Can't initilize a mutex.\n");
  }
  if ((r = mccp_cond_create(&s_cond)) !=  MCCP_RESULT_OK) {
    mccp_perror(r, "mccp_cond_create()");
    mccp_exit_fatal("Can't initilize a cond.\n");
  }

  (void)pthread_atfork(NULL, NULL, s_child_at_fork);
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  mccp_msg_debug(10, "The global status tracker initialized.\n");
}


static inline void
s_final(void) {
  if (s_cond != NULL) {
    mccp_cond_destroy(&s_cond);
  }
  if (s_lck != NULL) {
    (void)mccp_mutex_lock(&s_lck);
    (void)mccp_mutex_unlock(&s_lck);
    mccp_mutex_destroy(&s_lck);
  }
}


static void
s_dtors(void) {
  s_final();

  mccp_msg_debug(10, "The global status tracker finalized.\n");
}





static inline void
s_lock(void) {
  (void)mccp_mutex_lock(&s_lck);
}


static inline void
s_unlock(void) {
  (void)mccp_mutex_unlock(&s_lck);
}


static inline bool
s_is_valid_state(mccp_global_state_t s) {
  /*
   * Make sure the state transition one way only.
   */
  return ((int)s >= (int)s_gs) ? true : false;
}





mccp_result_t
mccp_global_state_set(mccp_global_state_t s) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (IS_VALID_MCCP_GLOBAL_STATE(s) == true) {

    s_lock();
    {
      if (s_is_valid_state(s) == true) {
        s_gs = s;
        (void)mccp_cond_notify(&s_cond, true);
        ret = MCCP_RESULT_OK;
      } else {
        ret = MCCP_RESULT_INVALID_STATE_TRANSITION;
      }
    }
    s_unlock();

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_global_state_get(mccp_global_state_t *sptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (sptr != NULL) {

    s_lock();
    {
      *sptr = s_gs;
    }
    s_unlock();

    ret = MCCP_RESULT_OK;

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_global_state_wait_for(mccp_global_state_t s_wait_for,
                           mccp_global_state_t *cur_sptr,
                           shutdown_grace_level_t *cur_gptr,
                           mccp_chrono_t nsec) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (IS_VALID_MCCP_GLOBAL_STATE(s_wait_for) == true) {

    s_lock();
    {
    recheck:
      if ((int)s_gs < (int)s_wait_for &&
          IS_MCCP_GLOBAL_STATE_SHUTDOWN(s_gs) == false) {
        ret = mccp_cond_wait(&s_cond, &s_lck, nsec);
        if (ret == MCCP_RESULT_OK) {
          goto recheck;
        }
      } else {
        if (cur_sptr != NULL) {
          *cur_sptr = s_gs;
        }
        if (cur_gptr != NULL) {
          *cur_gptr = s_gl;
        }
        if ((int)s_gs >= (int)s_wait_for) {
          ret = MCCP_RESULT_OK;
        } else if (IS_MCCP_GLOBAL_STATE_SHUTDOWN(s_gs) == true &&
                   IS_MCCP_GLOBAL_STATE_SHUTDOWN(s_wait_for) == false) {
          ret = MCCP_RESULT_NOT_OPERATIONAL;
        } else {
          ret = MCCP_RESULT_OK;
        }
      }
    }
    s_unlock();

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_global_state_wait_for_shutdown_request(shutdown_grace_level_t *cur_gptr,
    mccp_chrono_t nsec) {
  return mccp_global_state_wait_for(MCCP_GLOBAL_STATE_REQUEST_SHUTDOWN,
                                    NULL, cur_gptr, nsec);
}


mccp_result_t
mccp_global_state_request_shutdown(shutdown_grace_level_t l) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (IS_VALID_SHUTDOWN(l) == true) {

    s_lock();
    {
      if (IS_MCCP_GLOBAL_STATE_SHUTDOWN(s_gs) == false &&
          s_gs != MCCP_GLOBAL_STATE_REQUEST_SHUTDOWN) {
        if (s_is_valid_state(MCCP_GLOBAL_STATE_REQUEST_SHUTDOWN) == true) {
          s_gs = MCCP_GLOBAL_STATE_REQUEST_SHUTDOWN;
          s_gl = l;
          (void)mccp_cond_notify(&s_cond, true);

          /*
           * Wait until someone changes the state to
           * MCCP_GLOBAL_STATE_ACCEPT_SHUTDOWN
           */
        recheck:
          if (IS_MCCP_GLOBAL_STATE_SHUTDOWN(s_gs) == false) {
            if ((ret = mccp_cond_wait(&s_cond, &s_lck, -1LL)) ==
                MCCP_RESULT_OK) {
              goto recheck;
            }
          }
        } else {
          ret = MCCP_RESULT_INVALID_STATE_TRANSITION;
        }
      }
    }
    s_unlock();

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
mccp_global_state_cancel_janitor(void) {
  s_unlock();
}


void
mccp_global_state_reset(void) {
  s_lock();
  {
    s_gs = MCCP_GLOBAL_STATE_UNKNOWN;
  }
  s_unlock();
}

