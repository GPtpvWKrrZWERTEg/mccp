#include <mccp/mccp.h>
#include <mccp/mccp_thread_internal.h>





#define NTHDS	8


static mccp_bbq_t s_q = NULL;
static mccp_thread_t s_thds[NTHDS] = { 0 };


static mccp_result_t
s_main(const mccp_thread_t *tptr, void *arg) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  (void)arg;

  if (tptr != NULL) {
    mccp_result_t r;
    void *val;

    while (true) {
      val = (void *)false;
      if ((r = mccp_bbq_get(&s_q, &val, bool, -1LL)) !=
          MCCP_RESULT_OK) {
        if (r == MCCP_RESULT_NOT_OPERATIONAL) {
          r = MCCP_RESULT_OK;
        } else {
          mccp_perror(r, "mccp_bbq_get()");
        }
        break;
      }
    }

    ret = r;
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


static void
s_finalize(const mccp_thread_t *tptr, bool is_canceled, void *arg) {
  (void)arg;
  (void)tptr;

  mccp_msg_debug(1, "called. %s.\n",
                 (is_canceled == false) ? "finished" : "canceled");
  if (is_canceled == true) {
    mccp_bbq_cancel_janitor(&s_q);
  }
}


int
main(int argc, const char *const argv[]) {
  int ret = 1;
  mccp_result_t r;

  (void)argc;
  (void)argv;

  /*
   * Following both two cases must succeeded.
   */

  /*
   * 1) Shutdown the queue makes threads exit normally.
   */
  if ((r = mccp_bbq_create(&s_q, bool, 20, NULL)) == MCCP_RESULT_OK) {
    size_t i;

    for (i = 0; i < NTHDS; i++) {
      s_thds[i] = NULL;
      if ((r = mccp_thread_create(&s_thds[i],
                                  s_main,
                                  s_finalize,
                                  NULL,
                                  "getter", NULL)) != MCCP_RESULT_OK) {
        mccp_perror(r, "mccp_thread_create()");
        goto done;
      }
    }

    for (i = 0; i < NTHDS; i++) {
      if ((r = mccp_thread_start(&s_thds[i], false)) != MCCP_RESULT_OK) {
        mccp_perror(r, "mccp_thread_start()");
        goto done;
      }
    }

    sleep(1);

    /*
     * Shutdown the queue.
     */
    mccp_bbq_shutdown(&s_q, true);

    /*
     * Then threads exit. Delete all of them.
     */
    for (i = 0; i < NTHDS; i++) {
      if ((r = mccp_thread_wait(&s_thds[i], -1LL)) == MCCP_RESULT_OK) {
        mccp_thread_destroy(&s_thds[i]);
      }
    }

    /*
     * Delete the queue.
     */
    mccp_bbq_destroy(&s_q, true);
  }

  /*
   * 2) Cancel all the threads and delete the queue.
   */
  s_q = NULL;
  if ((r = mccp_bbq_create(&s_q, bool, 20, NULL)) == MCCP_RESULT_OK) {
    size_t i;

    for (i = 0; i < NTHDS; i++) {
      s_thds[i] = NULL;
      if ((r = mccp_thread_create(&s_thds[i],
                                  s_main,
                                  s_finalize,
                                  NULL,
                                  "getter", NULL)) != MCCP_RESULT_OK) {
        mccp_perror(r, "mccp_thread_create()");
        goto done;
      }
    }

    for (i = 0; i < NTHDS; i++) {
      if ((r = mccp_thread_start(&s_thds[i], false)) != MCCP_RESULT_OK) {
        mccp_perror(r, "mccp_thread_start()");
        goto done;
      }
    }

    sleep(1);

    /*
     * Cancel and destroy all the thread.
     */
    for (i = 0; i < NTHDS; i++) {
      if ((r = mccp_thread_cancel(&s_thds[i])) == MCCP_RESULT_OK) {
        mccp_thread_destroy(&s_thds[i]);
      }
    }

    /*
     * Delete the queue.
     */
    mccp_bbq_destroy(&s_q, true);
  }

  ret = 0;

done:
  return ret;
}
