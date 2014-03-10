#ifndef __MCCP_SYSDEP_DARWIN_H__
#define __MCCP_SYSDEP_DARWIN_H__





#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME	0
#endif /* ! CLOCK_REALTIME */

#ifndef PTHREAD_BARRIER_SERIAL_THREAD
#define PTHREAD_BARRIER_SERIAL_THREAD	1
#endif /* ! PTHREAD_BARRIER_SERIAL_THREAD */





typedef int clockid_t;

typedef struct darwin_barrier_record * pthread_barrier_t;
typedef int pthread_barrierattr_t;





__BEGIN_DECLS


int	clock_gettime(clockid_t clk_id, struct timespec *tp);

int	pthread_barrier_init(pthread_barrier_t * restrict barrier,
                             const pthread_barrierattr_t * restrict attr,
                             unsigned count); 
int	pthread_barrier_destroy(pthread_barrier_t *barrier);
int	pthread_barrier_wait(pthread_barrier_t *barrier);

int
pthread_mutex_timedlock(pthread_mutex_t *restrict mutex,
                        const struct timespec *restrict abs_timeout);
int
pthread_rwlock_timedwrlock(pthread_rwlock_t *restrict rwlock,
                           const struct timespec *restrict abs_timeout);
int
pthread_rwlock_timedrdlock(pthread_rwlock_t *restrict rwlock,
                           const struct timespec *restrict abs_timeout);


__END_DECLS





#endif /* ! __MCCP_SYSDEP_DARWIN_H__ */
