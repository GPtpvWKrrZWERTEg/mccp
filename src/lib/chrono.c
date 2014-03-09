#include <mccp/mccp.h>





mccp_chrono_t
mccp_chrono_now(void) {
  mccp_chrono_t ret = 0;
  WHAT_TIME_IS_IT_NOW_IN_NSEC(ret);
  return ret;
}


mccp_result_t
mccp_chrono_to_timespec(struct timespec *dstptr,
                        mccp_chrono_t nsec) {
  if (dstptr != NULL) {
    NSEC_TO_TS(nsec, *dstptr);
    return MCCP_RESULT_OK;
  } else {
    return MCCP_RESULT_INVALID_ARGS;
  }
}


mccp_result_t
mccp_chrono_to_timeval(struct timeval *dstptr,
                       mccp_chrono_t nsec) {
  if (dstptr != NULL) {
    NSEC_TO_TV(nsec, *dstptr);
    return MCCP_RESULT_OK;
  } else {
    return MCCP_RESULT_INVALID_ARGS;
  }
}


mccp_result_t
mccp_chrono_from_timespec(mccp_chrono_t *dstptr,
                          const struct timespec *specptr) {
  if (dstptr != NULL &&
      specptr != NULL) {
    *dstptr = TS_TO_NSEC(*specptr);
    return MCCP_RESULT_OK;
  } else {
    return MCCP_RESULT_INVALID_ARGS;
  }
}


mccp_result_t
mccp_chrono_from_timeval(mccp_chrono_t *dstptr,
                         const struct timeval *specptr) {
  if (dstptr != NULL &&
      specptr != NULL) {
    *dstptr = TV_TO_NSEC(*specptr);
    return MCCP_RESULT_OK;
  } else {
    return MCCP_RESULT_INVALID_ARGS;
  }
}


mccp_result_t
mccp_chrono_nanosleep(mccp_chrono_t nsec,
                      mccp_chrono_t *remptr) {
  struct timespec t;
  struct timespec r;

  if (nsec < 0) {
    return MCCP_RESULT_INVALID_ARGS;
  }

  if (remptr == NULL) {
    NSEC_TO_TS(nsec, t);

  retry:
    errno = 0;
    if (nanosleep(&t, &r) == 0) {
      return MCCP_RESULT_OK;
    } else {
      if (errno == EINTR) {
        t = r;
        goto retry;
      } else {
        return MCCP_RESULT_POSIX_API_ERROR;
      }
    }
  } else {
    errno = 0;
    if (nanosleep(&t, &r) == 0) {
      return MCCP_RESULT_OK;
    } else {
      *remptr = TS_TO_NSEC(r);
      return MCCP_RESULT_POSIX_API_ERROR;
    }
  }
}
