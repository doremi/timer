#include <pthread.h>

class Mutex {
public:
    Mutex() {
        pthread_mutex_init(&mMutex, NULL);
    }
    ~Mutex() {
        pthread_mutex_destroy(&mMutex);
    }
    int lock() {
        return pthread_mutex_lock(&mMutex);
    }
    int unlock() {
        return pthread_mutex_unlock(&mMutex);
    }
    int trylock() {
        return pthread_mutex_trylock(&mMutex);
    }
private:
    friend class Condition;
    pthread_mutex_t mMutex;
};

class Condition {
public:
    Condition() {
        pthread_cond_init(&cond, NULL);
    }
    ~Condition() {
        pthread_cond_destroy(&cond);
    }
    int wait(Mutex &mutex) {
        return pthread_cond_wait(&cond, &mutex.mMutex);
    }
    int signal() {
        return pthread_cond_signal(&cond);
    }
private:
    pthread_cond_t cond;
};
