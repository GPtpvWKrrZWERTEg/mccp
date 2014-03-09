#ifndef __QMUXER_TYPES_H__
#define __QMUXER_TYPES_H__





typedef struct mccp_qmuxer_poll_record {
  mccp_bbq_t m_bbq;
  mccp_qmuxer_poll_event_t m_type;
  ssize_t m_q_size;
  ssize_t m_q_rem_capacity;
} mccp_qmuxer_poll_record;


typedef struct mccp_qmuxer_record {
  mccp_mutex_t m_lock;
  mccp_cond_t m_cond;
} mccp_qmuxer_record;





#endif /* __QMUXER_TYPES_H__ */
