#include "debug.h"

#include "def.h"

#include "sys.h"
#include "opt.h"
#include "stats.h"
#include "tcpip.h"

#if SYS_LIGHTWEIGHT_PROT
//static pthread_mutex_t lwprot_mutex = PTHREAD_MUTEX_INITIALIZER;
//static pthread_t lwprot_thread = (pthread_t)0xDEAD;
//static int lwprot_count = 0;
#endif /* SYS_LIGHTWEIGHT_PROT */

/*-----------------------------------------------------------------------------------*/
/* Time */
#include "time.h"

static void get_monotonic_time(struct timespec *ts) {
  clock_gettime(CLOCK_MONOTONIC, ts);
}

u32_t lw_sys_now(void) {
  struct timespec ts;

  get_monotonic_time(&ts);
  return (u32_t)(ts.tv_sec * 1000L + ts.tv_nsec / 1000000L);
}

u32_t
lw_sys_jiffies(void)
{
  struct timespec ts;

  get_monotonic_time(&ts);
  return (u32_t)(ts.tv_sec * 1000000000L + ts.tv_nsec);
}

/*-----------------------------------------------------------------------------------*/
/* Init */
void lw_sys_init(void) { }

#if 0







#if !NO_SYS

static struct sys_thread *threads = NULL;
static pthread_mutex_t threads_mutex = PTHREAD_MUTEX_INITIALIZER;

struct lw_sys_mbox_msg {
  struct lw_sys_mbox_msg *next;
  void *msg;
};

#define lw_sys_mbox_SIZE 128

struct lw_sys_mbox {
  int first, last;
  void *msgs[lw_sys_mbox_SIZE];
  struct sys_sem *not_empty;
  struct sys_sem *not_full;
  struct sys_sem *mutex;
  int wait_send;
};

struct sys_sem {
  unsigned int c;
  pthread_condattr_t condattr;
  pthread_cond_t cond;
  pthread_mutex_t mutex;
};

struct sys_mutex {
  pthread_mutex_t mutex;
};

struct sys_thread {
  struct sys_thread *next;
  pthread_t pthread;
};

static struct sys_sem *lw_sys_sem_new_internal(u8_t count);
static void lw_sys_sem_free_internal(struct sys_sem *sem);

static u32_t cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex,
                       u32_t timeout);

/*-----------------------------------------------------------------------------------*/
/* Threads */
static struct sys_thread * 
introduce_thread(pthread_t id)
{
  struct sys_thread *thread;

  thread = (struct sys_thread *)malloc(sizeof(struct sys_thread));

  if (thread != NULL) {
    pthread_mutex_lock(&threads_mutex);
    thread->next = threads;
    thread->pthread = id;
    threads = thread;
    pthread_mutex_unlock(&threads_mutex);
  }

  return thread;
}

struct thread_wrapper_data
{
  lwip_thread_fn function;
  void *arg;
};

static void *
thread_wrapper(void *arg)
{
  struct thread_wrapper_data *thread_data = (struct thread_wrapper_data *)arg;

  thread_data->function(thread_data->arg);

  /* we should never get here */
  free(arg);
  return NULL;
}

sys_thread_t
lw_sys_thread_new(const char *name, lwip_thread_fn function, void *arg, int stacksize, int prio)
{
  int code;
  pthread_t tmp;
  struct sys_thread *st = NULL;
  struct thread_wrapper_data *thread_data;
  LWIP_UNUSED_ARG(name);
  LWIP_UNUSED_ARG(stacksize);
  LWIP_UNUSED_ARG(prio);

  thread_data = (struct thread_wrapper_data *)malloc(sizeof(struct thread_wrapper_data));
  thread_data->arg = arg;
  thread_data->function = function;
  code = pthread_create(&tmp,
                        NULL, 
                        thread_wrapper, 
                        thread_data);
  
  if (0 == code) {
    st = introduce_thread(tmp);
  }

  if (NULL == st) {
    LWIP_DEBUGF(SYS_DEBUG, ("lw_sys_thread_new: pthread_create %d, st = 0x%lx",
                       code, (unsigned long)st));
    abort();
  }
  return st;
}

#if LWIP_TCPIP_CORE_LOCKING
static pthread_t lwip_core_lock_holder_thread_id;
void sys_lock_tcpip_core(void)
{
  lw_sys_mutex_lock(&lock_tcpip_core);
  lwip_core_lock_holder_thread_id = pthread_self();
}

void sys_unlock_tcpip_core(void)
{
  lwip_core_lock_holder_thread_id = 0;
  lw_sys_mutex_unlock(&lock_tcpip_core);
}
#endif /* LWIP_TCPIP_CORE_LOCKING */

static pthread_t lwip_tcpip_thread_id;
void sys_mark_tcpip_thread(void)
{
  lwip_tcpip_thread_id = pthread_self();
}

void sys_check_core_locking(void)
{
  /* Embedded systems should check we are NOT in an interrupt context here */

  if (lwip_tcpip_thread_id != 0) {
    pthread_t current_thread_id = pthread_self();

#if LWIP_TCPIP_CORE_LOCKING
    LWIP_ASSERT("Function called without core lock", current_thread_id == lwip_core_lock_holder_thread_id);
#else /* LWIP_TCPIP_CORE_LOCKING */
    LWIP_ASSERT("Function called from wrong thread", current_thread_id == lwip_tcpip_thread_id);
#endif /* LWIP_TCPIP_CORE_LOCKING */
  }
}

/*-----------------------------------------------------------------------------------*/
/* Mailbox */
err_t
lw_sys_mbox_new(struct lw_sys_mbox **mb, int size)
{
  struct lw_sys_mbox *mbox;
  LWIP_UNUSED_ARG(size);

  mbox = (struct lw_sys_mbox *)malloc(sizeof(struct lw_sys_mbox));
  if (mbox == NULL) {
    return ERR_MEM;
  }
  mbox->first = mbox->last = 0;
  mbox->not_empty = lw_sys_sem_new_internal(0);
  mbox->not_full = lw_sys_sem_new_internal(0);
  mbox->mutex = lw_sys_sem_new_internal(1);
  mbox->wait_send = 0;

  SYS_STATS_INC_USED(mbox);
  *mb = mbox;
  return ERR_OK;
}

void
lw_sys_mbox_free(struct lw_sys_mbox **mb)
{
  if ((mb != NULL) && (*mb != lw_sys_mbox_NULL)) {
    struct lw_sys_mbox *mbox = *mb;
    SYS_STATS_DEC(mbox.used);
    lw_sys_arch_sem_wait(&mbox->mutex, 0);
    
    lw_sys_sem_free_internal(mbox->not_empty);
    lw_sys_sem_free_internal(mbox->not_full);
    lw_sys_sem_free_internal(mbox->mutex);
    mbox->not_empty = mbox->not_full = mbox->mutex = NULL;
    /*  LWIP_DEBUGF("lw_sys_mbox_free: mbox 0x%lx\n", mbox); */
    free(mbox);
  }
}

err_t
lw_sys_mbox_trypost(struct lw_sys_mbox **mb, void *msg)
{
  u8_t first;
  struct lw_sys_mbox *mbox;
  LWIP_ASSERT("invalid mbox", (mb != NULL) && (*mb != NULL));
  mbox = *mb;

  lw_sys_arch_sem_wait(&mbox->mutex, 0);

  LWIP_DEBUGF(SYS_DEBUG, ("lw_sys_mbox_trypost: mbox %p msg %p\n",
                          (void *)mbox, (void *)msg));

  if ((mbox->last + 1) >= (mbox->first + lw_sys_mbox_SIZE)) {
    lw_sys_sem_signal(&mbox->mutex);
    return ERR_MEM;
  }

  mbox->msgs[mbox->last % lw_sys_mbox_SIZE] = msg;

  if (mbox->last == mbox->first) {
    first = 1;
  } else {
    first = 0;
  }

  mbox->last++;

  if (first) {
    lw_sys_sem_signal(&mbox->not_empty);
  }

  lw_sys_sem_signal(&mbox->mutex);

  return ERR_OK;
}

err_t
lw_sys_mbox_trypost_fromisr(lw_sys_mbox_t *q, void *msg)
{
  return lw_sys_mbox_trypost(q, msg);
}

void
lw_sys_mbox_post(struct lw_sys_mbox **mb, void *msg)
{
  u8_t first;
  struct lw_sys_mbox *mbox;
  LWIP_ASSERT("invalid mbox", (mb != NULL) && (*mb != NULL));
  mbox = *mb;

  lw_sys_arch_sem_wait(&mbox->mutex, 0);

  LWIP_DEBUGF(SYS_DEBUG, ("lw_sys_mbox_post: mbox %p msg %p\n", (void *)mbox, (void *)msg));

  while ((mbox->last + 1) >= (mbox->first + lw_sys_mbox_SIZE)) {
    mbox->wait_send++;
    lw_sys_sem_signal(&mbox->mutex);
    lw_sys_arch_sem_wait(&mbox->not_full, 0);
    lw_sys_arch_sem_wait(&mbox->mutex, 0);
    mbox->wait_send--;
  }

  mbox->msgs[mbox->last % lw_sys_mbox_SIZE] = msg;

  if (mbox->last == mbox->first) {
    first = 1;
  } else {
    first = 0;
  }

  mbox->last++;

  if (first) {
    lw_sys_sem_signal(&mbox->not_empty);
  }

  lw_sys_sem_signal(&mbox->mutex);
}

u32_t
lw_sys_arch_mbox_tryfetch(struct lw_sys_mbox **mb, void **msg)
{
  struct lw_sys_mbox *mbox;
  LWIP_ASSERT("invalid mbox", (mb != NULL) && (*mb != NULL));
  mbox = *mb;

  lw_sys_arch_sem_wait(&mbox->mutex, 0);

  if (mbox->first == mbox->last) {
    lw_sys_sem_signal(&mbox->mutex);
    return lw_sys_mbox_EMPTY;
  }

  if (msg != NULL) {
    LWIP_DEBUGF(SYS_DEBUG, ("lw_sys_mbox_tryfetch: mbox %p msg %p\n", (void *)mbox, *msg));
    *msg = mbox->msgs[mbox->first % lw_sys_mbox_SIZE];
  }
  else{
    LWIP_DEBUGF(SYS_DEBUG, ("lw_sys_mbox_tryfetch: mbox %p, null msg\n", (void *)mbox));
  }

  mbox->first++;

  if (mbox->wait_send) {
    lw_sys_sem_signal(&mbox->not_full);
  }

  lw_sys_sem_signal(&mbox->mutex);

  return 0;
}

u32_t
lw_sys_arch_mbox_fetch(struct lw_sys_mbox **mb, void **msg, u32_t timeout)
{
  u32_t time_needed = 0;
  struct lw_sys_mbox *mbox;
  LWIP_ASSERT("invalid mbox", (mb != NULL) && (*mb != NULL));
  mbox = *mb;

  /* The mutex lock is quick so we don't bother with the timeout
     stuff here. */
  lw_sys_arch_sem_wait(&mbox->mutex, 0);

  while (mbox->first == mbox->last) {
    lw_sys_sem_signal(&mbox->mutex);

    /* We block while waiting for a mail to arrive in the mailbox. We
       must be prepared to timeout. */
    if (timeout != 0) {
      time_needed = lw_sys_arch_sem_wait(&mbox->not_empty, timeout);

      if (time_needed == LW_SYS_ARCH_TIMEOUT) {
        return LW_SYS_ARCH_TIMEOUT;
      }
    } else {
      lw_sys_arch_sem_wait(&mbox->not_empty, 0);
    }

    lw_sys_arch_sem_wait(&mbox->mutex, 0);
  }

  if (msg != NULL) {
    LWIP_DEBUGF(SYS_DEBUG, ("lw_sys_mbox_fetch: mbox %p msg %p\n", (void *)mbox, *msg));
    *msg = mbox->msgs[mbox->first % lw_sys_mbox_SIZE];
  }
  else{
    LWIP_DEBUGF(SYS_DEBUG, ("lw_sys_mbox_fetch: mbox %p, null msg\n", (void *)mbox));
  }

  mbox->first++;

  if (mbox->wait_send) {
    lw_sys_sem_signal(&mbox->not_full);
  }

  lw_sys_sem_signal(&mbox->mutex);

  return time_needed;
}

/*-----------------------------------------------------------------------------------*/
/* Semaphore */
static struct sys_sem *
lw_sys_sem_new_internal(u8_t count)
{
  struct sys_sem *sem;

  sem = (struct sys_sem *)malloc(sizeof(struct sys_sem));
  if (sem != NULL) {
    sem->c = count;
    pthread_condattr_init(&(sem->condattr));
#if !(defined(LWIP_UNIX_MACH) || (defined(LWIP_UNIX_ANDROID) && __ANDROID_API__ < 21))
    pthread_condattr_setclock(&(sem->condattr), CLOCK_MONOTONIC);
#endif
    pthread_cond_init(&(sem->cond), &(sem->condattr));
    pthread_mutex_init(&(sem->mutex), NULL);
  }
  return sem;
}

err_t
lw_sys_sem_new(struct sys_sem **sem, u8_t count)
{
  SYS_STATS_INC_USED(sem);
  *sem = lw_sys_sem_new_internal(count);
  if (*sem == NULL) {
    return ERR_MEM;
  }
  return ERR_OK;
}

static u32_t
cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex, u32_t timeout)
{
  struct timespec rtime1, rtime2, ts;
  int ret;

#ifdef __GNU__
  #define pthread_cond_wait pthread_hurd_cond_wait_np
  #define pthread_cond_timedwait pthread_hurd_cond_timedwait_np
#endif

  if (timeout == 0) {
    pthread_cond_wait(cond, mutex);
    return 0;
  }

  /* Get a timestamp and add the timeout value. */
  get_monotonic_time(&rtime1);
#if defined(LWIP_UNIX_MACH) || (defined(LWIP_UNIX_ANDROID) && __ANDROID_API__ < 21)
  ts.tv_sec = timeout / 1000L;
  ts.tv_nsec = (timeout % 1000L) * 1000000L;
  ret = pthread_cond_timedwait_relative_np(cond, mutex, &ts);
#else
  ts.tv_sec = rtime1.tv_sec + timeout / 1000L;
  ts.tv_nsec = rtime1.tv_nsec + (timeout % 1000L) * 1000000L;
  if (ts.tv_nsec >= 1000000000L) {
    ts.tv_sec++;
    ts.tv_nsec -= 1000000000L;
  }

  ret = pthread_cond_timedwait(cond, mutex, &ts);
#endif
  if (ret == ETIMEDOUT) {
    return LW_SYS_ARCH_TIMEOUT;
  }

  /* Calculate for how long we waited for the cond. */
  get_monotonic_time(&rtime2);
  ts.tv_sec = rtime2.tv_sec - rtime1.tv_sec;
  ts.tv_nsec = rtime2.tv_nsec - rtime1.tv_nsec;
  if (ts.tv_nsec < 0) {
    ts.tv_sec--;
    ts.tv_nsec += 1000000000L;
  }
  return (u32_t)(ts.tv_sec * 1000L + ts.tv_nsec / 1000000L);
}

u32_t
lw_sys_arch_sem_wait(struct sys_sem **s, u32_t timeout)
{
  u32_t time_needed = 0;
  struct sys_sem *sem;
  LWIP_ASSERT("invalid sem", (s != NULL) && (*s != NULL));
  sem = *s;

  pthread_mutex_lock(&(sem->mutex));
  while (sem->c <= 0) {
    if (timeout > 0) {
      time_needed = cond_wait(&(sem->cond), &(sem->mutex), timeout);

      if (time_needed == LW_SYS_ARCH_TIMEOUT) {
        pthread_mutex_unlock(&(sem->mutex));
        return LW_SYS_ARCH_TIMEOUT;
      }
      /*      pthread_mutex_unlock(&(sem->mutex));
              return time_needed; */
    } else {
      cond_wait(&(sem->cond), &(sem->mutex), 0);
    }
  }
  sem->c--;
  pthread_mutex_unlock(&(sem->mutex));
  return (u32_t)time_needed;
}

void
lw_sys_sem_signal(struct sys_sem **s)
{
  struct sys_sem *sem;
  LWIP_ASSERT("invalid sem", (s != NULL) && (*s != NULL));
  sem = *s;

  pthread_mutex_lock(&(sem->mutex));
  sem->c++;

  if (sem->c > 1) {
    sem->c = 1;
  }

  pthread_cond_broadcast(&(sem->cond));
  pthread_mutex_unlock(&(sem->mutex));
}

static void
lw_sys_sem_free_internal(struct sys_sem *sem)
{
  pthread_cond_destroy(&(sem->cond));
  pthread_condattr_destroy(&(sem->condattr));
  pthread_mutex_destroy(&(sem->mutex));
  free(sem);
}

void
lw_sys_sem_free(struct sys_sem **sem)
{
  if ((sem != NULL) && (*sem != SYS_SEM_NULL)) {
    SYS_STATS_DEC(sem.used);
    lw_sys_sem_free_internal(*sem);
  }
}

/*-----------------------------------------------------------------------------------*/
/* Mutex */
/** Create a new mutex
 * @param mutex pointer to the mutex to create
 * @return a new mutex */
err_t
lw_sys_mutex_new(struct sys_mutex **mutex)
{
  struct sys_mutex *mtx;

  mtx = (struct sys_mutex *)malloc(sizeof(struct sys_mutex));
  if (mtx != NULL) {
    pthread_mutex_init(&(mtx->mutex), NULL);
    *mutex = mtx;
    return ERR_OK;
  }
  else {
    return ERR_MEM;
  }
}

/** Lock a mutex
 * @param mutex the mutex to lock */
void
lw_sys_mutex_lock(struct sys_mutex **mutex)
{
  pthread_mutex_lock(&((*mutex)->mutex));
}

/** Unlock a mutex
 * @param mutex the mutex to unlock */
void
lw_sys_mutex_unlock(struct sys_mutex **mutex)
{
  pthread_mutex_unlock(&((*mutex)->mutex));
}

/** Delete a mutex
 * @param mutex the mutex to delete */
void
lw_sys_mutex_free(struct sys_mutex **mutex)
{
  pthread_mutex_destroy(&((*mutex)->mutex));
  free(*mutex);
}

#endif /* !NO_SYS */




/*-----------------------------------------------------------------------------------*/
/* Critical section */
#if SYS_LIGHTWEIGHT_PROT
/** sys_prot_t lw_sys_arch_protect(void)

This optional function does a "fast" critical region protection and returns
the previous protection level. This function is only called during very short
critical regions. An embedded system which supports ISR-based drivers might
want to implement this function by disabling interrupts. Task-based systems
might want to implement this by using a mutex or disabling tasking. This
function should support recursive calls from the same task or interrupt. In
other words, lw_sys_arch_protect() could be called while already protected. In
that case the return value indicates that it is already protected.

lw_sys_arch_protect() is only required if your port is supporting an operating
system.
*/
sys_prot_t
lw_sys_arch_protect(void)
{
    /* Note that for the UNIX port, we are using a lightweight mutex, and our
     * own counter (which is locked by the mutex). The return code is not actually
     * used. */
    if (lwprot_thread != pthread_self())
    {
        /* We are locking the mutex where it has not been locked before *
        * or is being locked by another thread */
        pthread_mutex_lock(&lwprot_mutex);
        lwprot_thread = pthread_self();
        lwprot_count = 1;
    }
    else
        /* It is already locked by THIS thread */
        lwprot_count++;
    return 0;
}

/** void lw_sys_arch_unprotect(sys_prot_t pval)

This optional function does a "fast" set of critical region protection to the
value specified by pval. See the documentation for lw_sys_arch_protect() for
more information. This function is only required if your port is supporting
an operating system.
*/
void
lw_sys_arch_unprotect(sys_prot_t pval)
{
    LWIP_UNUSED_ARG(pval);
    if (lwprot_thread == pthread_self())
    {
        lwprot_count--;
        if (lwprot_count == 0)
        {
            lwprot_thread = (pthread_t) 0xDEAD;
            pthread_mutex_unlock(&lwprot_mutex);
        }
    }
}
#endif /* SYS_LIGHTWEIGHT_PROT */

#endif
