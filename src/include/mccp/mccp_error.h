#ifndef __MCCP_ERROR_H__
#define __MCCP_ERROR_H__





/**
 *	@file	mccp_error.h
 */





#define MCCP_RESULT_OK		0


/*
 * Note:
 *
 *	Add error #s after this and add messages that corresponds to
 *	the error #s into s_error_strs[] in lib/error.c, IN ORDER.
 */


#define MCCP_RESULT_ANY_FAILURES		-1
#define MCCP_RESULT_POSIX_API_ERROR		-2
#define MCCP_RESULT_NO_MEMORY			-3
#define MCCP_RESULT_NOT_FOUND			-4
#define MCCP_RESULT_ALREADY_EXISTS		-5
#define MCCP_RESULT_NOT_OPERATIONAL		-6
#define MCCP_RESULT_INVALID_ARGS		-7
#define MCCP_RESULT_NOT_OWNER			-8
#define MCCP_RESULT_NOT_STARTED			-9
#define MCCP_RESULT_TIMEDOUT			-10
#define MCCP_RESULT_ITERATION_HALTED		-11
#define MCCP_RESULT_OUT_OF_RANGE		-12
#define MCCP_RESULT_NAN				-13
#define MCCP_RESULT_ALREADY_HALTED		-14
#define MCCP_RESULT_INVALID_OBJECT		-15
#define MCCP_RESULT_CRITICAL_REGION_NOT_CLOSED	-16
#define MCCP_RESULT_CRITICAL_REGION_NOT_OPENED	-17
#define MCCP_RESULT_INVALID_STATE_TRANSITION	-18
#define MCCP_RESULT_BUSY			-19
#define MCCP_RESULT_STOP			-20
#define MCCP_RESULT_UNSUPPORTED			-21
#define MCCP_RESULT_QUOTE_NOT_CLOSED		-22
#define MCCP_RESULT_NOT_ALLOWED			-23
#define MCCP_RESULT_NOT_DEFINED			-24





__BEGIN_DECLS


/**
 * Get a human readable error message from an API result code.
 *
 *	@param[in]	err	A result code.
 *
 *	@returns	A human readable error message.
 */
const char *	mccp_error_get_string(mccp_result_t err);


__END_DECLS





#endif /* ! __MCCP_ERROR_H__ */
