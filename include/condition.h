#ifndef _CONDITION_H_
#define _CONDITION_H_
//条件变量和互斥量封装一个状态，用于保护线程池的状态

#include <pthread.h>

//封装一个互斥量和条件变量作为状态
typedef struct condition{
    pthread_mutex_t pmutex;
    pthread_cond_t pcond;
}condition_t;

//对状态操作的函数
int condition_init(condition_t* cond);
int condition_lock(condition_t* cond);
int condition_unlock(condition_t* cond);
int condition_wait(condition_t* cond);
int condition_timewait(condition_t* cond, struct timespec* abstime);  //在time.h中
int condition_signal(condition_t* cond);
int condition_broadcast(condition_t* cond);
int condition_destroy(condition_t* cond);

#endif