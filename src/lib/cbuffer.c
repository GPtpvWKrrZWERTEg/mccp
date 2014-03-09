#include <mccp/mccp.h>
#include "qmuxer_internal.h"





#define N_EMPTY_ROOM	1LL





typedef struct mccp_cbuffer_record {
  mccp_mutex_t m_lock;
  mccp_cond_t m_cond_put;
  mccp_cond_t m_cond_get;

  volatile int64_t m_r_idx;
  volatile int64_t m_w_idx;
  volatile int64_t m_n_elements;

  volatile bool m_is_operational;

  mccp_cbuffer_value_freeup_proc_t m_del_proc;

  size_t m_element_size;

  int64_t m_n_max_elements;
  int64_t m_n_max_allocd_elements;

  mccp_qmuxer_t m_qmuxer;
  mccp_qmuxer_poll_event_t m_type;

  char m_data[0];
} mccp_cbuffer_record;





static inline void
s_adjust_indices(mccp_cbuffer_t cb) {
  if (cb != NULL) {
    if (cb->m_w_idx >= 0) {
      /*
       * This might improve the branch prediction performance.
       */
      return;
    } else {
      /*
       * Note that this could happen like once in a handred years
       * even with a 3.0 GHz CPU executing value insertion operations
       * EACH AND EVERY clock, but, prepare for THE CASE.
       */
      cb->m_w_idx = cb->m_n_max_allocd_elements;
      cb->m_r_idx = cb->m_r_idx % cb->m_n_max_allocd_elements;
    }
  }
}


static inline void
s_lock(mccp_cbuffer_t cb) {
  if (cb != NULL) {
    (void)mccp_mutex_lock(&(cb->m_lock));
  }
}


static inline void
s_unlock(mccp_cbuffer_t cb) {
  if (cb != NULL) {
    (void)mccp_mutex_unlock(&(cb->m_lock));
  }
}


static inline char *
s_data_addr(mccp_cbuffer_t cb, int64_t idx) {
  if (cb != NULL && idx >= 0) {
    return
      cb->m_data +
      (idx % cb->m_n_max_allocd_elements) * (int64_t)cb->m_element_size;
  } else {
    return NULL;
  }
}


static inline void
s_freeup_all_values(mccp_cbuffer_t cb) {
  if (cb != NULL) {
    if (cb->m_del_proc != NULL) {
      int64_t i;
      char *addr;

      s_adjust_indices(cb);
      for (i = cb->m_r_idx;
           i < cb->m_w_idx;
           i++) {
        addr = s_data_addr(cb, i);
        if (addr != NULL) {
          cb->m_del_proc((void **)addr);
        }
      }
    }
  }
}


static inline void
s_clean(mccp_cbuffer_t cb, bool free_values) {
  if (cb != NULL) {
    if (free_values == true) {
      s_freeup_all_values(cb);
    }
    cb->m_r_idx = 0;
    cb->m_w_idx = 0;
    (void)memset((void *)(cb->m_data), 0,
                 cb->m_element_size * (size_t)cb->m_n_max_allocd_elements);
    cb->m_n_elements = 0;
  }
}


static inline void
s_shutdown(mccp_cbuffer_t cb, bool free_values) {
  if (cb != NULL) {
    if (cb->m_is_operational == true) {
      cb->m_is_operational = false;
      s_clean(cb, free_values);
      if (cb->m_qmuxer != NULL) {
        qmuxer_notify(cb->m_qmuxer);
      }
      (void)mccp_cond_notify(&(cb->m_cond_get), true);
      (void)mccp_cond_notify(&(cb->m_cond_put), true);
    }
  }
}





mccp_result_t
mccp_cbuffer_create_with_size(mccp_cbuffer_t *cbptr,
                              size_t elemsize,
                              int64_t maxelems,
                              mccp_cbuffer_value_freeup_proc_t proc) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (cbptr != NULL &&
      elemsize > 0 &&
      maxelems > 0) {
    mccp_cbuffer_t cb =
      (mccp_cbuffer_t)malloc(sizeof(*cb) +
                             elemsize * (size_t)(maxelems + N_EMPTY_ROOM));

    *cbptr = NULL;

    if (cb != NULL) {
      if (((ret = mccp_mutex_create(&(cb->m_lock))) ==
           MCCP_RESULT_OK) &&
          ((ret = mccp_cond_create(&(cb->m_cond_put))) ==
           MCCP_RESULT_OK) &&
          ((ret = mccp_cond_create(&(cb->m_cond_get))) ==
           MCCP_RESULT_OK)) {
        cb->m_r_idx = 0;
        cb->m_w_idx = 0;
        cb->m_n_elements = 0;
        cb->m_n_max_elements = maxelems;
        cb->m_n_max_allocd_elements = maxelems + N_EMPTY_ROOM;
        cb->m_element_size = elemsize;
        cb->m_del_proc = proc;
        cb->m_is_operational = true;
        cb->m_qmuxer = NULL;
        cb->m_type = MCCP_QMUXER_POLL_UNKNOWN;

        *cbptr = cb;

        ret = MCCP_RESULT_OK;

      } else {
        free((void *)cb);
      }
    } else {
      ret = MCCP_RESULT_NO_MEMORY;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
mccp_cbuffer_shutdown(mccp_cbuffer_t *cbptr,
                      bool free_values) {
  if (cbptr != NULL &&
      *cbptr != NULL) {

    s_lock(*cbptr);
    {
      s_shutdown(*cbptr, free_values);
    }
    s_unlock(*cbptr);

  }
}


void
mccp_cbuffer_destroy(mccp_cbuffer_t *cbptr,
                     bool free_values) {
  if (cbptr != NULL &&
      *cbptr != NULL) {

    s_lock(*cbptr);
    {
      s_shutdown(*cbptr, free_values);
      mccp_cond_destroy(&((*cbptr)->m_cond_put));
      mccp_cond_destroy(&((*cbptr)->m_cond_get));
    }
    s_unlock(*cbptr);

    mccp_mutex_destroy(&((*cbptr)->m_lock));

    free((void *)*cbptr);
    *cbptr = NULL;
  }
}


mccp_result_t
mccp_cbuffer_clear(mccp_cbuffer_t *cbptr,
                   bool free_values) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (cbptr != NULL &&
      *cbptr != NULL) {

    s_lock(*cbptr);
    {
      s_clean(*cbptr, free_values);
      if ((*cbptr)->m_qmuxer != NULL &&
          NEED_WAIT_READABLE((*cbptr)->m_type) == true) {
        qmuxer_notify((*cbptr)->m_qmuxer);
      }
      (void)mccp_cond_notify(&((*cbptr)->m_cond_put), true);
    }
    s_unlock(*cbptr);

    ret = MCCP_RESULT_OK;

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}





mccp_result_t
mccp_cbuffer_put_with_size(mccp_cbuffer_t *cbptr,
                           void **valptr,
                           size_t valsz,
                           mccp_chrono_t nsec) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (cbptr != NULL &&
      *cbptr != NULL &&
      valptr != NULL &&
      valsz == (*cbptr)->m_element_size) {

    s_lock(*cbptr);
    {
    recheck:
      if ((*cbptr)->m_is_operational == true) {

        s_adjust_indices(*cbptr);

        if ((*cbptr)->m_n_elements < (*cbptr)->m_n_max_elements) {
          char *dstptr = s_data_addr(*cbptr, (*cbptr)->m_w_idx);
          char *srcptr = (char *)valptr;

          if (dstptr != NULL) {
            /*
             * Copy the value.
             */
            (void)memcpy((void *)dstptr, (void *)srcptr, valsz);
            (*cbptr)->m_w_idx++;
            (*cbptr)->m_n_elements++;
            /*
             * And wake all the get waiters.
             */
            if ((*cbptr)->m_qmuxer != NULL &&
                NEED_WAIT_READABLE((*cbptr)->m_type) == true) {
              qmuxer_notify((*cbptr)->m_qmuxer);
            }
            (void)mccp_cond_notify(&((*cbptr)->m_cond_get), true);

            ret = MCCP_RESULT_OK;

          } else {
            /*
             * Must not happen.
             */
            mccp_exit_fatal("Circular buffer write pointer error.\n");
          }
        } else {
          /*
           * The buffer is full. Wait until someone get.
           */
          if ((ret = mccp_cond_wait(&((*cbptr)->m_cond_put),
                                    &((*cbptr)->m_lock), nsec)) ==
              MCCP_RESULT_OK) {
            goto recheck;
          }
        }
      } else {
        ret = MCCP_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*cbptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_cbuffer_get_with_size(mccp_cbuffer_t *cbptr,
                           void **valptr,
                           size_t valsz,
                           mccp_chrono_t nsec) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (cbptr != NULL &&
      *cbptr != NULL &&
      valptr != NULL &&
      valsz == (*cbptr)->m_element_size) {

    s_lock(*cbptr);
    {
    recheck:
      if ((*cbptr)->m_is_operational == true) {

        s_adjust_indices(*cbptr);

        if ((*cbptr)->m_n_elements > 0) {
          char *dstptr = (char *)valptr;
          char *srcptr = s_data_addr(*cbptr, (*cbptr)->m_r_idx);

          if (srcptr != NULL) {
            /*
             * Copy the value.
             */
            (void)memcpy((void *)dstptr, (void *)srcptr, valsz);
            (*cbptr)->m_r_idx++;
            (*cbptr)->m_n_elements--;
            /*
             * And wake all the put waiters.
             */
            if ((*cbptr)->m_qmuxer != NULL &&
                NEED_WAIT_WRITABLE((*cbptr)->m_type) == true) {
              qmuxer_notify((*cbptr)->m_qmuxer);
            }
            (void)mccp_cond_notify(&((*cbptr)->m_cond_put), true);

            ret = MCCP_RESULT_OK;

          } else {
            /*
             * Must not happen.
             */
            mccp_exit_fatal("Circular buffer read pointer error.\n");
          }
        } else {
          /*
           * The buffer is empty. Wait until someone put.
           */
          if ((ret = mccp_cond_wait(&((*cbptr)->m_cond_get),
                                    &((*cbptr)->m_lock), nsec)) ==
              MCCP_RESULT_OK) {
            goto recheck;
          }
        }
      } else {
        ret = MCCP_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*cbptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_cbuffer_peek_with_size(mccp_cbuffer_t *cbptr,
                            void **valptr,
                            size_t valsz,
                            mccp_chrono_t nsec) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (cbptr != NULL &&
      *cbptr != NULL &&
      valptr != NULL &&
      valsz == (*cbptr)->m_element_size) {

    s_lock(*cbptr);
    {
    recheck:
      if ((*cbptr)->m_is_operational == true) {

        s_adjust_indices(*cbptr);

        if ((*cbptr)->m_n_elements > 0) {
          char *dstptr = (char *)valptr;
          char *srcptr = s_data_addr(*cbptr, (*cbptr)->m_r_idx);

          if (srcptr != NULL) {
            /*
             * Copy the value. Don't disturb others, just snoop the value.
             */
            (void)memcpy((void *)dstptr, (void *)srcptr, valsz);

            ret = MCCP_RESULT_OK;

          } else {
            /*
             * Must not happen.
             */
            mccp_exit_fatal("Circular buffer read pointer error.\n");
          }
        } else {
          /*
           * The buffer is empty. Wait until someone put.
           */
          if ((ret = mccp_cond_wait(&((*cbptr)->m_cond_get),
                                    &((*cbptr)->m_lock), nsec)) ==
              MCCP_RESULT_OK) {
            goto recheck;
          }
        }
      } else {
        ret = MCCP_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*cbptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}





mccp_result_t
mccp_cbuffer_size(mccp_cbuffer_t *cbptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (cbptr != NULL &&
      *cbptr != NULL) {

    s_lock(*cbptr);
    {
      if ((*cbptr)->m_is_operational == true) {
        ret = (*cbptr)->m_n_elements;
      } else {
        ret = MCCP_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*cbptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_cbuffer_remaining_capacity(mccp_cbuffer_t *cbptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (cbptr != NULL &&
      *cbptr != NULL) {

    s_lock(*cbptr);
    {
      if ((*cbptr)->m_is_operational == true) {
        ret = (*cbptr)->m_n_max_elements - (*cbptr)->m_n_elements;
      } else {
        ret = MCCP_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*cbptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_cbuffer_max_capacity(mccp_cbuffer_t *cbptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (cbptr != NULL &&
      *cbptr != NULL) {

    s_lock(*cbptr);
    {
      if ((*cbptr)->m_is_operational == true) {
        ret = (*cbptr)->m_n_max_elements;
      } else {
        ret = MCCP_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*cbptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_cbuffer_is_full(mccp_cbuffer_t *cbptr, bool *retptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (cbptr != NULL &&
      *cbptr != NULL &&
      retptr != NULL) {
    *retptr = false;

    s_lock(*cbptr);
    {
      if ((*cbptr)->m_is_operational == true) {
        *retptr = ((*cbptr)->m_n_elements >= (*cbptr)->m_n_max_elements) ?
                  true : false;
        ret = MCCP_RESULT_OK;
      } else {
        ret = MCCP_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*cbptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_cbuffer_is_empty(mccp_cbuffer_t *cbptr, bool *retptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (cbptr != NULL &&
      *cbptr != NULL &&
      retptr != NULL) {
    *retptr = false;

    s_lock(*cbptr);
    {
      if ((*cbptr)->m_is_operational == true) {
        *retptr = ((*cbptr)->m_n_elements == 0) ? true : false;
        ret = MCCP_RESULT_OK;
      } else {
        ret = MCCP_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*cbptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_cbuffer_is_operational(mccp_cbuffer_t *cbptr, bool *retptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (cbptr != NULL &&
      *cbptr != NULL &&
      retptr != NULL) {
    *retptr = false;

    s_lock(*cbptr);
    {
      *retptr = (*cbptr)->m_is_operational;
      ret = MCCP_RESULT_OK;
    }
    s_unlock(*cbptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
mccp_cbuffer_cancel_janitor(mccp_cbuffer_t *cbptr) {
  if (cbptr != NULL &&
      *cbptr != NULL) {
    s_unlock(*cbptr);
  }
}


mccp_result_t
cbuffer_setup_for_qmuxer(mccp_cbuffer_t cb,
                         mccp_qmuxer_t qmx,
                         ssize_t *szptr,
                         ssize_t *remptr,
                         mccp_qmuxer_poll_event_t type,
                         bool is_pre) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (cb != NULL &&
      qmx != NULL &&
      szptr != NULL &&
      remptr != NULL) {

    s_lock(cb);
    {
      if (cb->m_is_operational == true) {
        *szptr = cb->m_n_elements;
        *remptr = cb->m_n_max_elements - cb->m_n_elements;

        ret = 0;
        /*
         * Note that a case; (*szptr == 0 && *remptr == 0) never happen.
         */
        if (NEED_WAIT_READABLE(type) == true && *szptr == 0) {
          /*
           * Current queue size equals to zero so we need to wait for
           * readable.
           */
          ret = (mccp_result_t)MCCP_QMUXER_POLL_READABLE;
        } else if (NEED_WAIT_WRITABLE(type) == true && *remptr == 0) {
          /*
           * Current remaining capacity is zero so we need to wait for
           * writable.
           */
          ret = (mccp_result_t)MCCP_QMUXER_POLL_WRITABLE;
        }

        if (is_pre == true && ret > 0) {
          cb->m_qmuxer = qmx;
          cb->m_type = ret;
        } else {
          /*
           * We need this since the qmx could be not available when the
           * next event occurs.
           */
          cb->m_qmuxer = NULL;
          cb->m_type = 0;
        }

      } else {
        ret = MCCP_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(cb);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}

