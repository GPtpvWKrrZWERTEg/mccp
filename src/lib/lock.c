#include <mccp/mccp.h>





struct mccp_mutex_record {
  pthread_mutex_t m_mtx;
  pid_t m_creator_pid;
  int m_prev_cancel_state;
};


struct mccp_rwlock_record {
  pthread_rwlock_t m_rwl;
  pid_t m_creator_pid;
  int m_prev_cancel_state;
};


struct mccp_cond_record {
  pthread_cond_t m_cond;
  pid_t m_creator_pid;
};


struct mccp_barrier_record {
  pthread_barrier_t m_barrier;
  pid_t m_creator_pid;
};


typedef int (*notify_proc_t)(pthread_cond_t *cnd);





static notify_proc_t notify_single = pthread_cond_signal;
static notify_proc_t notify_all = pthread_cond_broadcast;





mccp_result_t
mccp_mutex_create(mccp_mutex_t *mtxptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;
  mccp_mutex_t mtx = NULL;

  if (mtxptr != NULL) {
    *mtxptr = NULL;
    mtx = (mccp_mutex_t)malloc(sizeof(*mtx));
    if (mtx != NULL) {
      int st;
      errno = 0;
      if ((st = pthread_mutex_init(&(mtx->m_mtx), NULL)) == 0) {
        mtx->m_creator_pid = getpid();
        mtx->m_prev_cancel_state = -INT_MAX;
        *mtxptr = mtx;
        ret = MCCP_RESULT_OK;
      } else {
        errno = st;
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = MCCP_RESULT_NO_MEMORY;
    }
    if (ret != MCCP_RESULT_OK) {
      free((void *)mtx);
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
mccp_mutex_destroy(mccp_mutex_t *mtxptr) {
  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    if ((*mtxptr)->m_creator_pid == getpid()) {
      (void)pthread_mutex_destroy(&((*mtxptr)->m_mtx));
    }
    free((void *)*mtxptr);
    *mtxptr = NULL;
  }
}


mccp_result_t
mccp_mutex_reinitialize(mccp_mutex_t *mtxptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;

    errno = 0;
    if ((st = pthread_mutex_init(&((*mtxptr)->m_mtx), NULL)) == 0) {
      (*mtxptr)->m_prev_cancel_state = -INT_MAX;
      ret = MCCP_RESULT_OK;
    } else {
      errno = st;
      ret = MCCP_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_mutex_lock(mccp_mutex_t *mtxptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;
    if ((st = pthread_mutex_lock(&((*mtxptr)->m_mtx))) == 0) {
      (*mtxptr)->m_prev_cancel_state = -INT_MAX;
      ret = MCCP_RESULT_OK;
    } else {
      errno = st;
      ret = MCCP_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_mutex_trylock(mccp_mutex_t *mtxptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;
    if ((st = pthread_mutex_trylock(&((*mtxptr)->m_mtx))) == 0) {
      (*mtxptr)->m_prev_cancel_state = -INT_MAX;
      ret = MCCP_RESULT_OK;
    } else {
      errno = st;
      if (st == EBUSY) {
        ret = MCCP_RESULT_BUSY;
      } else {
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_mutex_timedlock(mccp_mutex_t *mtxptr,
                     mccp_chrono_t nsec) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;

    if (nsec < 0) {
      if ((st = pthread_mutex_lock(&((*mtxptr)->m_mtx))) == 0) {
        (*mtxptr)->m_prev_cancel_state = -INT_MAX;
        ret = MCCP_RESULT_OK;
      } else {
        errno = st;
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    } else {
      struct timespec ts;
      mccp_chrono_t now;

      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      now += nsec;
      NSEC_TO_TS(now, ts);

      if ((st = pthread_mutex_timedlock(&((*mtxptr)->m_mtx),
                                        &ts)) == 0) {
        (*mtxptr)->m_prev_cancel_state = -INT_MAX;
        ret = MCCP_RESULT_OK;
      } else {
        errno = st;
        if (st == ETIMEDOUT) {
          ret = MCCP_RESULT_TIMEDOUT;
        } else {
          ret = MCCP_RESULT_POSIX_API_ERROR;
        }
      }
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_mutex_unlock(mccp_mutex_t *mtxptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;

    /*
     * The caller must have this mutex locked.
     */
    if ((*mtxptr)->m_prev_cancel_state == -INT_MAX) {
      if ((st = pthread_mutex_unlock(&((*mtxptr)->m_mtx))) == 0) {
        ret = MCCP_RESULT_OK;
      } else {
        errno = st;
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    } else {
      /*
       * Someone (including the caller itself) locked the mutex with
       * mccp_mutex_enter_critical() API.
       */
      ret = MCCP_RESULT_CRITICAL_REGION_NOT_CLOSED;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_mutex_enter_critical(mccp_mutex_t *mtxptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;
    int oldstate;

    if ((st = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                     &oldstate)) == 0) {
      if ((st = pthread_mutex_lock(&((*mtxptr)->m_mtx))) == 0) {
        (*mtxptr)->m_prev_cancel_state = oldstate;
        ret = MCCP_RESULT_OK;
      } else {
        errno = st;
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    } else {
      errno = st;
      ret = MCCP_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_mutex_leave_critical(mccp_mutex_t *mtxptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;
    int oldstate;

    /*
     * The caller must have this mutex locked.
     */
    oldstate = (*mtxptr)->m_prev_cancel_state;
    if (oldstate == PTHREAD_CANCEL_ENABLE ||
        oldstate == PTHREAD_CANCEL_DISABLE) {
      /*
       * This mutex is locked via mccp_mutex_enter_critical().
       */
      if ((st = pthread_mutex_unlock(&((*mtxptr)->m_mtx))) == 0) {
        if ((st = pthread_setcancelstate(oldstate, NULL)) == 0) {
          ret = MCCP_RESULT_OK;
          pthread_testcancel();
        } else {
          errno = st;
          ret = MCCP_RESULT_POSIX_API_ERROR;
        }
      } else {
        errno = st;
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = MCCP_RESULT_CRITICAL_REGION_NOT_OPENED;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}





mccp_result_t
mccp_rwlock_create(mccp_rwlock_t *rwlptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;
  mccp_rwlock_t rwl = NULL;

  if (rwlptr != NULL) {
    *rwlptr = NULL;
    rwl = (mccp_rwlock_t)malloc(sizeof(*rwl));
    if (rwl != NULL) {
      int st;
      errno = 0;
      if ((st = pthread_rwlock_init(&(rwl->m_rwl), NULL)) == 0) {
        rwl->m_creator_pid = getpid();
        rwl->m_prev_cancel_state = -INT_MAX;
        *rwlptr = rwl;
        ret = MCCP_RESULT_OK;
      } else {
        errno = st;
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = MCCP_RESULT_NO_MEMORY;
    }
    if (ret != MCCP_RESULT_OK) {
      free((void *)rwl);
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
mccp_rwlock_destroy(mccp_rwlock_t *rwlptr) {
  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    if ((*rwlptr)->m_creator_pid == getpid()) {
      (void)pthread_rwlock_destroy(&((*rwlptr)->m_rwl));
    }
    free((void *)*rwlptr);
    *rwlptr = NULL;
  }
}


mccp_result_t
mccp_rwlock_reinitialize(mccp_rwlock_t *rwlptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;

    errno = 0;
    if ((st = pthread_rwlock_init(&((*rwlptr)->m_rwl), NULL)) == 0) {
      (*rwlptr)->m_prev_cancel_state = -INT_MAX;
      ret = MCCP_RESULT_OK;
    } else {
      errno = st;
      ret = MCCP_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_rwlock_reader_lock(mccp_rwlock_t *rwlptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    if ((st = pthread_rwlock_rdlock(&((*rwlptr)->m_rwl))) == 0) {
      (*rwlptr)->m_prev_cancel_state = -INT_MAX;
      ret = MCCP_RESULT_OK;
    } else {
      errno = st;
      ret = MCCP_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_rwlock_reader_trylock(mccp_rwlock_t *rwlptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    if ((st = pthread_rwlock_tryrdlock(&((*rwlptr)->m_rwl))) == 0) {
      (*rwlptr)->m_prev_cancel_state = -INT_MAX;
      ret = MCCP_RESULT_OK;
    } else {
      errno = st;
      if (st == EBUSY) {
        ret = MCCP_RESULT_BUSY;
      } else {
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_rwlock_reader_timedlock(mccp_rwlock_t *rwlptr,
                             mccp_chrono_t nsec) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;

    if (nsec < 0) {
      if ((st = pthread_rwlock_rdlock(&((*rwlptr)->m_rwl))) == 0) {
        (*rwlptr)->m_prev_cancel_state = -INT_MAX;
        ret = MCCP_RESULT_OK;
      } else {
        errno = st;
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    } else {
      struct timespec ts;
      mccp_chrono_t now;

      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      now += nsec;
      NSEC_TO_TS(now, ts);

      if ((st = pthread_rwlock_timedrdlock(&((*rwlptr)->m_rwl),
                                           &ts)) == 0) {
        (*rwlptr)->m_prev_cancel_state = -INT_MAX;
        ret = MCCP_RESULT_OK;
      } else {
        errno = st;
        if (st == ETIMEDOUT) {
          ret = MCCP_RESULT_TIMEDOUT;
        } else {
          ret = MCCP_RESULT_POSIX_API_ERROR;
        }
      }
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_rwlock_writer_lock(mccp_rwlock_t *rwlptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    if ((st = pthread_rwlock_wrlock(&((*rwlptr)->m_rwl))) == 0) {
      (*rwlptr)->m_prev_cancel_state = -INT_MAX;
      ret = MCCP_RESULT_OK;
    } else {
      errno = st;
      ret = MCCP_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_rwlock_writer_trylock(mccp_rwlock_t *rwlptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    if ((st = pthread_rwlock_trywrlock(&((*rwlptr)->m_rwl))) == 0) {
      (*rwlptr)->m_prev_cancel_state = -INT_MAX;
      ret = MCCP_RESULT_OK;
    } else {
      errno = st;
      if (st == EBUSY) {
        ret = MCCP_RESULT_BUSY;
      } else {
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_rwlock_writer_timedlock(mccp_rwlock_t *rwlptr,
                             mccp_chrono_t nsec) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;

    if (nsec < 0) {
      if ((st = pthread_rwlock_wrlock(&((*rwlptr)->m_rwl))) == 0) {
        (*rwlptr)->m_prev_cancel_state = -INT_MAX;
        ret = MCCP_RESULT_OK;
      } else {
        errno = st;
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    } else {
      struct timespec ts;
      mccp_chrono_t now;

      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      now += nsec;
      NSEC_TO_TS(now, ts);

      if ((st = pthread_rwlock_timedwrlock(&((*rwlptr)->m_rwl),
                                           &ts)) == 0) {
        (*rwlptr)->m_prev_cancel_state = -INT_MAX;
        ret = MCCP_RESULT_OK;
      } else {
        errno = st;
        if (st == ETIMEDOUT) {
          ret = MCCP_RESULT_TIMEDOUT;
        } else {
          ret = MCCP_RESULT_POSIX_API_ERROR;
        }
      }
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_rwlock_unlock(mccp_rwlock_t *rwlptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;

    /*
     * The caller must have this rwlock locked.
     */
    if ((*rwlptr)->m_prev_cancel_state == -INT_MAX) {
      if ((st = pthread_rwlock_unlock(&((*rwlptr)->m_rwl))) == 0) {
        ret = MCCP_RESULT_OK;
      } else {
        errno = st;
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    } else {
      /*
       * Someone (including the caller itself) locked the rwlock with
       * mccp_rwlock_enter_critical() API.
       */
      ret = MCCP_RESULT_CRITICAL_REGION_NOT_CLOSED;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_rwlock_reader_enter_critical(mccp_rwlock_t *rwlptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    int oldstate;

    if ((st = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                     &oldstate)) == 0) {
      if ((st = pthread_rwlock_rdlock(&((*rwlptr)->m_rwl))) == 0) {
        (*rwlptr)->m_prev_cancel_state = oldstate;
        ret = MCCP_RESULT_OK;
      } else {
        errno = st;
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    } else {
      errno = st;
      ret = MCCP_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_rwlock_writer_enter_critical(mccp_rwlock_t *rwlptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    int oldstate;

    if ((st = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                     &oldstate)) == 0) {
      if ((st = pthread_rwlock_wrlock(&((*rwlptr)->m_rwl))) == 0) {
        (*rwlptr)->m_prev_cancel_state = oldstate;
        ret = MCCP_RESULT_OK;
      } else {
        errno = st;
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    } else {
      errno = st;
      ret = MCCP_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_rwlock_leave_critical(mccp_rwlock_t *rwlptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    int oldstate;

    /*
     * The caller must have this rwlock locked.
     */
    oldstate = (*rwlptr)->m_prev_cancel_state;
    if (oldstate == PTHREAD_CANCEL_ENABLE ||
        oldstate == PTHREAD_CANCEL_DISABLE) {
      /*
       * This rwlock is locked via mccp_rwlock_enter_critical().
       */
      if ((st = pthread_rwlock_unlock(&((*rwlptr)->m_rwl))) == 0) {
        if ((st = pthread_setcancelstate(oldstate, NULL)) == 0) {
          ret = MCCP_RESULT_OK;
          pthread_testcancel();
        } else {
          errno = st;
          ret = MCCP_RESULT_POSIX_API_ERROR;
        }
      } else {
        errno = st;
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = MCCP_RESULT_CRITICAL_REGION_NOT_OPENED;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}





mccp_result_t
mccp_cond_create(mccp_cond_t *cndptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;
  mccp_cond_t cnd = NULL;

  if (cndptr != NULL) {
    *cndptr = NULL;
    cnd = (mccp_cond_t)malloc(sizeof(*cnd));
    if (cnd != NULL) {
      int st;
      errno = 0;
      if ((st = pthread_cond_init(&(cnd->m_cond), NULL)) == 0) {
        cnd->m_creator_pid = getpid();
        *cndptr = cnd;
        ret = MCCP_RESULT_OK;
      } else {
        errno = st;
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = MCCP_RESULT_NO_MEMORY;
    }
    if (ret != MCCP_RESULT_OK) {
      free((void *)cnd);
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
mccp_cond_destroy(mccp_cond_t *cndptr) {
  if (cndptr != NULL &&
      *cndptr != NULL) {
    if ((*cndptr)->m_creator_pid == getpid()) {
      (void)pthread_cond_destroy(&((*cndptr)->m_cond));
    }
    free((void *)*cndptr);
    *cndptr = NULL;
  }
}


mccp_result_t
mccp_cond_wait(mccp_cond_t *cndptr,
               mccp_mutex_t *mtxptr,
               mccp_chrono_t nsec) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL &&
      cndptr != NULL &&
      *cndptr != NULL) {
    int st;

    errno = 0;
    if (nsec < 0) {
      if ((st = pthread_cond_wait(&((*cndptr)->m_cond),
                                  &((*mtxptr)->m_mtx))) == 0) {
        ret = MCCP_RESULT_OK;
      } else {
        errno = st;
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    } else {
      struct timespec ts;
      mccp_chrono_t now;

      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      now += nsec;
      NSEC_TO_TS(now, ts);
    retry:
      errno = 0;
      if ((st = pthread_cond_timedwait(&((*cndptr)->m_cond),
                                       &((*mtxptr)->m_mtx),
                                       &ts)) == 0) {
        ret = MCCP_RESULT_OK;
      } else {
        if (st == EINTR) {
          goto retry;
        } else if (st == ETIMEDOUT) {
          ret = MCCP_RESULT_TIMEDOUT;
        } else {
          errno = st;
          ret = MCCP_RESULT_POSIX_API_ERROR;
        }
      }
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_cond_notify(mccp_cond_t *cndptr,
                 bool for_all) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (cndptr != NULL &&
      *cndptr != NULL) {
    int st;

    errno = 0;
    if ((st = ((for_all == true) ? notify_all : notify_single)(
                &((*cndptr)->m_cond))) == 0) {
      ret = MCCP_RESULT_OK;
    } else {
      errno = st;
      ret = MCCP_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}





mccp_result_t
mccp_barrier_create(mccp_barrier_t *bptr, size_t n) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;
  mccp_barrier_t b = NULL;

  if (bptr != NULL && n > 0) {
    *bptr = NULL;
    b = (mccp_barrier_t)malloc(sizeof(*b));
    if (b != NULL) {
      int st;
      errno = 0;
      if ((st = pthread_barrier_init(&(b->m_barrier), NULL,
                                     (unsigned)n)) == 0) {
        b->m_creator_pid = getpid();
        *bptr = b;
        ret = MCCP_RESULT_OK;
      } else {
        errno = st;
        ret = MCCP_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = MCCP_RESULT_NO_MEMORY;
    }
    if (ret != MCCP_RESULT_OK) {
      free((void *)b);
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
mccp_barrier_destroy(mccp_barrier_t *bptr) {
  if (bptr != NULL &&
      *bptr != NULL) {
    if ((*bptr)->m_creator_pid == getpid()) {
      (void)pthread_barrier_destroy(&((*bptr)->m_barrier));
    }
    free((void *)*bptr);
    *bptr = NULL;
  }
}


mccp_result_t
mccp_barrier_wait(mccp_barrier_t *bptr, bool *is_master) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (bptr != NULL &&
      *bptr != NULL) {
    mccp_barrier_t b = *bptr;
    int st;

    errno = 0;
    st = pthread_barrier_wait(&(b->m_barrier));
    if (st == 0 || st == PTHREAD_BARRIER_SERIAL_THREAD) {
      ret = MCCP_RESULT_OK;
      if (st == PTHREAD_BARRIER_SERIAL_THREAD && is_master != NULL) {
        *is_master = true;
      }
    } else {
      errno = st;
      ret = MCCP_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


