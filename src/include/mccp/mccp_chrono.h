#ifndef __MCCP_CHRONO_H__
#define __MCCP_CHRONO_H__





/**
 *	@file	mccp_chrono.h
 */





mccp_chrono_t
mccp_chrono_now(void);


mccp_result_t
mccp_chrono_to_timespec(struct timespec *dstptr,
                        mccp_chrono_t nsec);


mccp_result_t
mccp_chrono_to_timeval(struct timeval *dstptr,
                       mccp_chrono_t nsec);


mccp_result_t
mccp_chrono_from_timespec(mccp_chrono_t *dstptr,
                          const struct timespec *specptr);


mccp_result_t
mccp_chrono_from_timeval(mccp_chrono_t *dstptr,
                         const struct timeval *valptr);


mccp_result_t
mccp_chrono_nanosleep(mccp_chrono_t nsec,
                      mccp_chrono_t *remptr);





#endif /* ! __MCCP_CHRONO_H__ */
