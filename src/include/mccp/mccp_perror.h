#ifndef __MCCP_PERROR_H__
#define __MCCP_PERROR_H__





#include <mccp/mccp_logger.h>


#ifdef perror
#undef perror
#endif /* perror */
#define perror(str)	mccp_msg_error("%s: %s\n", str, strerror(errno))





#endif /* __MCCP_PERROR_H__ */
