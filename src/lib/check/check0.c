#include <mccp/mccp.h>


static inline const char *
myname(const char *argv0) {
  const char *p = (const char *)strrchr(argv0, '/');
  if (p != NULL) {
    p++;
  } else {
    p = argv0;
  }
  return p;
}


int
main(int argc, const char *const argv[]) {
  const char *nm = myname(argv[0]);
  (void)argc;

  mccp_log_set_trace_flags(0x01LL |
                           0x02LL |
                           0x04LL);

  mccp_msg("this should emitted to stderr.\n");

  mccp_msg_trace(0x01LL, "hello test.\n");
  mccp_msg_trace(0x02LL, "error test.\n");
  mccp_msg_trace(0x01LL | 0x02LL, "hello|error test.\n");

  /*
   * log to stderr.
   */
  if (IS_MCCP_RESULT_OK(
        mccp_log_initialize(MCCP_LOG_EMIT_TO_UNKNOWN, NULL,
                            false, true, 1, 0x07LL)) == false) {
    mccp_msg_fatal("what's wrong??\n");
    /* not reached. */
  }
  mccp_dprint("debug to stderr.\n");

  /*
   * log to file.
   */
  if (IS_MCCP_RESULT_OK(
        mccp_log_initialize(MCCP_LOG_EMIT_TO_FILE, "./testlog.txt",
                            false, true, 10, 0x07LL)) == false) {
    mccp_msg_fatal("what's wrong??\n");
    /* not reached. */
  }
  mccp_dprint("debug to file.\n");
  mccp_msg_debug(5, "debug to file, again.\n");
  mccp_msg_trace(0x01LL, "hello file test.\n");
  mccp_msg_trace(0x02LL, "error file test.\n");
  mccp_msg_trace(0x01LL | 0x02LL, "hello|error file test.\n");

  if (IS_MCCP_RESULT_OK(
        mccp_log_initialize(MCCP_LOG_EMIT_TO_SYSLOG, nm,
                            false, false, 10, 0x07LL)) == false) {
    mccp_msg_fatal("what's wrong??\n");
    /* not reached. */
  }
  mccp_msg_debug(5, "debug to syslog.\n");
  mccp_msg_trace(0x01LL, "hello syslog test.\n");
  mccp_msg_trace(0x02LL, "error syslog test.\n");
  mccp_msg_trace(0x01LL | 0x02LL, "hello|error syslog test.\n");

  /*
   * log to stderr, again.
   */
  if (IS_MCCP_RESULT_OK(
        mccp_log_initialize(MCCP_LOG_EMIT_TO_UNKNOWN, NULL,
                            false, true, 1, 0x07LL)) == false) {
    mccp_msg_fatal("what's wrong??\n");
    /* not reached. */
  }
  mccp_dprint("will exit 1 ...\n");
  mccp_exit_error(1, "exit 1 on purpose.\n");

  return 0;
}
