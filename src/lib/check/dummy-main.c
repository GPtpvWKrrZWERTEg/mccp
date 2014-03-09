#include <mccp/mccp.h>





#define ONE_SEC		1000LL * 1000LL * 1000LL
#define REQ_TIMEDOUT	ONE_SEC





static volatile bool s_got_term_sig = false;


static void
s_term_handler(int sig) {
  mccp_result_t r = MCCP_RESULT_ANY_FAILURES;
  mccp_global_state_t gs = MCCP_GLOBAL_STATE_UNKNOWN;

  if ((r = mccp_global_state_get(&gs)) == MCCP_RESULT_OK) {

    if ((int)gs >= (int)MCCP_GLOBAL_STATE_STARTED) {

      shutdown_grace_level_t l = SHUTDOWN_UNKNOWN;
      if (sig == SIGTERM || sig == SIGINT) {
        l = SHUTDOWN_GRACEFULLY;
      } else if (sig == SIGQUIT) {
        l = SHUTDOWN_RIGHT_NOW;
      }
      if (IS_VALID_SHUTDOWN(l) == true) {
        mccp_msg_debug(5, "About to request shutdown(%s)...\n",
                       (l == SHUTDOWN_RIGHT_NOW) ?
                       "RIGHT_NOW" : "GRACEFULLY");
        if ((r = mccp_global_state_request_shutdown(l)) == MCCP_RESULT_OK) {
          mccp_msg_debug(5, "the shutdown request accepted.\n");
        } else {
          mccp_perror(r, "mccp_global_state_request_shutdown()");
          mccp_msg_error("can't request shutdown.\n");
        }
      }

    } else {
      if (sig == SIGTERM || sig == SIGINT || sig == SIGQUIT) {
        s_got_term_sig = true;
      }
    }

  }

}


static void
s_hup_handler(int sig) {
  (void)sig;
  mccp_msg_debug(5, "called. it's dummy for now.\n");
}


static mccp_chrono_t s_to = -1LL;


static inline void
usage(FILE *fd) {
  fprintf(fd, "usage:\n");
  fprintf(fd, "\t--help\tshow this.\n");
  fprintf(fd, "\t-to #\tset shutdown timeout (in sec.).\n");
  mccp_module_usage_all(fd);
  (void)fflush(fd);
}


static void
parse_args(int argc, const char *const argv[]) {
  (void)argc;

  while (*argv != NULL) {
    if (strcmp("--help", *argv) == 0) {
      usage(stderr);
      exit(0);
    } else if (strcmp("-to", *argv) == 0) {
      if (IS_VALID_STRING(*(argv + 1)) == true) {
        int64_t tmp;
        argv++;
        if (mccp_str_parse_int64(*argv, &tmp) == MCCP_RESULT_OK) {
          if (tmp >= 0) {
            s_to = ONE_SEC * tmp;
          }
        } else {
          fprintf(stderr, "can't parse \"%s\" as a number.\n", *argv);
          exit(1);
        }
      } else {
        fprintf(stderr, "A timeout # is not specified.\n");
        exit(1);
      }
    }
    argv++;
  }
}


int
main(int argc, const char *const argv[]) {
  const char *func = NULL;
  mccp_result_t st = MCCP_RESULT_ANY_FAILURES;

  (void)mccp_signal(SIGHUP, s_hup_handler, NULL);
  (void)mccp_signal(SIGINT, s_term_handler, NULL);
  (void)mccp_signal(SIGTERM, s_term_handler, NULL);
  (void)mccp_signal(SIGQUIT, s_term_handler, NULL);

  parse_args(argc - 1, argv + 1);

  (void)mccp_global_state_set(MCCP_GLOBAL_STATE_INITIALIZING);

  fprintf(stderr, "Initializing... ");
  func = "mccp_module_initialize_all()";
  if ((st = mccp_module_initialize_all(argc, argv)) ==
      MCCP_RESULT_OK &&
      s_got_term_sig == false) {
    fprintf(stderr, "Initialized.\n");
    fprintf(stderr, "Starting... ");
    (void)mccp_global_state_set(MCCP_GLOBAL_STATE_STARTING);
    func = "mccp_module_start_all()";
    if ((st = mccp_module_start_all()) ==
        MCCP_RESULT_OK &&
        s_got_term_sig == false) {
      fprintf(stderr, "Started.\n");
      func = "mccp_global_state_set()";
      if ((st = mccp_global_state_set(MCCP_GLOBAL_STATE_STARTED)) ==
          MCCP_RESULT_OK) {

        shutdown_grace_level_t l = SHUTDOWN_UNKNOWN;

        fprintf(stderr, "Running.\n");

        func = "mccp_global_state_wait_for_shutdown_request()";
        while ((st = mccp_global_state_wait_for_shutdown_request(&l,
                     REQ_TIMEDOUT)) ==
               MCCP_RESULT_TIMEDOUT) {
          mccp_msg_debug(5, "waiting shutdown request...\n");
        }
        if (st == MCCP_RESULT_OK) {
          func = "mccp_global_state_set()";
          if ((st = mccp_global_state_set(MCCP_GLOBAL_STATE_ACCEPT_SHUTDOWN)) ==
              MCCP_RESULT_OK) {
            func = "mccp_module_shutdown_all()";
            if ((st = mccp_module_shutdown_all(l)) == MCCP_RESULT_OK) {
              func = "mccp_module_wait_all()";
              if ((st = mccp_module_wait_all(s_to)) == MCCP_RESULT_OK) {
                fprintf(stderr, "Finished cleanly.\n");
              } else if (st == MCCP_RESULT_TIMEDOUT) {
                fprintf(stderr, "Trying to stop forcibly...\n");
                func = "mccp_module_stop_all()";
                if ((st = mccp_module_stop_all()) == MCCP_RESULT_OK) {
                  func = "mccp_module_wait_all()";
                  if ((st = mccp_module_wait_all(s_to)) ==
                      MCCP_RESULT_OK) {
                    fprintf(stderr, "Stopped forcibly.\n");
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  mccp_module_finalize_all();

  if (st != MCCP_RESULT_OK) {
    fprintf(stderr, "\n");
    mccp_perror(st, func);
  }

  return (st == MCCP_RESULT_OK) ? 0 : 1;
}
