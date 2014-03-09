#include <mccp/mccp.h>
#include <mccp/mccp_thread_internal.h>

#include <math.h>





typedef struct {
  mccp_thread_record m_thd;	/* must be on the head. */

  mccp_mutex_t m_start_lock;

  mccp_bbq_t m_q;
  ssize_t m_n_puts;
} test_thread_record;
typedef test_thread_record *test_thread_t;





static void
s_test_thread_finalize(const mccp_thread_t *tptr, bool is_canceled,
                       void *arg) {
  (void)tptr;
  (void)arg;

  mccp_msg_debug(1, "called. %s.\n",
                 (is_canceled == false) ? "finished" : "canceled");
}


static void
s_test_thread_destroy(const mccp_thread_t *tptr, void *arg) {
  (void)tptr;
  (void)arg;

  mccp_msg_debug(1, "called.\n");
}


static mccp_result_t
s_test_thread_main(const mccp_thread_t *tptr, void *arg) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  (void)arg;

  if (tptr != NULL) {
    test_thread_t tt = (test_thread_t)*tptr;

    if (tt != NULL) {
      ssize_t i;
      mccp_chrono_t now;

      /*
       * Ready, steady,
       */
      (void)mccp_mutex_lock(&(tt->m_start_lock));
      /*
       * go.
       */
      (void)mccp_mutex_unlock(&(tt->m_start_lock));

      for (i = 0; i < tt->m_n_puts; i++) {
        WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
        if (mccp_bbq_put(&(tt->m_q), &now, mccp_chrono_t, -1LL) !=
            MCCP_RESULT_OK) {
          goto done;
        }
      }
      now = -1LL;
      if (mccp_bbq_put(&(tt->m_q), &now, mccp_chrono_t, -1LL) !=
          MCCP_RESULT_OK) {
        goto done;
      }
      ret = i;
      mccp_msg_debug(1, "Done, ret = " PFSZS(020, d)
                     ", req = " PFSZS(020, u) ".\n",
                     (size_t)ret, (size_t)tt->m_n_puts);
    }
  }

done:
  return ret;
}


static inline bool
s_initialize(test_thread_t tt,
             mccp_mutex_t start_lock,
             mccp_bbq_t bbq,
             ssize_t n,
             const char *name) {
  bool ret = false;

  if (tt != NULL) {
    mccp_result_t r;
    if ((r = mccp_thread_create((mccp_thread_t *)&tt,
                                s_test_thread_main,
                                s_test_thread_finalize,
                                s_test_thread_destroy,
                                name, NULL)) != MCCP_RESULT_OK) {
      mccp_perror(r, "mccp_thread_create()");
      goto done;
    }
    tt->m_start_lock = start_lock;
    tt->m_q = bbq;
    tt->m_n_puts = n;
    ret = true;
  }
done:
  return ret;
}


static inline bool
test_thread_create(test_thread_t *ttptr,
                   mccp_mutex_t start_lock,
                   mccp_bbq_t bbq,
                   ssize_t n,
                   const char *name) {
  bool ret = false;

  if (ttptr != NULL) {
    test_thread_t tt;
    if (*ttptr == NULL) {
      tt = (test_thread_t)malloc(sizeof(*tt));
      if (tt == NULL) {
        goto done;
      }
    } else {
      tt = *ttptr;
    }
    ret = s_initialize(tt, start_lock, bbq, n, name);
    if (*ttptr == NULL) {
      *ttptr = tt;
    }
  }

done:
  return ret;
}





static int
do_run(size_t nthds, ssize_t nputs) {
  int ret = 1;

  mccp_result_t r;

  size_t i;
  size_t j;
  char thdname[16];

  mccp_mutex_t start_lock = NULL;
  test_thread_record *tts = NULL;
  size_t n_created_thds = 0;
  mccp_bbq_t *bbqs = NULL;
  size_t n_created_bbqs = 0;
  mccp_qmuxer_poll_t *polls = NULL;
  size_t n_created_polls = 0;
  mccp_qmuxer_t qmx = NULL;

  size_t n_need_watch = 0;
  size_t n_valid_polls = 0;
  ssize_t qsz;

  test_thread_t tt;
  mccp_bbq_t bbq;

  mccp_chrono_t put_date;

  mccp_chrono_t t_begin;
  mccp_chrono_t t_end;

  mccp_chrono_t p_begin;
  mccp_chrono_t p_t;

  ssize_t n_gets = 0;
  mccp_chrono_t p_min = LLONG_MAX;
  mccp_chrono_t p_max = LLONG_MIN;
  double p_sum = 0.0;
  double p_sum2 = 0.0;

  double p_avg;
  double p_sd;

  mccp_chrono_t t_total = 0;
  double t_avg;

  if ((r = mccp_mutex_create(&start_lock)) != MCCP_RESULT_OK) {
    mccp_perror(r, "mccp_mutex_create()");
    goto done;
  }

  /*
   * Create the qmuxer.
   */
  if ((r = mccp_qmuxer_create(&qmx)) != MCCP_RESULT_OK) {
    mccp_perror(r, "mccp_qmuxer_create()");
    goto done;
  }

  /*
   * Then create queues.
   */
  bbqs = (mccp_bbq_t *)malloc(sizeof(mccp_bbq_t) * nthds);
  if (bbqs == NULL) {
    goto done;
  }
  for (i = 0; i < nthds; i++) {
    if ((r = mccp_bbq_create(&(bbqs[i]), mccp_chrono_t,
                             1000LL * 1000LL,
                             NULL)) != MCCP_RESULT_OK) {
      mccp_perror(r, "mccp_bbq_create()");
      goto done;
    }
    n_created_bbqs++;
  }
  if (n_created_bbqs == 0) {
    goto done;
  }

  /*
   * Then create poll objects for the each queue.
   */
  polls = (mccp_qmuxer_poll_t *)malloc(sizeof(mccp_qmuxer_poll_t) *
                                       n_created_bbqs);
  if (polls == NULL) {
    goto done;
  }
  for (i = 0; i < n_created_bbqs; i++) {
    if ((r = mccp_qmuxer_poll_create(&(polls[i]),
                                     bbqs[i],
                                     MCCP_QMUXER_POLL_READABLE)) !=
        MCCP_RESULT_OK) {
      mccp_perror(r, "mccp_qmuxer_poll_create()");
      goto done;
    }
    n_created_polls++;
  }
  if (n_created_polls == 0) {
    goto done;
  }

  /*
   * Then create threads for the each poll objects/queues.
   */
  tts = (test_thread_record *)malloc(sizeof(test_thread_record) *
                                     n_created_polls);
  if (tts == NULL) {
    goto done;
  }
  for (i = 0; i < n_created_polls; i++) {
    snprintf(thdname, sizeof(thdname), "putter " PFSZS(4, u), i);
    tt = &(tts[i]);
    if (test_thread_create(&tt, start_lock, bbqs[i], nputs,
                           (const char *)thdname) != true) {
      goto done;
    }
    n_created_thds++;
  }
  if (n_created_thds == 0) {
    goto done;
  }

  /*
   * Let the initiation begin.
   */

  /*
   * Ready, note that all the created threads do this lock.
   */
  (void)mccp_mutex_lock(&start_lock);

  /*
   * Steady,
   */
  for (i = 0; i < n_created_thds; i++) {
    tt = &(tts[i]);
    if (mccp_thread_start((mccp_thread_t *)&tt, false) !=
        MCCP_RESULT_OK) {
      (void)mccp_mutex_unlock(&start_lock);
      goto done;
    }
  }

  fprintf(stdout, "Test for " PFSZ(u) " threads " PFSZ(u)
          " events/thdread start.\n", n_created_thds, (size_t)nputs);

  /*
   * Go.
   */
  (void)mccp_mutex_unlock(&start_lock);

  WHAT_TIME_IS_IT_NOW_IN_NSEC(t_begin);

  while (true) {
    /*
     * Like the select(2)/poll(2), initialize poll objects before
     * checking events.
     */
    n_need_watch = 0;
    n_valid_polls = 0;
    for (i = 0; i < n_created_thds; i++) {
      /*
       * Check if the poll has a valid queue.
       */
      bbq = NULL;
      if ((r = mccp_qmuxer_poll_get_queue(&(polls[i]), &bbq)) !=
          MCCP_RESULT_OK) {
        mccp_perror(r, "mccp_qmuxer_poll_get_queue()");
        break;
      }
      if (bbq != NULL) {
        n_valid_polls++;
      }

      /*
       * Reset the poll status.
       */
      if ((r = mccp_qmuxer_poll_reset(&(polls[i]))) != MCCP_RESULT_OK) {
        mccp_perror(r, "mccp_qmuxer_poll_reset()");
        break;
      }
      n_need_watch++;
    }

    /*
     * If there are no valid queues, exit.
     */
    if (n_valid_polls == 0) {
      break;
    }

    /*
     * Wait for an event.
     *
     *	Note that we better set timeout, not waiting forever.
     */
    r = mccp_qmuxer_poll(&qmx, (mccp_qmuxer_poll_t * const)polls,
                         n_need_watch,
                         100LL * 1000LL * 1000LL);

    if (r > 0) {
      /*
       * Check which poll got an event. Actually, check all the queues
       * in this sample.
       */
      WHAT_TIME_IS_IT_NOW_IN_NSEC(p_begin);

      for (i = 0; i < n_created_thds; i++) {

        if ((qsz = mccp_bbq_size(&(bbqs[i]))) > 0) {

          //qsz = (qsz <= 10000LL) ? qsz : 10000LL;

          mccp_msg_debug(1, "Got " PFSZS(8, u) " events from the Q"
                         PFSZS(03, u) ".\n",
                         (size_t)qsz, (size_t)i);

          for (j = 0; j < (size_t)qsz; j++) {
            if ((r = mccp_bbq_get(&(bbqs[i]), &put_date,
                                  mccp_chrono_t, -1LL)) ==
                MCCP_RESULT_OK) {
              /*
               * In this sample, -1LL is kinda 'EOF'. Check if we got
               * the EOF.
               */
              if (put_date == -1LL) {
                /*
                 * The queue is kinda 'closed'. From now on we don't
                 * check this queue anymore. To specify this:
                 */
                mccp_msg_debug(1, "Got an EOF from the Q" PFSZS(04, u)
                               ".\n", i);
                goto nullify;
              }

              p_t = p_begin - put_date;

              if (p_t < p_min) {
                p_min = p_t;
              }
              if (p_t > p_max) {
                p_max = p_t;
              }
              p_sum += (double)p_t;
              p_sum2 += ((double)p_t * (double)p_t);
              n_gets++;

            } else {
              /*
               * Something wrong for the queue. But we must not exit
               * here. Keep on checking other queues. In order to do
               * this, set NULL as the queue into the poll object for
               * the queue.
               */
              mccp_perror(r, "mccp_bbq_get()");
            nullify:
              if ((r = mccp_qmuxer_poll_set_queue(&(polls[i]), NULL)) !=
                  MCCP_RESULT_OK) {
                /*
                 * There is nothing we can do now.
                 */
                mccp_perror(r, "mccp_qmuxer_poll_set_queue()");
                goto done;
              }
            }
          }

        }

      }

    } else if (r == MCCP_RESULT_TIMEDOUT) {
      mccp_msg_debug(1, "Timedout. continue.\n");
      continue;
    } else {
      mccp_perror(r, "mccp_qmuxer_poll()");
      mccp_msg_debug(1, "Break the loop due to error(s).\n");
      goto done;
    }
  }

  WHAT_TIME_IS_IT_NOW_IN_NSEC(t_end);

  fprintf(stdout, "Done.\n");

  fprintf(stdout, "Total # of the events:\t" PFSZS(22, u) "\n\n", n_gets);

  p_avg = p_sum / (double)n_gets;
  p_sd = (p_sum2 -
          2.0 * p_avg * p_sum +
          p_avg * p_avg * (double)n_gets) / (double)(n_gets - 1);
  p_sd = sqrt(p_sd);

  fprintf(stdout, "Queue stats:\n");
  fprintf(stdout, "wait time min =\t" PFSZS(22, d) " nsec.\n", p_min);
  fprintf(stdout, "wait time max =\t" PFSZS(22, d) " nsec.\n", p_max);
  fprintf(stdout, "wait time avg =\t%25.2f nsec.\n", p_avg);
  fprintf(stdout, "wait time sd =\t%25.2f.\n\n", p_sd);

  t_total = t_end - t_begin;
  t_avg = (double)t_total / (double)n_gets;

  fprintf(stdout, "Throughput:\n");
  fprintf(stdout, "total time:\t" PFSZS(22, d) " msec.\n",
          (size_t)(t_total / 1000LL / 1000LL));
  fprintf(stdout, "total avg:\t%25.2f nsec/event.\n", t_avg);

  ret = 0;

done:
  if (bbqs != NULL) {
    for (i = 0; i < n_created_bbqs; i++) {
      mccp_bbq_destroy(&(bbqs[i]), true);
    }
    free((void *)bbqs);
  }

  if (polls != NULL) {
    for (i = 0; i < n_created_polls; i++) {
      mccp_qmuxer_poll_destroy(&(polls[i]));
    }
    free((void *)polls);
  }

  if (tts != NULL) {
    for (i = 0; i < n_created_thds; i++) {
      tt = &(tts[i]);
      mccp_thread_destroy((mccp_thread_t *)&tt);
    }
    free((void *)tts);
  }

  if (qmx != NULL) {
    mccp_qmuxer_destroy(&qmx);
  }

  if (start_lock != NULL) {
    mccp_mutex_destroy(&start_lock);
  }

  return ret;
}





int
main(int argc, const char *const argv[]) {
  size_t nthds = 8;
  ssize_t nputs = 1000LL * 1000LL * 10LL;
  pid_t c_pid;

  if (argc >= 2) {
    size_t n;
    if (mccp_str_parse_int64(argv[1], (int64_t *)&n) ==
        MCCP_RESULT_OK) {
      if (n > 0) {
        nthds = (size_t)n;
      }
    }
  }
  if (argc >= 3) {
    ssize_t n;
    if (mccp_str_parse_int64(argv[2], (int64_t *)&n) ==
        MCCP_RESULT_OK) {
      if (n > 0) {
        nputs = (ssize_t)n;
      }
    }
  }

  c_pid = fork();
  if (c_pid == 0) {
    return do_run(nthds, nputs);
  } else if (c_pid > 0) {
    int st;
    (void)waitpid(c_pid, &st, 0);
    return 0;
  } else {
    perror("fork");
    return 1;
  }
}
