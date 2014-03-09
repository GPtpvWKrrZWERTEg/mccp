#include <mccp/mccp.h>





static const char *const s_error_strs[] = {
  "No error(s)",			/*  0 */
  "Unknown, any failure(s)",		/*  1 */
  "POSIX API error(s)",			/*  2 */
  "Insufficient memory",		/*  3 */
  "Not found",				/*  4 */
  "Already exists",			/*  5 */
  "Not operational at this moment",	/*  6 */
  "Invalid argument(s)",		/*  7 */
  "Not the owner",			/*  8 */
  "Not started",			/*  9 */
  "Timedout",				/* 10 */
  "Iteration halted",			/* 11 */
  "Out of range",			/* 12 */
  "Not a number",			/* 13 */
  "Already halted",			/* 14 */
  "An invalid object",			/* 15 */
  "A critical region is not closed",	/* 16 */
  "A critical region is not opened",	/* 17 */
  "Invalid state transition",		/* 18 */
  "Busy",				/* 19 */
  "Stop",				/* 20 */
  "Unsupported",			/* 21 */
  "Quote not closed",			/* 22 */
  "Not allowed",			/* 23 */
  "Not defined",			/* 24 */
  NULL
};


static ssize_t s_n_error_strs =
  sizeof(s_error_strs) / sizeof(const char *);





const char *
mccp_error_get_string(mccp_result_t r) {
  if (r < 0) {
    ssize_t ar = -r;
    if (ar < s_n_error_strs) {
      return s_error_strs[ar];
    } else {
      return "??? error string out of range ???";
    }
  } else {
    return s_error_strs[0];
  }
}

