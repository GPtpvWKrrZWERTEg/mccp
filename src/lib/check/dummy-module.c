#include <mccp/mccp.h>





static volatile bool s_do_loop = false;
static volatile bool s_is_started = false;
static volatile bool s_is_initialized = false;
static volatile bool s_is_gracefull = false;
static mccp_thread_t s_thd = NULL;
static mccp_mutex_t s_lck = NULL;


static void
s_dummy_thd_finalize(const mccp_thread_t *tptr, bool is_canceled,
                     void *arg) {
  (void)tptr;
  (void)arg;

  if (is_canceled == true) {
    if (s_is_started == false) {
      /*
       * Means this thread is canceled while waiting for the global
       * state change.
       */
      mccp_global_state_cancel_janitor();
    }
  }

  mccp_msg_debug(5, "called, %s.\n",
                 (is_canceled == true) ? "canceled" : "exited");
}


static void
s_dummy_thd_destroy(const mccp_thread_t *tptr, void *arg) {
  (void)tptr;
  (void)arg;

  mccp_msg_debug(5, "called.\n");
}


static mccp_result_t
s_dummy_thd_main(const mccp_thread_t *tptr, void *arg) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;
  mccp_global_state_t s;
  shutdown_grace_level_t l;

  (void)tptr;
  (void)arg;

  mccp_msg_debug(5, "waiting for the gala opening...\n");

  ret = mccp_global_state_wait_for(MCCP_GLOBAL_STATE_STARTED, &s, &l, -1LL);
  if (ret == MCCP_RESULT_OK &&
      s == MCCP_GLOBAL_STATE_STARTED) {

    s_is_started = true;

    mccp_msg_debug(5, "gala opening.\n");

    /*
     * The main task loop.
     */
    while (s_do_loop == true) {
      mccp_msg_debug(6, "looping...\n");
      (void)mccp_chrono_nanosleep(1000LL * 1000LL * 500LL, NULL);
      /*
       * Create an explicit cancalation point since this loop has
       * none of it.
       */
      pthread_testcancel();
    }
    if (s_is_gracefull == true) {
      /*
       * This is just emulating/mimicking a graceful shutdown by
       * sleep().  Don't do this on actual modules.
       */
      mccp_msg_debug(5, "mimicking gracefull shutdown...\n");
      sleep(5);
      mccp_msg_debug(5, "mimicking gracefull shutdown done.\n");
      ret = MCCP_RESULT_OK;
    } else {
      ret = 1LL;
    }
  }

  return ret;
}





static inline void
s_lock(void) {
  if (s_lck != NULL) {
    (void)mccp_mutex_lock(&s_lck);
  }
}


static inline void
s_unlock(void) {
  if (s_lck != NULL) {
    (void)mccp_mutex_unlock(&s_lck);
  }
}





static mccp_result_t
dummy_module_initialize(int argc,
                        const char *const argv[],
                        void *extarg,
                        mccp_thread_t **thdptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  (void)extarg;

  mccp_msg_debug(5, "called.\n");

  if (thdptr != NULL) {
    *thdptr = NULL;
  }

  s_lock();
  {
    if (s_is_initialized == false) {
      int i;

      mccp_msg_debug(5, "argc: %d\n", argc);
      for (i = 0; i < argc; i++) {
        mccp_msg_debug(5, "%5d: '%s'\n", i, argv[i]);
      }

      ret = mccp_thread_create(&s_thd,
                               s_dummy_thd_main,
                               s_dummy_thd_finalize,
                               s_dummy_thd_destroy,
                               "dummy", NULL);
      if (ret == MCCP_RESULT_OK) {
        s_is_initialized = true;
        if (thdptr != NULL) {
          *thdptr = &s_thd;
        }
      }
    } else {
      ret = MCCP_RESULT_OK;
    }
  }
  s_unlock();

  return ret;
}


static mccp_result_t
dummy_module_start(void) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  mccp_msg_debug(5, "called.\n");

  if (s_thd != NULL) {

    s_lock();
    {
      if (s_is_initialized == true) {
        ret = mccp_thread_start(&s_thd, false);
        if (ret == MCCP_RESULT_OK) {
          s_do_loop = true;
        }
      } else {
        ret = MCCP_RESULT_INVALID_STATE_TRANSITION;
      }
    }
    s_unlock();

  } else {
    ret = MCCP_RESULT_INVALID_OBJECT;
  }

  return ret;
}


static mccp_result_t
dummy_module_shutdown(shutdown_grace_level_t l) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  mccp_msg_debug(5, "called.\n");

  if (s_thd != NULL) {

    s_lock();
    {
      if (s_is_initialized == true) {
        if (l == SHUTDOWN_GRACEFULLY) {
          s_is_gracefull = true;
        }
        s_do_loop = false;
        ret = MCCP_RESULT_OK;
      } else {
        ret = MCCP_RESULT_INVALID_STATE_TRANSITION;
      }
    }
    s_unlock();

  } else {
    ret = MCCP_RESULT_INVALID_OBJECT;
  }

  return ret;
}


static mccp_result_t
dummy_module_stop(void) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  mccp_msg_debug(5, "called.\n");

  if (s_thd != NULL) {

    s_lock();
    {
      if (s_is_initialized == true) {
        ret = mccp_thread_cancel(&s_thd);
      } else {
        ret = MCCP_RESULT_INVALID_STATE_TRANSITION;
      }
    }
    s_unlock();

  } else {
    ret = MCCP_RESULT_INVALID_OBJECT;
  }

  return ret;
}


static void
dummy_module_finalize(void) {
  mccp_msg_debug(5, "called.\n");

  if (s_thd != NULL) {

    s_lock();
    {
      mccp_thread_destroy(&s_thd);
    }
    s_unlock();

  }
}


static void
dummy_module_usage(FILE *fd) {
  if (fd != NULL) {
    fprintf(fd, "\t--dummy\tdummy.\n");
  }
}





#define MY_MOD_IDX	MCCP_MODULE_CONSTRUCTOR_INDEX_BASE + 100
#define MY_MOD_NAME	"dummy"


static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static void	s_ctors(void) __attr_constructor__(MY_MOD_IDX);
static void	s_dtors(void) __attr_destructor__(MY_MOD_IDX);


static void
s_once_proc(void) {
  mccp_result_t r;

  mccp_msg_debug(5, "called.\n");

  if ((r = mccp_mutex_create(&s_lck)) != MCCP_RESULT_OK) {
    mccp_perror(r, "mccp_mutex_create()");
    mccp_exit_fatal("can't initialize a mutex.\n");
  }

  if ((r = mccp_module_register(MY_MOD_NAME,
                                dummy_module_initialize, NULL,
                                dummy_module_start,
                                dummy_module_shutdown,
                                dummy_module_stop,
                                dummy_module_finalize,
                                dummy_module_usage)) != MCCP_RESULT_OK) {
    mccp_perror(r, "mccp_module_register()");
    mccp_exit_fatal("can't register the %s module.\n", MY_MOD_NAME);
  }
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  mccp_msg_debug(5, "called.\n");
}


static inline void
s_final(void) {
  if (s_lck != NULL) {
    mccp_mutex_destroy(&s_lck);
  }
}


static void
s_dtors(void) {
  s_final();

  mccp_msg_debug(5, "called.\n");
}
