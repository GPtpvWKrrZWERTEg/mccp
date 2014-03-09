#include <mccp/mccp.h>
#include <mccp/mccp_thread_internal.h>





static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static pthread_attr_t s_attr;
static mccp_hashmap_t s_thd_tbl;
static mccp_hashmap_t s_alloc_tbl;
static void s_ctors(void) __attr_constructor__(104);
static void s_dtors(void) __attr_destructor__(104);





static void
s_child_at_fork(void) {
  mccp_hashmap_atfork_child(&s_thd_tbl);
  mccp_hashmap_atfork_child(&s_alloc_tbl);
}


static void
s_once_proc(void) {
  int st;
  mccp_result_t r;

  if ((st = pthread_attr_init(&s_attr)) == 0) {
    if ((st = pthread_attr_setdetachstate(&s_attr,
                                          PTHREAD_CREATE_DETACHED)) != 0) {
      errno = st;
      perror("pthread_attr_setdetachstate");
      mccp_exit_fatal("can't initialize detached thread attr.\n");
    }
  } else {
    errno = st;
    perror("pthread_attr_init");
    mccp_exit_fatal("can't initialize thread attr.\n");
  }

  if ((r = mccp_hashmap_create(&s_thd_tbl, MCCP_HASHMAP_TYPE_ONE_WORD,
                               NULL)) != MCCP_RESULT_OK) {
    mccp_perror(r, "mccp_hashmap_create()");
    mccp_exit_fatal("can't initialize the thread table.\n");
  }
  if ((r = mccp_hashmap_create(&s_alloc_tbl, MCCP_HASHMAP_TYPE_ONE_WORD,
                               NULL)) != MCCP_RESULT_OK) {
    mccp_perror(r, "mccp_hashmap_create()");
    mccp_exit_fatal("can't initialize the thread allocation table.\n");
  }

  (void)pthread_atfork(NULL, NULL, s_child_at_fork);
}


static inline void
s_init(void) {
  mccp_heapcheck_module_initialize();
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  mccp_msg_debug(10, "The thread module is initialized.\n");
}


static void
s_final(void) {
  mccp_hashmap_destroy(&s_thd_tbl, true);
  mccp_hashmap_destroy(&s_alloc_tbl, true);
}


static void
s_dtors(void) {
  s_final();

  mccp_msg_debug(10, "The thread module is finalized.\n");
}





static inline void
s_add_thd(mccp_thread_t thd) {
  void *val = (void *)true;
  (void)mccp_hashmap_add(&s_thd_tbl, (void *)thd, &val, true);
}


static inline void
s_alloc_mark_thd(mccp_thread_t thd) {
  void *val = (void *)true;
  (void)mccp_hashmap_add(&s_alloc_tbl, (void *)thd, &val, true);
}


static inline void
s_delete_thd(mccp_thread_t thd) {
  (void)mccp_hashmap_delete(&s_thd_tbl, (void *)thd, NULL, true);
  (void)mccp_hashmap_delete(&s_alloc_tbl, (void *)thd, NULL, true);
}


static inline bool
s_is_thd(mccp_thread_t thd) {
  void *val;
  mccp_result_t r = mccp_hashmap_find(&s_thd_tbl, (void *)thd, &val);
  return (r == MCCP_RESULT_OK && (bool)val == true) ?
         true : false;
}


static inline bool
s_is_alloc_marked(mccp_thread_t thd) {
  void *val;
  mccp_result_t r = mccp_hashmap_find(&s_alloc_tbl, (void *)thd, &val);
  return (r == MCCP_RESULT_OK && (bool)val == true) ?
         true : false;
}


static inline void
s_op_lock(mccp_thread_t thd) {
  if (thd != NULL) {
    (void)mccp_mutex_lock(&(thd->m_op_lock));
  }
}


static inline void
s_op_unlock(mccp_thread_t thd) {
  if (thd != NULL) {
    (void)mccp_mutex_unlock(&(thd->m_op_lock));
  }
}


static inline void
s_wait_lock(mccp_thread_t thd) {
  if (thd != NULL) {
    (void)mccp_mutex_lock(&(thd->m_wait_lock));
  }
}


static inline void
s_wait_unlock(mccp_thread_t thd) {
  if (thd != NULL) {
    (void)mccp_mutex_unlock(&(thd->m_wait_lock));
  }
}


static inline void
s_cancel_lock(mccp_thread_t thd) {
  if (thd != NULL) {
    (void)mccp_mutex_lock(&(thd->m_cancel_lock));
  }
}


static inline void
s_cancel_unlock(mccp_thread_t thd) {
  if (thd != NULL) {
    (void)mccp_mutex_unlock(&(thd->m_cancel_lock));
  }
}





static inline mccp_result_t
s_initialize(mccp_thread_t thd,
             mccp_thread_main_proc_t mainproc,
             mccp_thread_finalize_proc_t finalproc,
             mccp_thread_freeup_proc_t freeproc,
             const char *name,
             void *arg) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (thd != NULL) {
    (void)memset((void *)thd, 0, sizeof(*thd));
    s_add_thd(thd);
    if (((ret = mccp_mutex_create(&(thd->m_wait_lock))) ==
         MCCP_RESULT_OK) &&
        ((ret = mccp_mutex_create(&(thd->m_op_lock))) ==
         MCCP_RESULT_OK) &&
        ((ret = mccp_mutex_create(&(thd->m_cancel_lock))) ==
         MCCP_RESULT_OK) &&
        ((ret = mccp_cond_create(&(thd->m_wait_cond))) ==
         MCCP_RESULT_OK) &&
        ((ret = mccp_cond_create(&(thd->m_startup_cond))) ==
         MCCP_RESULT_OK)) {
      thd->m_arg = arg;
      if (IS_VALID_STRING(name) == true) {
        snprintf(thd->m_name, sizeof(thd->m_name), "%s", name);
      }
      thd->m_creator_pid = getpid();
      thd->m_pthd = MCCP_INVALID_THREAD;
      thd->m_main_proc = mainproc;
      thd->m_final_proc = finalproc;
      thd->m_freeup_proc = freeproc;
      thd->m_result_code = MCCP_RESULT_NOT_STARTED;

      thd->m_is_started = false;
      thd->m_is_activated = false;
      thd->m_is_canceled = false;
      thd->m_is_finalized = false;
      thd->m_is_destroying = false;

      thd->m_do_autodelete = false;

      ret = MCCP_RESULT_OK;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_finalize(mccp_thread_t thd, bool is_canceled) {
  bool do_autodelete = false;

  s_wait_lock(thd);
  {
    if (thd->m_final_proc != NULL) {
      thd->m_final_proc(&thd, is_canceled, thd->m_arg);
    }
    thd->m_pthd = MCCP_INVALID_THREAD;
    do_autodelete = thd->m_do_autodelete;
    thd->m_is_finalized = true;
    thd->m_is_activated = false;
    (void)mccp_cond_notify(&(thd->m_wait_cond), true);
  }
  s_wait_unlock(thd);

  if (do_autodelete == true) {
    mccp_thread_destroy(&thd);
  }

  if (is_canceled == false) {
    pthread_exit(NULL);
  }
}


static inline void
s_delete(mccp_thread_t thd) {
  if (thd != NULL) {
#if 0
    if (s_is_thd(thd) == true) {
      if (mccp_heapcheck_is_mallocd((const void *)thd) == true) {
        if (s_is_alloc_marked(thd) == true) {

          mccp_msg_debug(5, "free %p\n", (void *)thd);
          free((void *)thd);
        } else {
          mccp_msg_debug(5, "%p is a thread object NOT allocated by the "
                         "mccp_thread_create(). If you want to free(3) "
                         "this by calling the mccp_thread_destroy(), "
                         "call mccp_thread_free_when_destroy(%p).\n",
                         (void *)thd, (void *)thd);
        }
      } else if (mccp_heapcheck_is_in_heap((const void *)thd) == true) {
        mccp_msg_debug(5, "A thread was allocated in a heap address (%p) "
                       "but it is not a head address of malloc()'d block. "
                       "Call free(3) for the block containing the address "
                       "%p explicitly or it will leak.\n",
                       (void *)thd, (void *)thd);
      } else {
        mccp_msg_debug(10, "A thread was created in an address %p and it "
                       "seems not in the heap area.\n", (void *)thd);
      }

      s_delete_thd(thd);
    }
#else
    if (s_is_thd(thd) == true) {
      if (s_is_alloc_marked(thd) == true) {
        mccp_msg_debug(5, "free %p\n", (void *)thd);
        free((void *)thd);
      } else {
        if (mccp_heapcheck_is_in_heap((const void *)thd) == true) {
          mccp_msg_debug(5, "%p is a thread object NOT allocated by the "
                         "mccp_thread_create(). If you want to free(3) "
                         "this by calling the mccp_thread_destroy(), "
                         "call mccp_thread_free_when_destroy(%p).\n",
                         (void *)thd, (void *)thd);
        } else {
          mccp_msg_debug(10, "A thread was created in an address %p and it "
                         "seems not in the heap area.\n", (void *)thd);
        }
      }

      s_delete_thd(thd);
    }
#endif
  }
}


static inline void
s_destroy(mccp_thread_t thd) {
  if (thd != NULL) {
    if (s_is_thd(thd) == true) {
      if (thd->m_wait_lock != NULL) {

        s_wait_lock(thd);
        {
          if (thd->m_op_lock != NULL) {
            (void)mccp_mutex_destroy(&(thd->m_op_lock));
          }
          if (thd->m_cancel_lock != NULL) {
            (void)mccp_mutex_destroy(&(thd->m_cancel_lock));
          }
          if (thd->m_wait_cond != NULL) {
            (void)mccp_cond_destroy(&(thd->m_wait_cond));
          }
          if (thd->m_startup_cond != NULL) {
            (void)mccp_cond_destroy(&(thd->m_startup_cond));
          }
        }
        s_wait_unlock(thd);

        (void)mccp_mutex_destroy(&(thd->m_wait_lock));
      }

      s_delete(thd);

    } else {
      mccp_msg_error("Trying to delete a non-thread pointer (%p) "
                     "as a thread.\n", (void *)thd);
    }
  }
}





static void
s_pthd_cancel_handler(void *ptr) {
  if (ptr != NULL) {
    mccp_thread_t thd = (mccp_thread_t)ptr;

    s_cancel_lock(thd);
    {
      thd->m_is_canceled = true;
    }
    s_cancel_unlock(thd);

    s_finalize(thd, true);
  }
}


static void *
s_pthd_entry_point(void *ptr) {
  if (ptr != NULL) {
    mccp_thread_t thd = (mccp_thread_t)ptr;
    int o_cancel_state;
    mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;
    volatile bool is_cleanly_finished = false;

    pthread_cleanup_push(s_pthd_cancel_handler, ptr);
    {
      const mccp_thread_t *tptr =
        (const mccp_thread_t *)&thd;

      (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &o_cancel_state);
      {

        s_wait_lock(thd);
        {
          s_cancel_lock(thd);
          {
            thd->m_is_canceled = false;
          }
          s_cancel_unlock(thd);

          thd->m_is_finalized = false;
          thd->m_is_activated = true;
          thd->m_is_started = true;
          (void)mccp_cond_notify(&(thd->m_startup_cond), true);
        }
        s_wait_unlock(thd);

      }
      (void)pthread_setcancelstate(o_cancel_state, NULL);

      /*
       * A BOGUS ALERT:
       *
       *	OMG, according to clang/scan-build, the thd is
       *	modified here. How could I live like this?
       */
      ret = thd->m_main_proc(tptr, thd->m_arg);
      is_cleanly_finished = true;
    }
    pthread_cleanup_pop((is_cleanly_finished == true) ? 0 : 1);

    s_op_lock(thd);
    {
      /*
       * A BOGUS ALERT:
       *
       *	OMG, also according to clang/scan-build, the thd could
       *	be NULL. How could I live like this?
       */
      thd->m_result_code = ret;
    }
    s_op_unlock(thd);

    s_finalize(thd, false);
  }

  return NULL;
}





mccp_result_t
mccp_thread_create(mccp_thread_t *thdptr,
                   mccp_thread_main_proc_t mainproc,
                   mccp_thread_finalize_proc_t finalproc,
                   mccp_thread_freeup_proc_t freeproc,
                   const char *name,
                   void *arg) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      mainproc != NULL) {
    mccp_thread_t thd;

    if (*thdptr != NULL) {
      thd = *thdptr;
    } else {
      thd = (mccp_thread_t)malloc(sizeof(*thd));
      if (thd == NULL) {
        *thdptr = NULL;
        ret = MCCP_RESULT_NO_MEMORY;
        goto done;
      }
      s_alloc_mark_thd(thd);
    }
    ret = s_initialize(thd, mainproc, finalproc, freeproc, name, arg);
    if (ret != MCCP_RESULT_OK) {
      s_destroy(thd);
      thd = NULL;
    }
    if (*thdptr == NULL) {
      *thdptr = thd;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}


mccp_result_t
mccp_thread_start(const mccp_thread_t *thdptr,
                  bool autodelete) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {

      int st;

      s_wait_lock(*thdptr);
      {
        if ((*thdptr)->m_is_activated == false) {
          (*thdptr)->m_do_autodelete = autodelete;
          errno = 0;
          (*thdptr)->m_pthd = MCCP_INVALID_THREAD;
          if ((st = pthread_create((pthread_t *)&((*thdptr)->m_pthd),
                                   &s_attr,
                                   s_pthd_entry_point,
                                   (void *)*thdptr)) == 0) {

            (*thdptr)->m_is_activated = false;
            (*thdptr)->m_is_finalized = false;
            (*thdptr)->m_is_destroying = false;
            (*thdptr)->m_is_canceled = false;
            (*thdptr)->m_is_started = false;

#ifdef HAVE_PTHREAD_SETNAME_NP
            if (IS_VALID_STRING((*thdptr)->m_name) == true) {
              (void)pthread_setname_np((*thdptr)->m_pthd, (*thdptr)->m_name);
            }
#endif /* HAVE_PTHREAD_SETNAME_NP */

            /*
             * Wait the spawned thread starts.
             */

          startcheck:
            if ((*thdptr)->m_is_started == false) {

              /*
               * Note that very here, very this moment the spawned
               * thread starts to run since this thread sleeps via
               * calling of the pthread_cond_wait() that implies
               * release of the lock.
               */

              ret = mccp_cond_wait(&((*thdptr)->m_startup_cond),
                                   &((*thdptr)->m_wait_lock),
                                   -1);

              if (ret == MCCP_RESULT_OK) {
                goto startcheck;
              } else {
                goto unlock;
              }
            }

            ret = MCCP_RESULT_OK;

          } else {
            errno = st;
            ret = MCCP_RESULT_POSIX_API_ERROR;
          }
        } else {
          ret = MCCP_RESULT_ALREADY_EXISTS;
        }
      }
    unlock:
      s_wait_unlock(*thdptr);

    } else {
      ret = MCCP_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_thread_cancel(const mccp_thread_t *thdptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {
    if (getpid() == (*thdptr)->m_creator_pid) {
      if (s_is_thd(*thdptr) == true) {

        s_wait_lock(*thdptr);
        {
          if ((*thdptr)->m_is_activated == true) {

            s_cancel_lock(*thdptr);
            {
              if ((*thdptr)->m_is_canceled == false &&
                  (*thdptr)->m_pthd != MCCP_INVALID_THREAD) {
                int st;

                errno = 0;
                if ((st = pthread_cancel((*thdptr)->m_pthd)) == 0) {
                  ret = MCCP_RESULT_OK;
                } else {
                  errno = st;
                  ret = MCCP_RESULT_POSIX_API_ERROR;
                }
              } else {
                ret = MCCP_RESULT_OK;
              }
            }
            s_cancel_unlock(*thdptr);

          } else {
            ret = MCCP_RESULT_OK;
          }
        }
        s_wait_unlock(*thdptr);

        /*
         * Very here, very this moment the s_finalize() would start to
         * run.
         */

      } else {
        ret = MCCP_RESULT_INVALID_OBJECT;
      }
    } else {
      ret = MCCP_RESULT_NOT_OWNER;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_thread_wait(const mccp_thread_t *thdptr,
                 mccp_chrono_t nsec) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {
    if (getpid() == (*thdptr)->m_creator_pid) {
      if (s_is_thd(*thdptr) == true) {
        if ((*thdptr)->m_do_autodelete == false) {

          s_wait_lock(*thdptr);
          {
          waitcheck:
            if ((*thdptr)->m_is_activated == true) {
              ret = mccp_cond_wait(&((*thdptr)->m_wait_cond),
                                   &((*thdptr)->m_wait_lock),
                                   nsec);
              if (ret == MCCP_RESULT_OK) {
                goto waitcheck;
              }
            } else {
              ret = MCCP_RESULT_OK;
            }
          }
          s_wait_unlock(*thdptr);

        } else {
          ret = MCCP_RESULT_NOT_OPERATIONAL;
        }
      } else {
        ret = MCCP_RESULT_INVALID_OBJECT;
      }
    } else {
      ret = MCCP_RESULT_NOT_OWNER;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
mccp_thread_destroy(mccp_thread_t *thdptr) {
  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {

      (void)mccp_thread_cancel(thdptr);
      (void)mccp_thread_wait(thdptr, -1);

      s_wait_lock(*thdptr);
      {
        if ((*thdptr)->m_is_destroying == false) {
          (*thdptr)->m_is_destroying = true;
          if ((*thdptr)->m_freeup_proc != NULL) {
            (*thdptr)->m_freeup_proc(thdptr, (*thdptr)->m_arg);
          }
        }
      }
      s_wait_unlock(*thdptr);

      s_destroy(*thdptr);
      *thdptr = NULL;

    }
  }
}


mccp_result_t
mccp_thread_get_pthread_id(const mccp_thread_t *thdptr,
                           pthread_t *tidptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL &&
      tidptr != NULL) {

    *tidptr = MCCP_INVALID_THREAD;

    if (s_is_thd(*thdptr) == true) {

      s_wait_lock(*thdptr);
      {
        if ((*thdptr)->m_is_started == true) {
          *tidptr = (*thdptr)->m_pthd;
          if ((*thdptr)->m_pthd != MCCP_INVALID_THREAD) {
            ret = MCCP_RESULT_OK;
          } else {
            ret = MCCP_RESULT_ALREADY_HALTED;
          }
        } else {
          ret = MCCP_RESULT_NOT_STARTED;
        }
      }
      s_wait_unlock(*thdptr);

    } else {
      ret = MCCP_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_thread_set_result_code(const mccp_thread_t *thdptr,
                            mccp_result_t code) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {

      s_op_lock(*thdptr);
      {
        (*thdptr)->m_result_code = code;
        ret = MCCP_RESULT_OK;
      }
      s_op_unlock(*thdptr);

    } else {
      ret = MCCP_RESULT_INVALID_OBJECT;
    }

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_thread_get_result_code(const mccp_thread_t *thdptr,
                            mccp_result_t *codeptr,
                            mccp_chrono_t nsec) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL &&
      codeptr != NULL) {

    *codeptr = MCCP_RESULT_ANY_FAILURES;

    if (s_is_thd(*thdptr) == true) {
      if ((ret = mccp_thread_wait(thdptr, nsec)) == MCCP_RESULT_OK) {

        s_op_lock(*thdptr);
        {
          *codeptr = (*thdptr)->m_result_code;
          ret = MCCP_RESULT_OK;
        }
        s_op_unlock(*thdptr);

      }
    } else {
      ret = MCCP_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_thread_is_canceled(const mccp_thread_t *thdptr,
                        bool *retptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL &&
      retptr != NULL) {

    *retptr = false;

    if (s_is_thd(*thdptr) == true) {

      s_cancel_lock(*thdptr);
      {
        *retptr = (*thdptr)->m_is_canceled;
        ret = MCCP_RESULT_OK;
      }
      s_cancel_unlock(*thdptr);

    } else {
      ret = MCCP_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_thread_free_when_destroy(mccp_thread_t *thdptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {
      s_alloc_mark_thd(*thdptr);
      ret = MCCP_RESULT_OK;
    } else {
      ret = MCCP_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_thread_is_valid(const mccp_thread_t *thdptr,
                     bool *retptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL &&
      retptr != NULL) {

    if (s_is_thd(*thdptr) == true) {
      *retptr = true;
      ret = MCCP_RESULT_OK;
    } else {
      *retptr = false;
      ret = MCCP_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
mccp_thread_atfork_child(const mccp_thread_t *thdptr) {
  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {
      (void)mccp_mutex_reinitialize(&((*thdptr)->m_op_lock));
      (void)mccp_mutex_reinitialize(&((*thdptr)->m_wait_lock));
      (void)mccp_mutex_reinitialize(&((*thdptr)->m_cancel_lock));
    }
  }
}


void
mccp_thread_module_initialize(void) {
  s_init();
}


void
mccp_thread_module_finalize(void) {
  s_final();
}
