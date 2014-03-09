#ifndef __MCCP_TYPES_H__
#define __MCCP_TYPES_H__





/**
 *	@file	mccp_types.h
 */





#ifndef HAVE_SIGHANDLER_T
#ifdef HAVE_SIG_T
typedef sig_t sighandler_t;
#endif /* HAVE_SIG_T */
#endif /* ! HAVE_SIGHANDLER_T */


/**
 * @details The result type.
 */
typedef int64_t mccp_result_t;


/**
 * @details The flat nano second expression of the time, mainly
 * acquired by \b clock_gettime(). For arithmetic operations, the sign
 * extension is needed.
 */
typedef	int64_t	mccp_chrono_t;





#endif /* ! __MCCP_TYPES_H__ */
