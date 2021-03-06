#include <mccp/mccp.h>





#define MAX_MODULES		1024
#define MAX_MODULE_NAME		64





typedef enum {
  MODULE_STATE_UNKNOWN = 0,
  MODULE_STATE_REGISTERED,
  MODULE_STATE_INITIALIZED,
  MODULE_STATE_STARTED,
  MODULE_STATE_SHUTDOWN,
  MODULE_STATE_CANCELLING,
  MODULE_STATE_STOPPED,
  MODULE_STATE_FINALIZED
} a_module_state_t;


typedef struct {
  char m_name[MAX_MODULE_NAME];
  mccp_module_initialize_proc_t m_init_proc;
  void *m_init_arg;
  mccp_thread_t *m_thdptr;
  mccp_module_start_proc_t m_start_proc;
  mccp_module_shutdown_proc_t m_shutdown_proc;
  mccp_module_stop_proc_t m_stop_proc;
  mccp_module_finalize_proc_t m_finalize_proc;
  mccp_module_usage_proc_t m_usage_proc;
  a_module_state_t m_status;
} a_module;





static size_t s_n_modules = 0;
static a_module s_modules[MAX_MODULES];

static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static mccp_mutex_t s_lck = NULL;


static void	s_ctors(void) __attr_constructor__(108);
static void	s_dtors(void) __attr_destructor__(108);





static void
s_once_proc(void) {
  mccp_result_t r;
  s_n_modules = 0;
  (void)memset((void *)s_modules, sizeof(s_modules), 0);

  if ((r = mccp_mutex_create(&s_lck)) != MCCP_RESULT_OK) {
    mccp_perror(r, "mccp_mutex_create()");
    mccp_exit_fatal("can't initialize a mutex.\n");
  }
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  mccp_msg_debug(10, "The module manager is initialized.\n");
}


static void
s_final(void) {
  if (s_lck != NULL) {
    mccp_mutex_destroy(&s_lck);
  }
}


static void
s_dtors(void) {
  s_final();

  mccp_msg_debug(10, "The module manager is finalized.\n");
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





static inline mccp_result_t
s_find_module(const char *name, a_module **retmptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(name) == true) {
    ret = MCCP_RESULT_NOT_FOUND;

    if (retmptr != NULL) {
      *retmptr = NULL;
    }

    if (s_n_modules > 0) {
      a_module *mptr;
      size_t i;

      for (i = 0; i < s_n_modules; i++) {
        mptr = &(s_modules[i]);
        if (strcmp(mptr->m_name, name) == 0) {
          if (retmptr != NULL) {
            *retmptr = mptr;
            return (mccp_result_t)i;
          }
        }
      }
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline mccp_result_t
s_register_module(const char *name,
                  mccp_module_initialize_proc_t init_proc,
                  void *extarg,
                  mccp_module_start_proc_t start_proc,
                  mccp_module_shutdown_proc_t shutdown_proc,
                  mccp_module_stop_proc_t stop_proc,
                  mccp_module_finalize_proc_t finalize_proc,
                  mccp_module_usage_proc_t usage_proc) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;
  a_module *mptr = NULL;

  if (s_n_modules < MAX_MODULES &&
      (mptr = &(s_modules[s_n_modules])) != NULL) {
    if (IS_VALID_STRING(name) == true &&
        init_proc != NULL &&
        start_proc != NULL &&
        shutdown_proc != NULL &&
        finalize_proc != NULL) {
      if (s_find_module(name, NULL) == MCCP_RESULT_NOT_FOUND) {
        snprintf(mptr->m_name, sizeof(mptr->m_name), "%s", name);
        mptr->m_init_proc = init_proc;
        mptr->m_init_arg = extarg;
        mptr->m_thdptr = NULL;
        mptr->m_start_proc = start_proc;
        mptr->m_shutdown_proc = shutdown_proc;
        mptr->m_stop_proc = stop_proc;
        mptr->m_finalize_proc = finalize_proc;
        mptr->m_usage_proc = usage_proc;
        mptr->m_status = MODULE_STATE_REGISTERED;

        s_n_modules++;

        ret = MCCP_RESULT_OK;

      } else {
        ret = MCCP_RESULT_ALREADY_EXISTS;
      }
    } else {
      ret = MCCP_RESULT_INVALID_ARGS;
    }
  } else {
    ret = MCCP_RESULT_NO_MEMORY;
  }

  return ret;
}


static inline mccp_result_t
s_initialize_module(a_module *mptr, int argc, const char *const argv[]) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mptr != NULL &&
      mptr->m_init_proc != NULL) {
    if (mptr->m_status == MODULE_STATE_REGISTERED) {
      ret = (mptr->m_init_proc)(argc, argv, mptr->m_init_arg,
                                &(mptr->m_thdptr));
      if (ret == MCCP_RESULT_OK) {
        mptr->m_status = MODULE_STATE_INITIALIZED;
      }
    } else {
      ret = MCCP_RESULT_INVALID_STATE_TRANSITION;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline mccp_result_t
s_start_module(a_module *mptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mptr != NULL &&
      mptr->m_start_proc != NULL) {
    if (mptr->m_status == MODULE_STATE_INITIALIZED) {
      ret = (mptr->m_start_proc)();
      if (ret == MCCP_RESULT_OK) {
        mptr->m_status = MODULE_STATE_STARTED;
      }
    } else {
      if (mptr->m_status == MODULE_STATE_STARTED) {
        ret = MCCP_RESULT_OK;
      } else {
        ret = MCCP_RESULT_INVALID_STATE_TRANSITION;
      }
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline mccp_result_t
s_shutdown_module(a_module *mptr, shutdown_grace_level_t level) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mptr != NULL &&
      mptr->m_shutdown_proc != NULL &&
      IS_VALID_SHUTDOWN(level) == true) {
    if (mptr->m_status == MODULE_STATE_STARTED) {
      ret = (mptr->m_shutdown_proc)(level);
      if (mptr->m_thdptr == NULL) {
        /*
         * Means that this module doesn't have any mccp'd threads
         * nor pthreads. Just change the state to shutdwon.
         */
        mptr->m_status = MODULE_STATE_SHUTDOWN;
      }
#if 0
      else {
        /*
         * Note that we don't update the module status.
         */
      }
#endif
    } else {
      if (mptr->m_status == MODULE_STATE_SHUTDOWN ||
          mptr->m_status == MODULE_STATE_STOPPED) {
        ret = MCCP_RESULT_OK;
      } else {
        ret = MCCP_RESULT_INVALID_STATE_TRANSITION;
      }
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline mccp_result_t
s_stop_module(a_module *mptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mptr != NULL) {
    if (mptr->m_status == MODULE_STATE_STARTED) {
      if (mptr->m_stop_proc != NULL) {
        ret = (mptr->m_stop_proc)();
      } else {
        if (mptr->m_thdptr != NULL) {
          ret = mccp_thread_cancel(mptr->m_thdptr);
        } else {
          /*
           * Means this module doesn't support any stop/cancel
           * methods. Do nothing.
           */
          return MCCP_RESULT_OK;
        }
      }
      if (ret == MCCP_RESULT_OK) {
        if (mptr->m_thdptr == NULL) {
          /*
           * Means that this module doesn't have any mccp'd threads
           * nor pthreads. Just change the state to shutdwon.
           */
          mptr->m_status = MODULE_STATE_SHUTDOWN;
        } else {
          mptr->m_status = MODULE_STATE_CANCELLING;
        }
      }
    } else {
      if (mptr->m_status == MODULE_STATE_CANCELLING ||
          mptr->m_status == MODULE_STATE_SHUTDOWN ||
          mptr->m_status == MODULE_STATE_STOPPED) {
        ret = MCCP_RESULT_OK;
      } else {
        ret = MCCP_RESULT_INVALID_STATE_TRANSITION;
      }
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline mccp_result_t
s_wait_module(a_module *mptr, mccp_chrono_t nsec) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mptr != NULL) {
    if ((mptr->m_status == MODULE_STATE_CANCELLING ||
         mptr->m_status == MODULE_STATE_STARTED) &&
        mptr->m_thdptr != NULL) {
      ret = mccp_thread_wait(mptr->m_thdptr, nsec);
      if (ret == MCCP_RESULT_OK) {
        bool is_canceled = false;
        (void)mccp_thread_is_canceled(mptr->m_thdptr, &is_canceled);
        mptr->m_status = (is_canceled == false) ?
                         MODULE_STATE_SHUTDOWN : MODULE_STATE_STOPPED;
      }
    } else {
      if (mptr->m_status == MODULE_STATE_SHUTDOWN ||
          mptr->m_status == MODULE_STATE_STOPPED) {
        ret = MCCP_RESULT_OK;
      } else {
        ret = MCCP_RESULT_INVALID_STATE_TRANSITION;
      }
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_finalize_module(a_module *mptr) {
  if (mptr != NULL &&
      mptr->m_finalize_proc != NULL) {
    if (mptr->m_status != MODULE_STATE_STARTED &&
        mptr->m_status != MODULE_STATE_CANCELLING) {
      (mptr->m_finalize_proc)();
    } else {
      mccp_msg_warning("the module \"%s\" seems to be still running, "
                       "won't destruct it for safe.\n",
                       mptr->m_name);
    }
  }
}


static inline void
s_usage_module(a_module *mptr, FILE *fd) {
  if (mptr != NULL && fd != NULL &&
      mptr->m_usage_proc != NULL) {
    (mptr->m_usage_proc)(fd);
  }
}





mccp_result_t
mccp_module_register(const char *name,
                     mccp_module_initialize_proc_t init_proc,
                     void *extarg,
                     mccp_module_start_proc_t start_proc,
                     mccp_module_shutdown_proc_t shutdown_proc,
                     mccp_module_stop_proc_t stop_proc,
                     mccp_module_finalize_proc_t finalize_proc,
                     mccp_module_usage_proc_t usage_proc) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  s_lock();
  {
    ret = s_register_module(name,
                            init_proc, extarg,
                            start_proc,
                            shutdown_proc,
                            stop_proc,
                            finalize_proc,
                            usage_proc);
  }
  s_unlock();

  return ret;
}





void
mccp_module_usage_all(FILE *fd) {
  if (fd != NULL) {

    s_lock();
    {
      if (s_n_modules > 0) {
        size_t i;
        a_module *mptr;

        for (i = 0; i < s_n_modules; i++) {
          mptr = &(s_modules[i]);
          s_usage_module(mptr, fd);
        }
      }
    }
    s_unlock();

  }
}


mccp_result_t
mccp_module_initialize_all(int argc, const char *const argv[]) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  s_lock();
  {
    if (s_n_modules > 0) {
      size_t i;
      a_module *mptr;

      for (ret = MCCP_RESULT_OK, i = 0;
           ret == MCCP_RESULT_OK && i < s_n_modules;
           i++) {
        mptr = &(s_modules[i]);
        ret = s_initialize_module(mptr, argc, argv);
        if (ret != MCCP_RESULT_OK) {
          mccp_perror(ret, "s_initialize_module()");
          mccp_msg_error("can't initialize module \"%s\".\n",
                         mptr->m_name);
        }
      }
    } else {
      ret = MCCP_RESULT_OK;
    }
  }
  s_unlock();

  return ret;
}


mccp_result_t
mccp_module_start_all(void) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  s_lock();
  {
    if (s_n_modules > 0) {
      size_t i;
      a_module *mptr;

      for (ret = MCCP_RESULT_OK, i = 0;
           ret == MCCP_RESULT_OK && i < s_n_modules;
           i++) {
        mptr = &(s_modules[i]);
        ret = s_start_module(mptr);
        if (ret != MCCP_RESULT_OK) {
          mccp_perror(ret, "s_start_module()");
          mccp_msg_error("can't start module \"%s\".\n",
                         mptr->m_name);
        }
      }
    } else {
      ret = MCCP_RESULT_OK;
    }
  }
  s_unlock();

  return ret;
}


mccp_result_t
mccp_module_shutdown_all(shutdown_grace_level_t level) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (IS_VALID_SHUTDOWN(level) == true) {

    s_lock();
    {
      if (s_n_modules > 0) {
        mccp_result_t first_err = MCCP_RESULT_OK;
        size_t i;
        a_module *mptr;

        /*
         * Reverse order.
         */
        for (i = 0; i < s_n_modules; i++) {
          mptr = &(s_modules[s_n_modules - i - 1]);
          ret = s_shutdown_module(mptr, level);
          if (ret != MCCP_RESULT_OK) {
            mccp_perror(ret, "s_shutdown_module()");
            mccp_msg_error("can't shutdown module \"%s\".\n",
                           mptr->m_name);
            if (first_err == MCCP_RESULT_OK) {
              first_err = ret;
            }
          }
          /*
           * Just carry on shutting down no matter what kind of errors
           * occur.
           */
        }

        ret = first_err;

      } else {
        ret = MCCP_RESULT_OK;
      }
    }
    s_unlock();

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_module_stop_all(void) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  s_lock();
  {
    if (s_n_modules > 0) {
      mccp_result_t first_err = MCCP_RESULT_OK;
      size_t i;
      a_module *mptr;

      /*
       * Reverse order.
       */
      for (i = 0; i < s_n_modules; i++) {
        mptr = &(s_modules[s_n_modules - i - 1]);
        ret = s_stop_module(mptr);
        if (ret != MCCP_RESULT_OK) {
          mccp_perror(ret, "s_stop_module()");
          mccp_msg_error("can't stop module \"%s\".\n",
                         mptr->m_name);
          if (first_err == MCCP_RESULT_OK) {
            first_err = ret;
          }
        }
        /*
         * Just carry on stopping no matter what kind of errors
         * occur.
         */
      }

      ret = first_err;

    } else {
      ret = MCCP_RESULT_OK;
    }
  }
  s_unlock();

  return ret;
}


mccp_result_t
mccp_module_wait_all(mccp_chrono_t nsec) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  s_lock();
  {
    if (s_n_modules > 0) {
      mccp_result_t first_err = MCCP_RESULT_OK;
      size_t i;
      a_module *mptr;

      /*
       * Reverse order.
       */

      if (nsec < 0LL) {

        for (i = 0; i < s_n_modules; i++) {
          mptr = &(s_modules[s_n_modules - i - 1]);
          ret = s_wait_module(mptr, -1LL);
          if (ret != MCCP_RESULT_OK) {
            mccp_perror(ret, "s_wait_module()");
            mccp_msg_error("can't wait module \"%s\".\n",
                           mptr->m_name);
            if (first_err == MCCP_RESULT_OK) {
              first_err = ret;
            }
          }
          /*
           * Just carry on wait no matter what kind of errors
           * occur.
           */
        }

      } else {

        mccp_chrono_t w_begin;
        mccp_chrono_t w_end;
        mccp_chrono_t w = nsec;

        for (i = 0; i < s_n_modules; i++) {
          mptr = &(s_modules[s_n_modules - i - 1]);
          WHAT_TIME_IS_IT_NOW_IN_NSEC(w_begin);
          ret = s_wait_module(mptr, w);
          WHAT_TIME_IS_IT_NOW_IN_NSEC(w_end);
          if (ret != MCCP_RESULT_OK) {
            mccp_perror(ret, "s_wait_module()");
            mccp_msg_error("can't wait module \"%s\".\n",
                           mptr->m_name);
            if (first_err == MCCP_RESULT_OK) {
              first_err = ret;
            }
          }
          /*
           * Just carry on wait no matter what kind of errors
           * occur.
           */
          w = nsec - (w_end - w_begin);
          if (w < 0LL) {
            w = 0LL;
          }
        }
      }

      ret = first_err;

    } else {
      ret = MCCP_RESULT_OK;
    }
  }
  s_unlock();

  return ret;
}


void
mccp_module_finalize_all(void) {
  s_lock();
  {
    if (s_n_modules > 0) {
      size_t i;
      a_module *mptr;

      /*
       * Reverse order.
       */
      for (i = 0; i < s_n_modules; i++) {
        mptr = &(s_modules[s_n_modules - i - 1]);
        s_finalize_module(mptr);
      }
    }
  }
  s_unlock();
}





mccp_result_t
mccp_module_find(const char *name) {
  return s_find_module(name, NULL);
}
