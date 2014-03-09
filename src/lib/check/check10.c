#include <mccp/mccp.h>





static volatile bool s_do_stop = false;


static mccp_result_t
s_setup(const mccp_pipeline_stage_t *sptr) {
  (void)sptr;

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

  fprintf(stdout, "Creating...\n");
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
    fprintf(stdout, "Setting up...\n");
    func = "mccp_pipeline_stage_setup()";
    st = mccp_pipeline_stage_setup(&s);
    if (st == MCCP_RESULT_OK) {
      fprintf(stdout, "Set up.\n");
      fprintf(stdout, "Starting...\n");
      func = "mccp_pipeline_stage_start()";
      st = mccp_pipeline_stage_start(&s);
      if (st == MCCP_RESULT_OK) {
        fprintf(stdout, "Started.\n");
        sleep(1);
        fprintf(stdout, "Opening the front door...\n");
        func = "mccp_global_state_set()";
        st = mccp_global_state_set(MCCP_GLOBAL_STATE_STARTED);
        if (st == MCCP_RESULT_OK) {
          fprintf(stdout, "The front door is open.\n");
          sleep(1);
          fprintf(stdout, "Shutting down...\n");
          func = "mccp_pipeline_stage_shutdown()";
          st = mccp_pipeline_stage_shutdown(&s, SHUTDOWN_GRACEFULLY);
          if (st == MCCP_RESULT_OK) {
            fprintf(stdout, "Shutdown accepted.\n");
            sleep(1);
            s_do_stop = true;
            fprintf(stdout, "Waiting shutdown...\n");
            func = "mccp_pipeline_stage_wait()";
            st = mccp_pipeline_stage_wait(&s, 1000LL * 1000LL * 1000LL);
            if (st == MCCP_RESULT_OK) {
              fprintf(stdout, "OK, Shutdown.\n");
            }
          }
        }
      }
    }
  }

  if (st != MCCP_RESULT_OK) {
    mccp_perror(st, func);
  }

  fprintf(stdout, "Destroying...\n");
  mccp_pipeline_stage_destroy(&s);
  fprintf(stdout, "Destroyed.\nn");

  return (st == MCCP_RESULT_OK) ? 0 : 1;
}
