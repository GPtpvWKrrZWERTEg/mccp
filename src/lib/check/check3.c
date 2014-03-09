#include <mccp/mccp.h>





static mccp_result_t
s_main_proc(mccp_thread_t *selfptr, void *arg) {
  mccp_msg("Called with (%p, %p)\n", *selfptr, arg);
  return MCCP_RESULT_OK;
}


static mccp_result_t
s_main_proc_with_sleep(mccp_thread_t *selfptr, void *arg) {
  mccp_msg("Called with (%p, %p)\n", *selfptr, arg);
  sleep(10);
  return MCCP_RESULT_OK;
}

static void
s_freeup_proc(mccp_thread_t *selfptr, void *arg) {
  mccp_msg("Called with (%p, %p)\n", *selfptr, arg);
}

static void
s_finalize_proc(mccp_thread_t *selfptr,
                bool is_canceled, void *arg) {
  mccp_msg("Called with (%p, %s, %p)\n",
           *selfptr, BOOL_TO_STR(is_canceled), arg);
}

static void
s_check_cancelled(mccp_thread_t *thread, bool require) {
  bool canceled = !require;
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;
  if ((ret = mccp_thread_is_canceled(thread, &canceled))
      != MCCP_RESULT_OK) {
    mccp_perror(ret, "mccp_thread_is_canceled()");
    mccp_msg_error("Can't check is_cancelled.\n");
  } else if (canceled != require) {
    mccp_msg_error(
      "is_cancelled() required %s, but %s.\n",
      BOOL_TO_STR(require), BOOL_TO_STR(canceled));
  }
}

int
main(int argc, char const *const argv[]) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;
  int i;
  int n = 10000;
  mccp_thread_t thread = NULL;
  pthread_t tid = MCCP_INVALID_THREAD;

  mccp_log_set_debug_level(10);

  if (argc > 1) {
    int tmp;
    if (mccp_str_parse_int32(argv[1], &tmp) == MCCP_RESULT_OK &&
        tmp > 0) {
      n = tmp;
    }
  }

  /* create, start, wait, destroy */
  for (i = 0; i < n; i++) {
    thread = NULL;

    if ((ret = mccp_thread_create(
                 (mccp_thread_t *)&thread,
                 (mccp_thread_main_proc_t)s_main_proc,
                 (mccp_thread_finalize_proc_t)s_finalize_proc,
                 (mccp_thread_freeup_proc_t)s_freeup_proc,
                 (const char *) "thread",
                 (void *) NULL)) != MCCP_RESULT_OK) {
      mccp_perror(ret, "mccp_thread_create()");
      mccp_exit_fatal("Can't create a thread.\n");
    } else {
      fprintf(stderr, "create a thread [%d]\n", i);
      if ((ret = mccp_thread_get_pthread_id(&thread, &tid))
          != MCCP_RESULT_NOT_STARTED) {
        mccp_perror(ret, "mccp_thread_get_pthread_id()");
        goto done;
      }
    }

    if ((ret = mccp_thread_start(&thread, false)) != MCCP_RESULT_OK) {
      mccp_perror(ret, "mccp_thread_start()");
      mccp_exit_fatal("Can't start the thread.\n");
    } else {
      ret = mccp_thread_get_pthread_id(&thread, &tid);
      if (ret == MCCP_RESULT_OK ||
          ret == MCCP_RESULT_ALREADY_HALTED) {
        fprintf(stderr, " start thread:  %lu\n", tid);
      } else {
        mccp_perror(ret, "mccp_thread_get_pthread_id()");
        goto done;
      }
    }

    s_check_cancelled(&thread, false);

    if ((ret = mccp_thread_wait(&thread, 1000LL * 1000LL * 1000LL)) !=
        MCCP_RESULT_OK) {
      mccp_perror(ret, "mccp_thread_wait()");
      mccp_msg_error("Can't wait the thread.\n");
    } else {
      if ((ret = mccp_thread_get_pthread_id(&thread, &tid))
          == MCCP_RESULT_OK) {
        mccp_msg_error(
          "Thread has been completed, but I can get thread ID, %lu\n",
          tid);
      } else if (ret != MCCP_RESULT_ALREADY_HALTED) {
        mccp_perror(ret, "mccp_thread_get_pthread_id()");
        goto done;
      }
    }

    s_check_cancelled(&thread, false);

    mccp_thread_destroy(&thread);
    fprintf(stderr, "destroy the thread [%d]\n", i);

    fprintf(stderr, "\n");
  }

  /* create, start, cancel, destroy */
  for (i = 0; i < n; i++) {
    thread = NULL;

    if ((ret = mccp_thread_create(
                 (mccp_thread_t *)&thread,
                 (mccp_thread_main_proc_t)s_main_proc_with_sleep,
                 (mccp_thread_finalize_proc_t)s_finalize_proc,
                 (mccp_thread_freeup_proc_t)s_freeup_proc,
                 (const char *) "thread",
                 (void *) NULL)) != MCCP_RESULT_OK) {
      mccp_perror(ret, "mccp_thread_create()");
      mccp_exit_fatal("Can't create a thread.\n");
    } else {
      fprintf(stderr, "create a thread [%d]\n", i);

      if ((ret = mccp_thread_get_pthread_id(&thread, &tid))
          != MCCP_RESULT_NOT_STARTED) {
        mccp_perror(ret, "mccp_thread_get_pthread_id()");
        goto done;
      }
    }

    if ((ret = mccp_thread_start(&thread, false)) != MCCP_RESULT_OK) {
      mccp_perror(ret, "mccp_thread_start()");
      mccp_exit_fatal("Can't start the thread.\n");
    } else {
      ret = mccp_thread_get_pthread_id(&thread, &tid);
      if (ret == MCCP_RESULT_OK ||
          ret == MCCP_RESULT_ALREADY_HALTED) {
        fprintf(stderr, " start thread:  %lu\n", tid);
      } else {
        mccp_perror(ret, "mccp_thread_get_pthread_id()");
        goto done;
      }
    }

    s_check_cancelled(&thread, false);

    if ((ret = mccp_thread_cancel(&thread)) !=
        MCCP_RESULT_OK) {
      mccp_perror(ret, "mccp_thread_cancel()");
      mccp_msg_error("Can't cancel the thread.\n");
    } else {
      if ((ret = mccp_thread_get_pthread_id(&thread, &tid))
          == MCCP_RESULT_OK) {
        mccp_msg_error(
          "Thread has been canceled, but I can get thread ID, %lu\n",
          tid);
      } else if (ret != MCCP_RESULT_NOT_STARTED &&
                 ret != MCCP_RESULT_ALREADY_HALTED) {
        mccp_perror(ret, "mccp_thread_get_pthread_id()");
        goto done;
      }
    }

    s_check_cancelled(&thread, true);

    mccp_thread_destroy(&thread);
    fprintf(stderr, "destroy the thread [%d]\n", i);

    fprintf(stderr, "\n");
  }

  /* create, start, destroy */
  for (i = 0; i < n; i++) {
    thread = NULL;

    if ((ret = mccp_thread_create(
                 (mccp_thread_t *)&thread,
                 (mccp_thread_main_proc_t)s_main_proc_with_sleep,
                 (mccp_thread_finalize_proc_t)s_finalize_proc,
                 (mccp_thread_freeup_proc_t)s_freeup_proc,
                 (const char *) "thread",
                 (void *) NULL)) != MCCP_RESULT_OK) {
      mccp_perror(ret, "mccp_thread_create()");
      mccp_exit_fatal("Can't create a thread.\n");
    } else {
      fprintf(stderr, "create a thread [%d]\n", i);

      if ((ret = mccp_thread_get_pthread_id(&thread, &tid))
          != MCCP_RESULT_NOT_STARTED) {
        mccp_perror(ret, "mccp_thread_get_pthread_id()");
        goto done;
      }
    }

    if ((ret = mccp_thread_start(&thread, false)) != MCCP_RESULT_OK) {
      mccp_perror(ret, "mccp_thread_start()");
      mccp_exit_fatal("Can't start the thread.\n");
    } else {
      ret = mccp_thread_get_pthread_id(&thread, &tid);
      if (ret == MCCP_RESULT_OK ||
          ret == MCCP_RESULT_ALREADY_HALTED) {
        fprintf(stderr, " start thread:  %lu\n", tid);
      } else {
        mccp_perror(ret, "mccp_thread_get_pthread_id()");
        goto done;
      }
    }

    mccp_thread_destroy(&thread);
    fprintf(stderr, "destroy the thread [%d]\n", i);

    fprintf(stderr, "\n");
  }

done:
  return (ret == MCCP_RESULT_OK) ? 0 : 1;
}
