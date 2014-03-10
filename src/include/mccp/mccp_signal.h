#ifndef __MCCP_SIGNAL_H__
#define __MCCP_SIGNAL_H__





/**
 *	@file mccp_signal.h
 */





__BEGIN_DECLS


/**
 * A MT-Safe signal(2).
 */
mccp_result_t
mccp_signal(int signum, sighandler_t new, sighandler_t *oldptr);


/**
 * Fallback the signal handling mechanism to the good-old-school
 * semantics.
 */
void	mccp_signal_old_school_semantics(void);


__END_DECLS





#endif /* ! __MCCP_SIGNAL_H__ */
