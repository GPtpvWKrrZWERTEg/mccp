#include <mccp/mccp.h>

#include "qmuxer_types.h"
#include "qmuxer_internal.h"





static inline mccp_result_t
s_poll_initialize(mccp_qmuxer_poll_t mp,
                  mccp_bbq_t bbq,
                  mccp_qmuxer_poll_event_t type) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mp != NULL &&
      bbq != NULL &&
      IS_VALID_POLL_TYPE(type) == true) {
    (void)memset((void *)mp, 0, sizeof(*mp));
    mp->m_bbq = bbq;
    mp->m_type = type;
    mp->m_q_size = 0;
    mp->m_q_rem_capacity = 0;

    ret = MCCP_RESULT_OK;

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_poll_destroy(mccp_qmuxer_poll_t mp) {
  free((void *)mp);
}





mccp_result_t
mccp_qmuxer_poll_create(mccp_qmuxer_poll_t *mpptr,
                        mccp_bbq_t bbq,
                        mccp_qmuxer_poll_event_t type) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mpptr != NULL) {
    *mpptr = (mccp_qmuxer_poll_t)malloc(sizeof(**mpptr));
    if (*mpptr == NULL) {
      ret = MCCP_RESULT_NO_MEMORY;
      goto done;
    }
    ret = s_poll_initialize(*mpptr, bbq, type);
    if (ret != MCCP_RESULT_OK) {
      s_poll_destroy(*mpptr);
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}


void
mccp_qmuxer_poll_destroy(mccp_qmuxer_poll_t *mpptr) {
  if (mpptr != NULL) {
    s_poll_destroy(*mpptr);
  }
}


mccp_result_t
mccp_qmuxer_poll_reset(mccp_qmuxer_poll_t *mpptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL) {
    (*mpptr)->m_q_size = 0;
    (*mpptr)->m_q_rem_capacity = 0;

    ret = MCCP_RESULT_OK;

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_qmuxer_poll_set_queue(mccp_qmuxer_poll_t *mpptr,
                           mccp_bbq_t bbq) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL) {
    (*mpptr)->m_bbq = bbq;
    if (bbq != NULL) {
      bool tmp = false;
      ret = mccp_bbq_is_operational(&bbq, &tmp);
      if (ret == MCCP_RESULT_OK && tmp == true) {
        ret = MCCP_RESULT_OK;
      } else {
        /*
         * rather MCCP_RESULT_INVALID_ARGS ?
         */
        ret = MCCP_RESULT_NOT_OPERATIONAL;
      }
    } else {
      ret = MCCP_RESULT_OK;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_qmuxer_poll_get_queue(mccp_qmuxer_poll_t *mpptr,
                           mccp_bbq_t *bbqptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL &&
      bbqptr != NULL) {
    *bbqptr = (*mpptr)->m_bbq;
    ret = MCCP_RESULT_OK;
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_qmuxer_poll_set_type(mccp_qmuxer_poll_t *mpptr,
                          mccp_qmuxer_poll_event_t type) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL &&
      IS_VALID_POLL_TYPE(type) == true) {

    if ((*mpptr)->m_bbq != NULL) {
      (*mpptr)->m_type = type;
    } else {
      (*mpptr)->m_type = MCCP_QMUXER_POLL_UNKNOWN;
    }
    ret = MCCP_RESULT_OK;
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_qmuxer_poll_size(mccp_qmuxer_poll_t *mpptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL) {
    if ((*mpptr)->m_bbq != NULL) {
      ret = (*mpptr)->m_q_size;
    } else {
      ret = 0;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_qmuxer_poll_remaining_capacity(mccp_qmuxer_poll_t *mpptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL) {
    if ((*mpptr)->m_bbq != NULL) {
      ret = (*mpptr)->m_q_rem_capacity;
    } else {
      ret = 0;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}

