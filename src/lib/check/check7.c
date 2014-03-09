#include <mccp/mccp.h>
#include <mccp/mccp_thread_internal.h>





#define NTHDS	8





typedef struct a_obj_record {
  mccp_mutex_t m_lock;
  bool m_bool;
} a_obj_record;
typedef a_obj_record *a_obj_t;


typedef struct null_thread_record {
  mccp_thread_record m_thd;
  mccp_mutex_t m_lock;
  a_obj_t m_o;
} null_thread_record;
typedef null_thread_record 	*null_thread_t;





static null_thread_record s_thds[NTHDS];





static inline a_obj_t
a_obj_create(void) {
  a_obj_t ret = (a_obj_t)malloc(sizeof(*ret));

  if (ret != NULL) {
    if (mccp_mutex_create(&(ret->m_lock)) == MCCP_RESULT_OK) {
      ret->m_bool = false;
    } else {
      free((void *)ret);
      ret = NULL;
    }
  }

  return ret;
}


static inline void
a_obj_destroy(a_obj_t o) {
  mccp_msg_debug(1, "enter.\n");

  if (o != NULL) {
    if (o->m_lock != NULL) {
      (void)mccp_mutex_lock(&(o->m_lock));
      (void)mccp_mutex_unlock(&(o->m_lock));
      (void)mccp_mutex_destroy(&(o->m_lock));
    }
    free((void *)o);
  }

  mccp_msg_debug(1, "leave.\n");
}


static inline bool
a_obj_get(a_obj_t o) {
  bool ret = false;

  if (o != NULL) {

    (void)mccp_mutex_enter_critical(&(o->m_lock));
    {
      ret = o->m_bool;
      /*
       * fprintf() is a cancel point.
       */
      fprintf(stderr, "%s:%d:%s: called.\n",
              __FILE__, __LINE__, __func__);
    }
    (void)mccp_mutex_leave_critical(&(o->m_lock));

  }

  return ret;
}


static inline void
a_obj_set(a_obj_t o, bool val) {
  if (o != NULL) {

    (void)mccp_mutex_enter_critical(&(o->m_lock));
    {
      o->m_bool = val;
      /*
       * fprintf() is a cancel point.
       */
      fprintf(stderr, "%s:%d:%s:0x" PFTIDS(016, x) ": called.\n",
              __FILE__, __LINE__, __func__, pthread_self());
    }
    (void)mccp_mutex_leave_critical(&(o->m_lock));

  }
}





static mccp_result_t
s_main(const mccp_thread_t *tptr, void *arg) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  (void)arg;

  if (tptr != NULL) {
    null_thread_t nptr = (null_thread_t)*tptr;
    a_obj_t o = nptr->m_o;

    if (nptr != NULL &&
        o != NULL) {

      while (true) {

        (void)mccp_mutex_enter_critical(&(nptr->m_lock));
        {
          /*
           * fprintf() is a cancel point.
           */
          fprintf(stderr, "%s:%d:%s:0x" PFTIDS(016, x) ": sleep one sec.\n",
                  __FILE__, __LINE__, __func__, pthread_self());
          sleep(1);
        }
        (void)mccp_mutex_leave_critical(&(nptr->m_lock));

        a_obj_set(o, true);

      }

      ret = MCCP_RESULT_OK;

    } else {
      ret = MCCP_RESULT_INVALID_ARGS;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

  return ret;
}



static void
s_finalize(const mccp_thread_t *tptr, bool is_canceled, void *arg) {
  (void)arg;
  (void)tptr;

  mccp_msg_debug(1, "called. %s.\n",
                 (is_canceled == false) ? "finished" : "canceled");
}


static inline mccp_result_t
s_create(null_thread_t *nptr, a_obj_t o) {
  mccp_result_t ret = MCCP_RESULT_ANY_FAILURES;

  if (nptr != NULL) {
    if ((ret = mccp_thread_create((mccp_thread_t *)nptr,
                                  s_main, s_finalize, NULL,
                                  "waiter", NULL)) == MCCP_RESULT_OK) {
      if ((ret = mccp_mutex_create(&((*nptr)->m_lock))) ==
          MCCP_RESULT_OK) {
        (*nptr)->m_o = o;
      } else {
        mccp_perror(ret, "mccp_mutex_create()");
        goto done;
      }
    } else {
      mccp_perror(ret, "mccp_thread_create()");
      goto done;
    }
  } else {
    ret = MCCP_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}





int
main(int argc, const char *const argv[]) {
  int ret = 1;
  mccp_result_t r;
  null_thread_t nt;
  size_t i;
  a_obj_t o;

  (void)argc;
  (void)argv;

  o = a_obj_create();
  if (o == NULL) {
    goto done;
  }

  for (i = 0; i < NTHDS; i++) {
    nt = &s_thds[i];
    if ((r = s_create(&nt, o)) != MCCP_RESULT_OK) {
      mccp_perror(r, "s_create()");
      goto done;
    }
  }

  for (i = 0; i < NTHDS; i++) {
    nt = &s_thds[i];
    if ((r = mccp_thread_start((mccp_thread_t *)&nt, false)) !=
        MCCP_RESULT_OK) {
      mccp_perror(r, "mccp_thread_start()");
      goto done;
    }
  }

  sleep(1);

  /*
   * Cancel all the thread.
   */
  for (i = 0; i < NTHDS; i++) {
    nt = &s_thds[i];
    if ((r = mccp_thread_cancel((mccp_thread_t *)&nt)) !=
        MCCP_RESULT_OK) {
      mccp_perror(r, "mccp_thread_cancel()");
      goto done;
    }
  }

  /*
   * Destroy all the thread. If any deadlocks occur, we failed.
   */
  for (i = 0; i < NTHDS; i++) {
    nt = &s_thds[i];
    mccp_thread_destroy((mccp_thread_t *)&nt);
  }

  /*
   * And destroy a_obj. If a deadlock occurs, we failed.
   */
  a_obj_destroy(o);

  ret = 0;

done:
  return ret;
}
