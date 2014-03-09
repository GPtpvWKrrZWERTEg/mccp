#include <mccp/mccp.h>





#include "hash.h"
#include "hash.c"





typedef struct mccp_hashmap_record {
  mccp_hashmap_type_t m_type;
  mccp_rwlock_t m_lock;
  HashTable m_hashtable;
  mccp_hashmap_value_freeup_proc_t m_del_proc;
  ssize_t m_n_entries;
  bool m_is_operational;
} mccp_hashmap_record;





static inline void
s_read_lock(mccp_hashmap_t hm) {
  if (hm != NULL) {
    (void)mccp_rwlock_reader_enter_critical(&(hm->m_lock));
  }
}


static inline void
s_write_lock(mccp_hashmap_t hm) {
  if (hm != NULL) {
    (void)mccp_rwlock_writer_enter_critical(&(hm->m_lock));
  }
}


static inline void
s_unlock(mccp_hashmap_t hm) {
  if (hm != NULL) {
    (void)mccp_rwlock_leave_critical(&(hm->m_lock));
  }
}


static inline bool
s_do_iterate(mccp_hashmap_t hm,
             mccp_hashmap_iteration_proc_t proc, void *arg) {
  bool ret = false;
  if (hm != NULL && proc != NULL) {
    HashSearch s;
    mccp_hashentry_t he;

    for (he = FirstHashEntry(&(hm->m_hashtable), &s);
         he != NULL;
         he = NextHashEntry(&s)) {
      if ((ret = proc(GetHashKey(&(hm->m_hashtable), he),
                      GetHashValue(he),
                      he,
                      arg)) == false) {
        break;
      }
    }
  }
  return ret;
}


static inline mccp_hashentry_t
s_find_entry(mccp_hashmap_t hm, void *key) {
  mccp_hashentry_t ret = NULL;

  if (hm != NULL) {
    ret = FindHashEntry(&(hm->m_hashtable), key);
  }

  return ret;
}


static inline mccp_hashentry_t
s_create_entry(mccp_hashmap_t hm, void *key) {
  mccp_hashentry_t ret = NULL;

  if (hm != NULL) {
    int is_new;
    ret = CreateHashEntry(&(hm->m_hashtable), key, &is_new);
  }

  return ret;
}


static bool
s_freeup_proc(void *key, void *val, mccp_hashentry_t he, void *arg) {
  bool ret = false;
  (void)key;
  (void)he;

  if (arg != NULL) {
    mccp_hashmap_t hm = (mccp_hashmap_t)arg;
    if (hm->m_del_proc != NULL) {
      if (val != NULL) {
        hm->m_del_proc(val);
      }
      ret = true;
    }
  }

  return ret;
}


static inline void
s_freeup_all_values(mccp_hashmap_t hm) {
  s_do_iterate(hm, s_freeup_proc, (void *)hm);
}


static inline void
s_clean(mccp_hashmap_t hm, bool free_values) {
  if (free_values == true) {
    s_freeup_all_values(hm);
  }
  DeleteHashTable(&(hm->m_hashtable));
  (void)memset(&(hm->m_hashtable), 0, sizeof(HashTable));
  hm->m_n_entries = 0;
}


static inline void
s_reinit(mccp_hashmap_t hm, bool free_values) {
  s_clean(hm, free_values);
  InitHashTable(&(hm->m_hashtable), (unsigned int)hm->m_type);
}





void
mccp_hashmap_set_value(mccp_hashentry_t he, void *val) {
  if (he != NULL) {
    SetHashValue(he, val);
  }
}


mccp_result_t
mccp_hashmap_create(mccp_hashmap_t *retptr,
                    mccp_hashmap_type_t t,
                    mccp_hashmap_value_freeup_proc_t proc) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;
  mccp_hashmap_t hm;

  if (retptr != NULL) {
    *retptr = NULL;
    hm = (mccp_hashmap_t)malloc(sizeof(*hm));
    if (hm != NULL) {
      if ((ret = mccp_rwlock_create(&(hm->m_lock))) ==
          MCCP_RESULT_OK) {
        hm->m_type = t;
        InitHashTable(&(hm->m_hashtable), (unsigned int)t);
        hm->m_del_proc = proc;
        hm->m_n_entries = 0;
        hm->m_is_operational = true;
        *retptr = hm;
        ret = MCCP_RESULT_OK;
      } else {
        free((void *)hm);
      }
    } else {
      ret = MCCP_RESULT_NO_MEMORY;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
mccp_hashmap_shutdown(mccp_hashmap_t *hmptr, bool free_values) {
  if (hmptr != NULL &&
      *hmptr != NULL) {

    s_write_lock(*hmptr);
    {
      if ((*hmptr)->m_is_operational == true) {
        (*hmptr)->m_is_operational = false;
        s_clean(*hmptr, free_values);
      }
    }
    s_unlock(*hmptr);

  }
}


void
mccp_hashmap_destroy(mccp_hashmap_t *hmptr, bool free_values) {
  if (hmptr != NULL &&
      *hmptr != NULL) {

    s_write_lock(*hmptr);
    {
      if ((*hmptr)->m_is_operational == true) {
        (*hmptr)->m_is_operational = false;
        s_clean(*hmptr, free_values);
      }
    }
    s_unlock(*hmptr);

    mccp_rwlock_destroy(&((*hmptr)->m_lock));
    free((void *)*hmptr);
    *hmptr = NULL;
  }
}


mccp_result_t
mccp_hashmap_clear(mccp_hashmap_t *hmptr, bool free_values) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {

    s_write_lock(*hmptr);
    {
      if ((*hmptr)->m_is_operational == true) {
        (*hmptr)->m_is_operational = false;
        s_reinit(*hmptr, free_values);
        (*hmptr)->m_is_operational = true;
        ret = MCCP_RESULT_OK;
      } else {
        ret = MCCP_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*hmptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline mccp_result_t
s_find(mccp_hashmap_t *hmptr,
       void *key, void **valptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;
  mccp_hashentry_t he;

  *valptr = NULL;

  if ((*hmptr)->m_is_operational == true) {
    if ((he = s_find_entry(*hmptr, key)) != NULL) {
      *valptr = GetHashValue(he);
      ret = MCCP_RESULT_OK;
    } else {
      ret = MCCP_RESULT_NOT_FOUND;
    }
  } else {
    ret = MCCP_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


mccp_result_t
mccp_hashmap_find_no_lock(mccp_hashmap_t *hmptr,
                          void *key, void **valptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      valptr != NULL) {

    ret = s_find(hmptr, key, valptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_hashmap_find(mccp_hashmap_t *hmptr, void *key, void **valptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      valptr != NULL) {

    s_read_lock(*hmptr);
    {
      ret = s_find(hmptr, key, valptr);
    }
    s_unlock(*hmptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline mccp_result_t
s_add(mccp_hashmap_t *hmptr,
      void *key, void **valptr,
      bool allow_overwrite) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;
  void *oldval = NULL;
  mccp_hashentry_t he;

  if ((*hmptr)->m_is_operational == true) {
    if ((he = s_find_entry(*hmptr, key)) != NULL) {
      oldval = GetHashValue(he);
      if (allow_overwrite == true) {
        SetHashValue(he, *valptr);
        ret = MCCP_RESULT_OK;
      } else {
        ret = MCCP_RESULT_ALREADY_EXISTS;
      }
    } else {
      he = s_create_entry(*hmptr, key);
      if (he != NULL) {
        SetHashValue(he, *valptr);
        (*hmptr)->m_n_entries++;
        ret = MCCP_RESULT_OK;
      } else {
        ret = MCCP_RESULT_NO_MEMORY;
      }
    }
    *valptr = oldval;
  } else {
    ret = MCCP_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


mccp_result_t
mccp_hashmap_add(mccp_hashmap_t *hmptr,
                 void *key, void **valptr,
                 bool allow_overwrite) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      valptr != NULL) {

    s_write_lock(*hmptr);
    {
      ret = s_add(hmptr, key, valptr, allow_overwrite);
    }
    s_unlock(*hmptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_hashmap_add_no_lock(mccp_hashmap_t *hmptr,
                         void *key, void **valptr,
                         bool allow_overwrite) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      valptr != NULL) {

    ret = s_add(hmptr, key, valptr, allow_overwrite);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline mccp_result_t
s_delete(mccp_hashmap_t *hmptr,
         void *key, void **valptr,
         bool free_value) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;
  void *val = NULL;
  mccp_hashentry_t he;

  if ((*hmptr)->m_is_operational == true) {
    if ((he = s_find_entry(*hmptr, key)) != NULL) {
      val = GetHashValue(he);
      if (val != NULL &&
          free_value == true &&
          (*hmptr)->m_del_proc != NULL) {
        (*hmptr)->m_del_proc(val);
      }
      DeleteHashEntry(he);
      (*hmptr)->m_n_entries--;
    }
    ret = MCCP_RESULT_OK;
  } else {
    ret = MCCP_RESULT_NOT_OPERATIONAL;
  }

  if (valptr != NULL) {
    *valptr = val;
  }

  return ret;
}


mccp_result_t
mccp_hashmap_delete(mccp_hashmap_t *hmptr,
                    void *key, void **valptr,
                    bool free_value) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {

    s_write_lock(*hmptr);
    {
      ret = s_delete(hmptr, key, valptr, free_value);
    }
    s_unlock(*hmptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_hashmap_delete_no_lock(mccp_hashmap_t *hmptr,
                            void *key, void **valptr,
                            bool free_value) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {

    ret = s_delete(hmptr, key, valptr, free_value);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline mccp_result_t
s_iterate(mccp_hashmap_t *hmptr,
          mccp_hashmap_iteration_proc_t proc,
          void *arg) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if ((*hmptr)->m_is_operational == true) {
    if (s_do_iterate(*hmptr, proc, arg) == true) {
      ret = MCCP_RESULT_OK;
    } else {
      ret = MCCP_RESULT_ITERATION_HALTED;
    }
  } else {
    ret = MCCP_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


mccp_result_t
mccp_hashmap_iterate(mccp_hashmap_t *hmptr,
                     mccp_hashmap_iteration_proc_t proc,
                     void *arg) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      proc != NULL) {

    /*
     * The proc could modify hash values so we use write lock.
     */
    s_write_lock(*hmptr);
    {
      ret = s_iterate(hmptr, proc, arg);
    }
    s_unlock(*hmptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_hashmap_iterate_no_lock(mccp_hashmap_t *hmptr,
                             mccp_hashmap_iteration_proc_t proc,
                             void *arg) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      proc != NULL) {

    ret = s_iterate(hmptr, proc, arg);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}





mccp_result_t
mccp_hashmap_size(mccp_hashmap_t *hmptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {

    s_read_lock(*hmptr);
    {
      if ((*hmptr)->m_is_operational == true) {
        ret = (*hmptr)->m_n_entries;
      } else {
        ret = MCCP_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*hmptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_hashmap_size_no_lock(mccp_hashmap_t *hmptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {

    if ((*hmptr)->m_is_operational == true) {
      ret = (*hmptr)->m_n_entries;
    } else {
      ret = MCCP_RESULT_NOT_OPERATIONAL;
    }

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


mccp_result_t
mccp_hashmap_statistics(mccp_hashmap_t *hmptr, const char **msgptr) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      msgptr != NULL) {

    *msgptr = NULL;

    s_read_lock(*hmptr);
    {
      if ((*hmptr)->m_is_operational == true) {
        *msgptr = (const char *)HashStats(&((*hmptr)->m_hashtable));
        if (*msgptr != NULL) {
          ret = MCCP_RESULT_OK;
        } else {
          ret = MCCP_RESULT_NO_MEMORY;
        }
      } else {
        ret = MCCP_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*hmptr);

  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
mccp_hashmap_atfork_child(mccp_hashmap_t *hmptr) {
  if (hmptr != NULL &&
      *hmptr != NULL) {
    mccp_rwlock_reinitialize(&((*hmptr)->m_lock));
  }
}
