#include <mccp/mccp.h>





static volatile bool s_do_stop = false;

static mccp_rwlock_t s_lock = NULL;
static volatile uint64_t s_sum = 0LL;





static inline void
s_incr(void) {
  (void)mccp_rwlock_writer_lock(&s_lock);
  s_sum++;
  (void)mccp_rwlock_unlock(&s_lock);
}


static inline uint64_t
s_get(void) {
  uint64_t ret;

  (void)mccp_rwlock_reader_lock(&s_lock);
  ret = s_sum;
  (void)mccp_rwlock_unlock(&s_lock);

  return ret;
}


static inline void
s_set(uint64_t v) {
  (void)mccp_rwlock_writer_lock(&s_lock);
  s_sum = v;
  (void)mccp_rwlock_unlock(&s_lock);
}





static mccp_result_t
s_setup(const mccp_pipeline_stage_t *sptr) {
  (void)sptr;

  (void)mccp_rwlock_create(&s_lock);
  mccp_msg_debug(1, "called.\n");

  return MCCP_RESULT_OK;
}


static mccp_result_t
s_fetch(const mccp_pipeline_stage_t *sptr,
        size_t idx, void *buf, size_t max) {
  (void)sptr;
  (void)idx;
  (void)buf;
  (void)max;

  return (s_do_stop == false) ? 1LL : 0LL;
}


static mccp_result_t
s_main(const mccp_pipeline_stage_t *sptr,
       size_t idx, void *buf, size_t n) {
  (void)sptr;
  (void)buf;

  s_incr();

  mccp_msg_debug(1, "called " PFSZ(u) ".\n", idx);

  return (mccp_result_t)n;
}


static mccp_result_t
s_throw(const mccp_pipeline_stage_t *sptr,
        size_t idx, void *buf, size_t n) {
  (void)sptr;
  (void)idx;
  (void)buf;

  return (mccp_result_t)n;
}


static mccp_result_t
s_sched(const mccp_pipeline_stage_t *sptr,
        void *buf, size_t n) {
  (void)sptr;
  (void)buf;

  return (mccp_result_t)n;
}


static mccp_result_t
s_shutdown(const mccp_pipeline_stage_t *sptr,
           shutdown_grace_level_t l) {
  (void)sptr;

  mccp_msg_debug(1, "called with \"%s\".\n",
                 (l == SHUTDOWN_RIGHT_NOW) ? "right now" : "gracefully");

  return MCCP_RESULT_OK;
}


static void
s_finalize(const mccp_pipeline_stage_t *sptr,
           bool is_canceled) {
  (void)sptr;

  mccp_msg_debug(1, "%s.\n",
                 (is_canceled == false) ? "exit normally" : "canceled");
}


static void
s_freeup(const mccp_pipeline_stage_t *sptr) {
  (void)sptr;

  mccp_msg_debug(1, "called.\n");

  if (s_lock != NULL) {
    (void)mccp_rwlock_destroy(&s_lock);
    s_lock = NULL;
  }
}





int
main(int argc, const char *const argv[]) {
  mccp_result_t st = MCCP_RESULT_ANY_FAILURES;
  mccp_pipeline_stage_t s = NULL;
  const char *func = NULL;
  size_t nthd = 1;

  (void)argc;

  if (IS_VALID_STRING(argv[1]) == true) {
    size_t tmp;
    if (mccp_str_parse_uint64(argv[1], &tmp) == MCCP_RESULT_OK &&
        tmp > 0LL) {
      nthd = tmp;
    }
  }

  fprintf(stdout, "Creating... ");
  func = "mccp_pipeline_stage_create()";
  st = mccp_pipeline_stage_create(&s, 0, "a_test",
                                  nthd,
                                  sizeof(void *), 1024,
                                  s_sched,
                                  NULL,
                                  s_setup,
                                  s_fetch,
                                  s_main,
                                  s_throw,
                                  s_shutdown,
                                  s_finalize,
                                  s_freeup);
  if (st == MCCP_RESULT_OK) {
    fprintf(stdout, "Created.\n");
    fprintf(stdout, "Setting up... ");
    func = "mccp_pipeline_stage_setup()";
    st = mccp_pipeline_stage_setup(&s);
    if (st == MCCP_RESULT_OK) {
      fprintf(stdout, "Set up.\n");
      fprintf(stdout, "Starting... ");
      func = "mccp_pipeline_stage_start()";
      st = mccp_pipeline_stage_start(&s);
      if (st == MCCP_RESULT_OK) {
        fprintf(stdout, "Started.\n");
        fprintf(stdout, "Opening the front door... ");
        func = "mccp_global_state_set()";
        st = mccp_global_state_set(MCCP_GLOBAL_STATE_STARTED);
        if (st == MCCP_RESULT_OK) {
          char buf[1024];
          char *cmd = NULL;
          fprintf(stdout, "The front door is open.\n");

          fprintf(stdout, "> ");
          while (fgets(buf, sizeof(buf), stdin) != NULL &&
                 st == MCCP_RESULT_OK) {
            (void)mccp_str_trim_right(buf, "\r\n\t ", &cmd);

            if (strcasecmp(cmd, "pause") == 0 ||
                strcasecmp(cmd, "spause") == 0) {
              fprintf(stdout, "Pausing... ");
              func = "mccp_pipeline_stage_pause()";
              if ((st = mccp_pipeline_stage_pause(&s, -1LL)) ==
                  MCCP_RESULT_OK) {
                if (strcasecmp(cmd, "spause") == 0) {
                  s_set(0LL);
                }
                fprintf(stdout, "Paused " PF64(u) "\n", s_get());
              } else {
                fprintf(stdout, "Failure.\n");
              }
            } else if (strcasecmp(cmd, "resume") == 0) {
              fprintf(stdout, "Resuming... ");
              func = "mccp_pipeline_stage_resume()";
              if ((st = mccp_pipeline_stage_resume(&s)) ==
                  MCCP_RESULT_OK) {
                fprintf(stdout, "Resumed.\n");
              } else {
                fprintf(stdout, "Failure.\n");
              }
            } else if (strcasecmp(cmd, "get") == 0) {
              fprintf(stdout, PF64(u) "\n", s_get());
            }

            free((void *)cmd);
            cmd = NULL;
            fprintf(stdout, "> ");
          }
          fprintf(stdout, "\nDone.\n");

          fprintf(stdout, "Shutting down... ");
          func = "mccp_pipeline_stage_shutdown()";
          st = mccp_pipeline_stage_shutdown(&s, SHUTDOWN_GRACEFULLY);
          if (st == MCCP_RESULT_OK) {
            fprintf(stdout, "Shutdown accepted... ");
            sleep(1);
            s_do_stop = true;
            fprintf(stdout, "Waiting shutdown... ");
            func = "mccp_pipeline_stage_wait()";
            st = mccp_pipeline_stage_wait(&s, -1LL);
            if (st == MCCP_RESULT_OK) {
              fprintf(stdout, "OK, Shutdown.\n");
            }
          }
        }
      }
    }
  }
  fflush(stdout);

  if (st != MCCP_RESULT_OK) {
    mccp_perror(st, func);
  }

  fprintf(stdout, "Destroying... ");
  mccp_pipeline_stage_destroy(&s);
  fprintf(stdout, "Destroyed.\n");

  return (st == MCCP_RESULT_OK) ? 0 : 1;
}
