#include <mccp/mccp.h>





static pthread_mutex_t s_log_lock = PTHREAD_MUTEX_INITIALIZER;
static FILE *s_log_fd = NULL;
static mccp_log_destination_t s_log_dst = MCCP_LOG_EMIT_TO_UNKNOWN;
static char *s_log_arg = NULL;
static bool s_do_multi_process = false;
static bool s_do_date = false;
static uint64_t s_dbg_level = 0;
static uint64_t s_trace_flags = 0LL;
static bool s_is_fork_initialized = false;

static const char *const s_log_level_strs[] = {
  "",
  "[DEBUG]",
  "[TRACE]",
  "[INFO ]",
  "[NOTE ]",
  "[WARN ]",
  "[ERROR]",
  "[FATAL]",
  NULL
};

static int const s_syslog_priorities[] = {
  LOG_INFO,
  LOG_DEBUG,
  LOG_INFO,
  LOG_INFO,
  LOG_NOTICE,
  LOG_WARNING,
  LOG_ERR,
  LOG_CRIT
};





static void
s_child_at_fork(void) {
  (void)pthread_mutex_init(&s_log_lock, NULL);
}





static inline void
s_lock_fd(FILE *fd, bool cmd) {
  if (fd != NULL) {
    struct flock fl;

    fl.l_type = (cmd == true) ? F_WRLCK : F_UNLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;	/* Entire lock. */
    fl.l_pid = 0;

    (void)fcntl(fileno(fd), F_SETLKW, &fl);
  }
}


static inline void
s_lock(void) {
  (void)pthread_mutex_lock(&s_log_lock);
  if (s_do_multi_process == true &&
      s_log_dst == MCCP_LOG_EMIT_TO_FILE &&
      s_log_fd != NULL) {
    s_lock_fd(s_log_fd, true);
  }
}


static inline void
s_unlock(void) {
  if (s_do_multi_process == true &&
      s_log_dst == MCCP_LOG_EMIT_TO_FILE &&
      s_log_fd != NULL) {
    s_lock_fd(s_log_fd, false);
  }
  (void)pthread_mutex_unlock(&s_log_lock);
}


static inline void
s_log_final(void) {
  if (s_log_dst == MCCP_LOG_EMIT_TO_FILE) {
    if (s_log_fd != NULL) {
      s_lock_fd(s_log_fd, false);
      (void)fclose(s_log_fd);
      s_log_fd = NULL;
    }
  } else if (s_log_dst == MCCP_LOG_EMIT_TO_SYSLOG) {
    closelog();
  }
  free((void *)s_log_arg);
  s_log_arg = NULL;
}


static inline bool
s_is_file(const char *file) {
  bool ret = false;
  struct stat s;

  if (stat(file, &s) == 0) {
    if (S_ISDIR(s.st_mode)) {
      return ret;
    } else {
      ret = true;
    }
  } else {
    ret = true;
  }

  return ret;
}


static inline bool
s_log_init(mccp_log_destination_t dst,
           const char *arg,
           bool multi_process,
           bool emit_date,
           uint64_t debug_level,
           uint64_t trace_flags) {
  bool ret = false;

  if (s_is_fork_initialized == false) {
    (void)pthread_atfork(NULL, NULL, s_child_at_fork);
    s_is_fork_initialized = true;
  }

  if (dst == MCCP_LOG_EMIT_TO_FILE ||
      dst == MCCP_LOG_EMIT_TO_UNKNOWN) {
    int ofd = -INT_MAX;

    s_log_final();

    if (IS_VALID_STRING(arg) == true &&
        dst == MCCP_LOG_EMIT_TO_FILE &&
        s_is_file(arg) == true) {
      ofd = open(arg, O_RDWR | O_CREAT | O_APPEND, 0600);
      if (ofd >= 0) {
        s_log_fd = fdopen(ofd, "a+");
        if (s_log_fd != NULL) {
          ret = true;
        }
      }
    } else {
      s_log_fd = NULL;	/* use stderr. */
      ret = true;
    }
  } else if (dst == MCCP_LOG_EMIT_TO_SYSLOG) {
    if (IS_VALID_STRING(arg) == true) {

      s_log_final();

      openlog(arg, 0, LOG_USER);

      ret = true;
    }
  }

  if (ret == true) {
    s_log_dst = dst;
    free((void *)s_log_arg);
    s_log_arg = (IS_VALID_STRING(arg) == true) ? strdup(arg) : NULL;
    s_do_multi_process = multi_process;
    s_do_date = emit_date;
    s_dbg_level = debug_level;
    s_trace_flags = trace_flags;
  }

  return ret;
}


static inline bool
s_log_reinit(void) {
  char *arg =
    (IS_VALID_STRING(s_log_arg) == true) ? strdup(s_log_arg) : NULL;
  bool ret = s_log_init(s_log_dst, arg,
                        s_do_multi_process, s_do_date,
                        s_dbg_level, s_trace_flags);

  free((void *)arg);

  return ret;
}


static inline const char *
s_get_level_str(mccp_log_level_t l) {
  if ((int)l >= (int)MCCP_LOG_LEVEL_UNKNOWN &&
      (int)l <= (int)MCCP_LOG_LEVEL_FATAL) {
    return s_log_level_strs[(int)l];
  }
  return NULL;
}


static inline int
s_get_syslog_priority(mccp_log_level_t l) {
  if ((int)l >= (int)MCCP_LOG_LEVEL_UNKNOWN &&
      (int)l <= (int)MCCP_LOG_LEVEL_FATAL) {
    return s_syslog_priorities[(int)l];
  }
  return LOG_INFO;
}


static inline size_t
s_get_date_str(char *buf, size_t buf_len) {
  const char *fmt = "[%a %h %d %T %Z %Y]";
  time_t x = time(NULL);
  return strftime(buf, buf_len, fmt, localtime(&x));
}


static inline void
s_do_log(mccp_log_level_t l, const char *msg) {
  FILE *fd;
  int o_cancel_state;

  (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &o_cancel_state);

  s_lock();

  switch (s_log_dst) {
    case MCCP_LOG_EMIT_TO_FILE:
    case MCCP_LOG_EMIT_TO_UNKNOWN: {
      fd = (s_log_fd != NULL) ? s_log_fd : stderr;
      (void)fprintf(fd, "%s", msg);
      (void)fflush(fd);
      break;
    }
    case MCCP_LOG_EMIT_TO_SYSLOG: {
      int prio = s_get_syslog_priority(l);
      syslog(prio, "%s", msg);
    }
  }

  s_unlock();

  (void)pthread_setcancelstate(o_cancel_state, NULL);
}





void
mccp_log_emit(mccp_log_level_t lv,
              uint64_t flags,
              const char *file,
              int line,
              const char *func,
              const char *fmt, ...) {
  if ((lv != MCCP_LOG_LEVEL_DEBUG && lv != MCCP_LOG_LEVEL_TRACE) ||
      (lv == MCCP_LOG_LEVEL_DEBUG &&
       flags <= s_dbg_level) ||
      (lv == MCCP_LOG_LEVEL_TRACE &&
       (s_trace_flags & flags) != 0LL)) {

    va_list args;
    char date_buf[32];
    char msg[4096];
    size_t hdr_len;
    size_t left_len;
    char thd_info_buf[1024];
    pthread_t tid;
    char thd_name[32];
    int st = 0;
    int s_errno = errno;

    if (s_do_date == true) {
      s_get_date_str(date_buf, sizeof(date_buf));
    } else {
      date_buf[0] = '\0';
    }

    tid = pthread_self();
#ifdef HAVE_PTHREAD_SETNAME_NP
    st = pthread_getname_np(tid, thd_name, sizeof(thd_name));
#else
    thd_name[0] = '\0';
    st = -1;
#endif /* HAVE_PTHREAD_SETNAME_NP */
#if SIZEOF_PTHREAD_T == SIZEOF_INT64_T
#define TIDFMT "0x" PFTIDS(016, x)
#if SIZEOF_INT64_T == SIZEOF_LONG_INT
#define TIDTYPE long int
#else
#define TIDTYPE int64_t
#endif /* SIZEOF_INT64_T == SIZEOF_LONG_INT */
#elif SIZEOF_PTHREAD_T == SIZEOF_INT
#define TIDFMT "0x" PFTIDS(08, x)
#define TIDTYPE int32_t
#endif /* SIZEOF_PTHREAD_T == SIZEOF_INT64_T ... */
    if (st == 0 && IS_VALID_STRING(thd_name) == true) {
      snprintf(thd_info_buf, sizeof(thd_info_buf), "[%u:" TIDFMT ":%s]",
               (unsigned int)getpid(),
               (TIDTYPE)tid,
               thd_name);
    } else {
      snprintf(thd_info_buf, sizeof(thd_info_buf), "[%u:" TIDFMT "]",
               (unsigned int)getpid(),
               (TIDTYPE)tid);
    }
#undef TIDFMT
#undef TIDTYPE

    va_start(args, fmt);
    va_end(args);

    hdr_len = (size_t)snprintf(msg, sizeof(msg),
                               "%s%s%s:%s:%d:%s: ",
                               date_buf,
                               s_get_level_str(lv),
                               thd_info_buf,
                               file, line, func);

    /*
     * hdr_len indicates the buffer length WITHOUT '\0'.
     */
    left_len = sizeof(msg) - hdr_len;
    if (left_len > 1) {
      (void)vsnprintf(msg + hdr_len, left_len -1, fmt, args);
    }

    s_do_log(lv, msg);

    errno = s_errno;
  }
}


mccp_result_t
mccp_log_initialize(mccp_log_destination_t dst,
                    const char *arg,
                    bool multi_process,
                    bool emit_date,
                    uint64_t debug_level,
                    uint64_t trace_flags) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;
  int s_errno = errno;

  (void)pthread_mutex_lock(&s_log_lock);
  ret =
    (s_log_init(dst, arg, multi_process, emit_date,
                debug_level, trace_flags) == true) ?
    MCCP_RESULT_OK : MCCP_RESULT_ANY_FAILURES;
  (void)pthread_mutex_unlock(&s_log_lock);

  errno = s_errno;

  return ret;
}


mccp_result_t
mccp_log_reinitialize(void) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;
  int s_errno = errno;

  mccp_msg_debug(10, "Reinitialize the logger.\n");

  (void)pthread_mutex_lock(&s_log_lock);
  ret = (s_log_reinit() == true) ?
        MCCP_RESULT_OK : MCCP_RESULT_ANY_FAILURES;
  (void)pthread_mutex_unlock(&s_log_lock);

  errno = s_errno;

  return ret;
}


void
mccp_log_finalize(void) {
  int s_errno = errno;

  mccp_msg_debug(10, "Finalize the logger.\n");

  (void)pthread_mutex_lock(&s_log_lock);
  s_log_final();
  (void)pthread_mutex_unlock(&s_log_lock);

  errno = s_errno;
}


void
mccp_log_set_debug_level(uint64_t l) {
  (void)pthread_mutex_lock(&s_log_lock);
  s_dbg_level = l;
  (void)pthread_mutex_unlock(&s_log_lock);
}


uint64_t
mccp_log_get_debug_level(void) {
  uint64_t ret;

  (void)pthread_mutex_lock(&s_log_lock);
  ret = s_dbg_level;
  (void)pthread_mutex_unlock(&s_log_lock);

  return ret;
}


void
mccp_log_set_trace_flags(uint64_t flags) {
  (void)pthread_mutex_lock(&s_log_lock);
  s_trace_flags = s_trace_flags | flags;
  (void)pthread_mutex_unlock(&s_log_lock);
}


void
mccp_log_unset_trace_flags(uint64_t flags) {
  (void)pthread_mutex_lock(&s_log_lock);
  s_trace_flags = s_trace_flags & ~flags;
  (void)pthread_mutex_unlock(&s_log_lock);
}


uint64_t
mccp_log_get_trace_flags(void) {
  uint64_t ret;

  (void)pthread_mutex_lock(&s_log_lock);
  ret = s_trace_flags;
  (void)pthread_mutex_unlock(&s_log_lock);

  return ret;
}


mccp_log_destination_t
mccp_log_get_destination(void) {
  mccp_log_destination_t ret = MCCP_LOG_EMIT_TO_UNKNOWN;

  (void)pthread_mutex_lock(&s_log_lock);
  ret = s_log_dst;
  (void)pthread_mutex_unlock(&s_log_lock);

  return ret;
}





static pthread_once_t s_once = PTHREAD_ONCE_INIT;

static void s_ctors(void) __attr_constructor__(103);
static void s_dtors(void) __attr_destructor__(103);


typedef bool	(*path_verify_func_t)(const char *path,
                                    char *buf,
                                    size_t buflen);


static inline bool
s_is_valid_path(const char *path, char *buf, size_t buflen) {
  bool ret = false;

  if (IS_VALID_STRING(path) == true &&
      buf != NULL) {
    const char *org = path;
    char *t = NULL;

    while (isspace((int)*org)) {
      org++;
    }
    if (mccp_str_trim_right(org, "\t\r\n ", &t) >= 0) {
      if (strlen(t) > 0) {
        snprintf(buf, buflen, "%s", t);
        ret = true;
      }
      free((void *)t);
    }
  }

  return ret;
}


static inline char *
s_validate_path(const char *file, path_verify_func_t func) {
  static char buf[PATH_MAX];

  if ((func)(file, buf, sizeof(buf)) == true) {
    return buf;
  }

  return NULL;
}


static void
s_once_proc(void) {
  char *dbg_lvl_str = getenv("MCCP_LOG_DEBUGLEVEL");
  char *logfile = getenv("MCCP_LOG_FILE");
  uint64_t d = 0;
  mccp_log_destination_t log_dst = MCCP_LOG_EMIT_TO_UNKNOWN;

  if (IS_VALID_STRING(dbg_lvl_str) == true) {
    uint64_t tmp = 0;
    if (mccp_str_parse_uint64(dbg_lvl_str, &tmp) == MCCP_RESULT_OK) {
      d = tmp;
    }
  }
  if ((logfile = s_validate_path(logfile, s_is_valid_path)) != NULL) {
    log_dst = MCCP_LOG_EMIT_TO_FILE;
  }

  if (mccp_log_initialize(log_dst, logfile, false, true, d, 0LL) !=
      MCCP_RESULT_OK) {
    mccp_exit_error(1, "logger initialization error.\n");
  }

  if (d > 0) {
    mccp_msg_debug(d, "Logger debug level is set to: " PF64(u) ".\n", d);
  }
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();
}


static void
s_dtors(void) {
  mccp_log_finalize();
}

