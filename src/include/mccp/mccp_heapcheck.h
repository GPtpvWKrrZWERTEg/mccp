#ifndef __MCCP_HEAPCHECK_H__
#define __MCCP_HEAPCHECK_H__





__BEGIN_DECLS


void
mccp_heapcheck_module_initialize(void);

bool
mccp_heapcheck_is_in_heap(const void *addr);

#if 0
bool
mccp_heapcheck_is_mallocd(const void *addr);
#endif


__END_DECLS





#endif /* ! __MCCP_HEAPCHECK_H__ */
