#include "threadpool.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

//创建的线程执行(线程执行的函数)
void* thread_routine(void* arg)
{
    struct timespec abstime;  //获取此时的绝对时间
    int timeout;
    printf("thread %d is starting!\n",(int)pthread_self());
    threadpool_t* pool = (threadpool_t*) arg;
    while(1)
    {
        timeout = 0;
        //访问线程池之前需要先加锁
        condition_lock(&pool->ready);
        //空闲
        pool->idle++;
        //等待队列有任务到来 或者 收到线程池销毁的通知
        while(pool->first == NULL && !pool->quit)  //当没有任务且不退出时
        {
            //线程阻塞等待
            printf("thread %d is waiting\n",(int)pthread_self());
            //获取当前时间并存储在abstime中
            clock_gettime(CLOCK_REALTIME,&abstime); 
            //设置超时睡眠时间
            abstime.tv_sec += 2;
            int status;
            status = condition_timewait(&pool->ready, &abstime);
            if(status == ETIMEDOUT)  //ETIMEOUT连接超时时间
            {
                printf("thread %d wait timed out\n",(int)pthread_self());
                timeout = 1;
                break;
            }
        }

        //否则就是有任务或者销毁线程池
        pool->idle--;
        //如果是有任务到来
        if(pool->first != NULL)
        {
            //取出队列最前面的任务并移出队列，执行任务
            task_t* t = pool->first;
            pool->first = t->next;
            //由于执行任务需要消耗时间，先解锁让其他线程访问线程池
            condition_unlock(&pool->ready);
            //执行任务
            t->run(t->arg);
            //执行完释放内存
            free(t);
            //重新加锁
            condition_lock(&pool->ready);
        }

        //如果是退出线程池
        if(pool->quit && pool->first == NULL)
        {
            pool->counter--;   //当前工作的线程数减1
            //若线程池中没有线程，通知等待线程（主线程）全部任务已经完成
            if(pool->counter == 0)
            {
                condition_signal(&pool->ready);
            }
            condition_unlock(&pool->ready);
            break;
        }

        //超时，跳出销毁线程
        if(timeout == 1)
        {
            pool->counter--; //当前工作的线程数-1
            condition_unlock(&pool->ready);
            break;
        }

        condition_unlock(&pool->ready);
    }
    printf("thread %d is exiting\n",(int)pthread_self());
    return NULL;
}

//线程池初始化
void threadpool_init(threadpool_t *pool, int threads)
{
    
    condition_init(&pool->ready);
    pool->first = NULL;
    pool->last =NULL;
    pool->counter =0;
    pool->idle =0;
    pool->max_threads = threads;
    pool->quit =0;    
}

//增加一个任务到线程池
void threadpool_add_task(threadpool_t* pool, void* (*run)(void* arg), void* arg)
{
    //产生一个新任务
    task_t* newtask = (task_t*)malloc(sizeof(task_t));
    newtask->run = run;
    newtask->arg = arg;
    newtask->next = NULL;//新增加的线程放在队列尾端

    //线程池的状态被多个线程共享，操作前需要加锁
    condition_lock(&pool->ready);

    if(pool->first == NULL)  //如果原本任务队列中没有任务，则添加到任务队列头部
    {
        pool->first = newtask;
    }
    else{
        pool->last->next = newtask;//否则添加到任务的尾部的后一个
    }
    pool->last = newtask;//然后将队列尾指向新加入的线程

    //线程池中有线程空闲，唤醒
    if(pool->idle > 0)
    {
        condition_signal(&pool->ready);
    }

    //当前线程池中线程个数没有达到设定的最大值并且没有空闲的线程，创建一个新线程
    else if(pool->counter < pool->max_threads)
    {
        pthread_t tid;
        pthread_create(&tid,NULL,thread_routine,pool);
        pool->counter++;
    }

    //结束访问
    condition_unlock(&pool->ready);
}

//线程池销毁
void threadpool_destroy(threadpool_t *pool)
{
    //如果已经调用销毁，直接返回
    if(pool->quit)
    {
    return;
    }
    //加锁
    condition_lock(&pool->ready);
    //设置销毁标记为1
    pool->quit = 1;
    //线程池中线程个数大于0
    if(pool->counter > 0)
    {
        //对于等待的线程，发送信号唤醒
        if(pool->idle > 0)
        {
            condition_broadcast(&pool->ready);
        }
        //正在执行任务的线程，等待他们结束任务
        while(pool->counter)
        {
            condition_wait(&pool->ready);
        }
    }
    condition_unlock(&pool->ready);
    condition_destroy(&pool->ready);
}
