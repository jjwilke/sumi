#ifndef dharma_THREAD_LOCK_H
#define dharma_THREAD_LOCK_H

#include <pthread.h>
#include <dharma/config.h>

namespace dharma {

class mutex_thread_lock
{

 public:
  mutex_thread_lock();

  ~mutex_thread_lock();

  void lock();

  void unlock();

  bool trylock();

  bool locked() const {
    return locked_;
  }

 private:
  pthread_mutex_t mutex_;

  bool locked_;

};

#if DHARMA_USE_SPINLOCK
class spin_thread_lock
{
 public:
  spin_thread_lock();

  ~spin_thread_lock();

  void lock();

  void unlock();

  bool trylock();

  bool locked() const {
    return locked_;
  }

 private:
  pthread_spinlock_t lock_;

  bool locked_;
};
#endif


}

#endif // THREAD_LOCK_H

