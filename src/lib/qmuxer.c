#include <mccp/mccp.h>

#include "qmuxer_types.h"
#include "qmuxer_internal.h"





static inline void
s_lock(mccp_qmuxer_t qmx) {
  if (qmx != NULL) {
    (void)mccp_mutex_lock(&(qmx->m_lock));
  }
}


static inline void
s_unlock(mccp_qmuxer_t qmx) {
  if (qmx != NULL) {
    (void)mccp_mutex_unlock(&(qmx->m_lock));
  }
}


static inline mccp_result_t
s_qmx_initialize(mccp_qmuxer_t qmx) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (qmx != NULL) {
    (void)memset((void *)qmx, 0, sizeof(*qmx));
    if (((ret = mccp_mutex_create(&(qmx->m_lock))) ==
         MCCP_RESULT_OK) &&
        ((ret = mccp_cond_create(&(qmx->m_cond))) ==
         MCCP_RESULT_OK)) {
      ;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_qmx_destroy(mccp_qmuxer_t qmx) {
  if (qmx != NULL) {

    if (qmx->m_lock != NULL) {

      s_lock(qmx);
      {
        if (qmx->m_cond != NULL) {
          (void)mccp_cond_destroy(&(qmx->m_cond));
        }
      }
      s_unlock(qmx);

      (void)mccp_mutex_destroy(&(qmx->m_lock));
    }

    free((void *)qmx);
  }
}


static inline mccp_result_t
s_qmx_setup_for_poll(mccp_qmuxer_t *qmxptr,
                     mccp_qmuxer_poll_t const polls[],
                     size_t npolls,
                     bool is_pre) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (qmxptr != NULL &&
      *qmxptr != NULL &&
      polls != NULL &&
      npolls > 0) {
    mccp_result_t st;
    const mccp_qmuxer_poll_t *mpptr;
    size_t i;
    size_t n_nulls = 0;
    ssize_t nev = 0;

    for (i = 0; i < npolls; i++) {
      mpptr = &(polls[i]);

      if (mpptr != NULL &&
          *mpptr != NULL) {
        if ((*mpptr)->m_bbq != NULL) {
          st = cbuffer_setup_for_qmuxer((*mpptr)->m_bbq,
                                        *qmxptr,
                                        &((*mpptr)->m_q_size),
                                        &((*mpptr)->m_q_rem_capacity),
                                        (*mpptr)->m_type,
                                        is_pre);
          /*
           * Note:
           *	st < 0:		Any mccp API error(s).
           *	st == 0:	The queue is readable/writable.
           *	st == 1:	Need to wait for reaable.
           *	st == 2:	Need to wait for writable.
           *	st == 3:	Need to wait for both readable and writable.
           *			So this must not happen and the
           *			function guarantees it.
           */
          if (st == 0) {
            /*
             * Count up # of poll objects having events at this moment.
             */
            nev++;
          }
        } else {
          /*
           * Count up # of poll objects having a NULL queue.
           */
          n_nulls++;
          /*
           * And compensation for not calling the
           * cbuffer_setup_for_qmuxer().
           */
          st = 0;
          (*mpptr)->m_q_size = 0;
          (*mpptr)->m_q_rem_capacity = 0;
        }
        if (is_pre == true && st < 0) {
          ret = st;
          goto done;
        }
      } else {
        ret = MCCP_RESULT_INVALID_ARGS;
        if (is_pre == true) {
          goto done;
        }
      }
    }

    if (n_nulls == npolls) {
      /*
       * All the poll objects are havinig a NULL bbq. What a shame.
       */
      ret = MCCP_RESULT_INVALID_ARGS;
    } else {
      ret = nev;
    }

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}





mccp_result_t
mccp_qmuxer_create(mccp_qmuxer_t *qmxptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (qmxptr != NULL) {
    *qmxptr = (mccp_qmuxer_t)malloc(sizeof(**qmxptr));
    if (*qmxptr == NULL) {
      ret = MCCP_RESULT_NO_MEMORY;
      goto done;
    }
    ret = s_qmx_initialize(*qmxptr);
    if (ret != MCCP_RESULT_OK) {
      s_qmx_destroy(*qmxptr);
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}


void
mccp_qmuxer_destroy(mccp_qmuxer_t *qmxptr) {
  if (qmxptr != NULL) {
    s_qmx_destroy(*qmxptr);
  }
}





mccp_result_t
mccp_qmuxer_poll(mccp_qmuxer_t *qmxptr,
                 mccp_qmuxer_poll_t const polls[],
                 size_t npolls,
                 mccp_chrono_t nsec) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (qmxptr != NULL &&
      *qmxptr != NULL &&
      polls != NULL &&
      npolls > 0) {

  recheck:
    /*
     * Setup the polls for pre-wait.
     */
    ret = s_qmx_setup_for_poll(qmxptr, polls, npolls, true);
    if (ret == 0) {

      /*
       * Need to wait.
       *
       * Note:
       *	As you see, locking the muxer itself is almost
       *	meaningless. If we don't need to concern about the
       *	performance, we want to lock the qmuxer just like
       *	using a giant lock among all the queues in the
       *	polls. Only reason to lock the *qmxptr is because
       *	pthread_cond_wait() needs it, at least at this
       *	moment. We would change this if the qmuxer mechanism
       *	seems doropping events regular basis and if we realize
       *	that catching events is more important than the
       *	overall performance.
       */
      s_lock(*qmxptr);
      {
        ret = mccp_cond_wait(&((*qmxptr)->m_cond),
                             &((*qmxptr)->m_lock),
                             nsec);
      }
      s_unlock(*qmxptr);

      if (ret == MCCP_RESULT_OK) {
        /*
         * Check if any events occur on all bbq/polls.
         */
        ret = s_qmx_setup_for_poll(qmxptr, polls, npolls, false);
        if (ret != 0) {
          /*
           * ret > 0 ... Having at least a queue that is ready for
           *		   play with.
           * ret < 0 ... An error.
           *
           *	no matter what the ret is we have to return anyway.
           */
          goto done;
        } else {
          /*
           * What happens here:
           *
           *	We are just awakened even the queues have no
           *	events (means ret == 0). Why ?:
           *
           *		1) We are running on a operating system other
           *		than the Linux since the Linux guarantees that
           *		the sleepers in pthread_cond_wait(3) are
           *		awakened ONLY by calling the
           *		pthread_cond_broadcast(3)/pthread_cond_signal(3),
           *		no system dependent events wakes the sleepers.
           *
           *		2) Most likely, any other threads just steal
           *		the events already. If this occured, maybe you
           *		better think why and rewrite your code.
           *
           *	Anyway, we have to start this all over again.
           */
          goto recheck;
        }
      }
      /*
       * else {
       *	Any errors occur. Just return.
       * }
       */
    }
    /*
     * else {
     *	We already have at least a queue having events, or, an
     *	error occurs. Either ways Just return.
     * }
     */

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}





void
qmuxer_notify(mccp_qmuxer_t qmx) {
  if (qmx != NULL) {

    s_lock(qmx);
    {
      (void)mccp_cond_notify(&(qmx->m_cond), true);
    }
    s_unlock(qmx);

  }
}


void
mccp_qmuxer_cancel_janitor(mccp_qmuxer_t *qmxptr) {
  if (qmxptr != NULL &&
      *qmxptr != NULL) {
    s_unlock(*qmxptr);
  }
}
