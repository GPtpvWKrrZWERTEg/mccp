#ifndef __MCCP_LOCK_H__
#define __MCCP_LOCK_H__





/**
 *	@file	mccp_lock.h
 */





typedef struct mccp_mutex_record *	mccp_mutex_t;
typedef struct mccp_rwlock_record *	mccp_rwlock_t;

typedef struct mccp_cond_record *	mccp_cond_t;

typedef struct mccp_barrier_record *	mccp_barrier_t;





__BEGIN_DECLS


mccp_result_t
mccp_mutex_create(mccp_mutex_t *mtxptr);

void
mccp_mutex_destroy(mccp_mutex_t *mtxptr);

mccp_result_t
mccp_mutex_reinitialize(mccp_mutex_t *mtxptr);


mccp_result_t
mccp_mutex_lock(mccp_mutex_t *mtxptr);

mccp_result_t
mccp_mutex_trylock(mccp_mutex_t *mtxptr);

mccp_result_t
mccp_mutex_timedlock(mccp_mutex_t *mtxptr, mccp_chrono_t nsec);


mccp_result_t
mccp_mutex_unlock(mccp_mutex_t *mtxptr);


mccp_result_t
mccp_mutex_enter_critical(mccp_mutex_t *mtxptr);

mccp_result_t
mccp_mutex_leave_critical(mccp_mutex_t *mtxptr);





mccp_result_t
mccp_rwlock_create(mccp_rwlock_t *rwlptr);

void
mccp_rwlock_destroy(mccp_rwlock_t *rwlptr);

mccp_result_t
mccp_rwlock_reinitialize(mccp_rwlock_t *rwlptr);


mccp_result_t
mccp_rwlock_reader_lock(mccp_rwlock_t *rwlptr);

mccp_result_t
mccp_rwlock_reader_trylock(mccp_rwlock_t *rwlptr);

mccp_result_t
mccp_rwlock_reader_timedlock(mccp_rwlock_t *rwlptr,
                             mccp_chrono_t nsec);

mccp_result_t
mccp_rwlock_writer_lock(mccp_rwlock_t *rwlptr);

mccp_result_t
mccp_rwlock_writer_trylock(mccp_rwlock_t *rwlptr);

mccp_result_t
mccp_rwlock_writer_timedlock(mccp_rwlock_t *rwlptr,
                             mccp_chrono_t nsec);

mccp_result_t
mccp_rwlock_unlock(mccp_rwlock_t *rwlptr);


mccp_result_t
mccp_rwlock_reader_enter_critical(mccp_rwlock_t *rwlptr);

mccp_result_t
mccp_rwlock_writer_enter_critical(mccp_rwlock_t *rwlptr);

mccp_result_t
mccp_rwlock_leave_critical(mccp_rwlock_t *rwlptr);





mccp_result_t
mccp_cond_create(mccp_cond_t *cndptr);

void
mccp_cond_destroy(mccp_cond_t *cndptr);

mccp_result_t
mccp_cond_wait(mccp_cond_t *cndptr,
               mccp_mutex_t *mtxptr,
               mccp_chrono_t nsec);

mccp_result_t
mccp_cond_notify(mccp_cond_t *cndptr,
                 bool for_all);





mccp_result_t
mccp_barrier_create(mccp_barrier_t *bptr, size_t n);


void
mccp_barrier_destroy(mccp_barrier_t *bptr);


mccp_result_t
mccp_barrier_wait(mccp_barrier_t *bptr, bool *is_master);


__END_DECLS





#endif /* ! __MCCP_LOCK_H__ */
