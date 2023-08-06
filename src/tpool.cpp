/* -------------------------------------------------------------------------
 * tpool.cpp - thread pool functions
 * Copyright (C) 2008 Dimitar Atanasov <datanasov@deisytechbg.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * -------------------------------------------------------------------------
 */

/*
 * most of the theory and implementation of the thread pool was taken
 * from the o'reilly pthreads programming book.
 */

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h> /* strerror() */
#include  <pthread.h>
#include "common.h"
#include "tpool.h"
#include "log.h"

extern log_t *logn;

/* the worker thread */
void *tpool_thread(void *tpool);

tpool_t::tpool_t(int threads_count, int queue)
{
    int i, rtn;

    /* set the desired thread pool values */
    num_threads = threads_count;
    max_queue_size = queue;
    do_not_block_when_full = POOL_BLOCK_ONFULL;

    if((threads = new pthread_t[num_threads]) == NULL)
    {
        logger( FATAL,"Unable to allocate thread info array\n");
    }

    /* initialize the work queue */
    cur_queue_size = 0;
    queue_head = NULL;
    queue_tail = NULL;
    queue_closed = 0;
    shutdown = 0;
    threads_awake = 0;

    /* create the mutexs and cond vars */
    if((rtn = pthread_mutex_init(&(queue_lock),NULL)) != 0) {
        logger(FATAL,"pthread_mutex_init %s",strerror(rtn));
    }
    if((rtn = pthread_cond_init(&(queue_not_empty),NULL)) != 0) {
        logger(FATAL,"pthread_cond_init %s",strerror(rtn));
    }
    if((rtn = pthread_cond_init(&(queue_not_full),NULL)) != 0) {
        logger(FATAL,"pthread_cond_init %s",strerror(rtn));
    }
    if((rtn = pthread_cond_init(&(queue_empty),NULL)) != 0) {
        logger(FATAL,"pthread_cond_init %s",strerror(rtn));
    }
    if((rtn = pthread_cond_init(&(all_work_done),NULL)) != 0) {
        logger(FATAL,"pthread_cond_init %s",strerror(rtn));
    }

    /**
     * from "man 3c pthread_attr_init"
     * Define the scheduling contention scope for the created thread.  The only
     * value     supported    in    the    LinuxThreads    implementation    is
     * !PTHREAD_SCOPE_SYSTEM!, meaning that the threads contend  for  CPU  time
     * with all processes running on the machine.
     *
     * so no need to explicitly set the SCOPE
     */

    /* create the individual worker threads */
    for(i = 0; i < num_threads; i++)
    {
        if( (rtn=pthread_create(&(threads[i]),NULL, tpool_thread,(void*)this)) != 0)
        {
            logger(FATAL,"pthread_create %s\n",strerror(rtn)), exit(1);

        }
        threads_awake++;
    }
}

int tpool_t::tpool_add_work(void (*routine)(void*), void *arg)
{
    int rtn;
    tpool_work_t *workp;

    if((rtn = pthread_mutex_lock(&queue_lock)) != 0)
    {
        logger(FATAL,"pthread mutex lock failure\n");
        exit(1);
    }

    /* now we have exclusive access to the work queue ! */

    if((cur_queue_size == max_queue_size) && (do_not_block_when_full))
    {
        if((rtn = pthread_mutex_unlock(&queue_lock)) != 0)
        {
            logger(FATAL,"pthread mutex lock failure\n");
            exit(1);
        }
        return -1;
    }

    /* wait for the queue to have an open space for new work, while
     * waiting the queue_lock will be released */
    while((cur_queue_size == max_queue_size) && (!(shutdown || queue_closed)))
    {
        if((rtn = pthread_cond_wait(&(queue_not_full), &(queue_lock)) ) != 0)
        {
            logger(FATAL,"pthread cond wait failure\n");
            exit(1);
        }
    }

    if(shutdown || queue_closed)
    {
        if((rtn = pthread_mutex_unlock(&queue_lock)) != 0)
        {
            logger(FATAL,"pthread mutex lock failure\n");
            exit(1);
        }
        return -1;
    }

    /* allocate the work structure */
    if((workp = new tpool_work_t) == NULL)
    {
        logger(FATAL,"unable to create work struct\n");
        exit(1);
    }

    /* set the function/routine which will handle the work,
     * (note: it must be reenterant) */
    workp->handler_routine = routine;
    workp->arg = arg;
    workp->next = NULL;

    if(cur_queue_size == 0)
    {
        queue_tail = queue_head = workp;
        if((rtn = pthread_cond_broadcast(&(queue_not_empty))) != 0)
        {
            logger(FATAL,"pthread broadcast error\n");
            exit(1);
        }
    }
    else
    {
        queue_tail->next = workp;
        queue_tail = workp;
    }


    cur_queue_size++;

    /* relinquish control of the queue */
    if((rtn = pthread_mutex_unlock(&queue_lock)) != 0)
    {
        logger(FATAL,"pthread mutex lock failure\n");
        exit(1);
    }

    return 1;
}

tpool_t::~tpool_t()
{
    int i, rtn;
    tpool_work_t *cur;

    /* relinquish control of the queue */
    if((rtn = pthread_mutex_lock(&(queue_lock))) != 0)
    {
        logger(FATAL,"pthread mutex lock failure\n");
        exit(1);
    }

    /* is a shutdown already going on ? */
    if(queue_closed || shutdown)
    {
        if((rtn = pthread_mutex_unlock(&(queue_lock))) != 0)
        {
            logger(FATAL,"pthread mutex lock failure\n");
            exit(1);
        }

        return;
    }

    /* close the queue to any new work */
    queue_closed = 1;

    /* if the finish flag is set, drain the queue */
    if(POOL_THREADS_FINISH)
    {
        while(cur_queue_size != 0)
        {
            /* wait for the queue to become empty,
             * while waiting queue lock will be released */
            if((rtn = pthread_cond_wait(&(queue_empty), &(queue_lock))) != 0)
            {
                logger(FATAL,"pthread_cond_wait %d\n",rtn),exit(1);
            }
        }
    }

    /* set the shutdown flag */
    shutdown = 1;

    if((rtn = pthread_mutex_unlock(&(queue_lock))) != 0)
    {
        logger(FATAL,"pthread mutex unlock failure\n"),exit(1);
    }

    /* wake up all workers to rechedk the shutdown flag */
    if((rtn = pthread_cond_broadcast(&(queue_not_empty))) != 0)
    {
        logger(FATAL,"pthread_cond_boradcast %d\n",rtn),exit(1);
    }

    if((rtn = pthread_cond_broadcast(&(queue_not_full))) != 0)
    {
        logger(FATAL,"pthread_cond_boradcast %d\n",rtn),exit(1);
    }

    /* wait for workers to exit */
    for(i = 0; i < num_threads; i++)
    {
        if((rtn = pthread_join(threads[i],NULL)) != 0)
        {
            logger(FATAL,"pthread_join error %d %d\n",rtn, i), exit(1);
        }
    }

    /* clean up memory */
    delete[] threads;

    while(queue_head != NULL)
    {
        cur = queue_head->next;
        queue_head = queue_head->next;
        delete cur;
    }
}

void *tpool_thread(void *tpool)
{
    tpool_work_t *my_work;
    tpool_t *pool = (tpool_t *)tpool;
    sigset_t mask;

    sigfillset(&mask); /* Mask all allowed signals */
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    for(;;) /* go forever */
    {

        pthread_mutex_lock(&(pool->queue_lock));


        /* sleep until there is work,
         * while asleep the queue_lock is relinquished */
        while((pool->cur_queue_size == 0) && (!pool->shutdown))
        {
            pool->threads_awake --;
            if(pool->threads_awake == 0 && (!pool->shutdown))
                pthread_cond_signal(&(pool->all_work_done));

            pthread_cond_wait(&(pool->queue_not_empty), &(pool->queue_lock));

            pool->threads_awake ++;
        }

        /* are we shutting down ? */
        if(pool->shutdown)
        {
            pool->threads_awake --;
            pthread_mutex_unlock(&(pool->queue_lock));
            pthread_exit(NULL);
        }

        /* process the work */
        my_work = pool->queue_head;
        pool->cur_queue_size--;

        if(pool->cur_queue_size == 0)
            pool->queue_head = pool->queue_tail = NULL;
        else
            pool->queue_head = my_work->next;


        /* broadcast that the queue is not full */
        if((!pool->do_not_block_when_full) &&
                (pool->cur_queue_size == (pool->max_queue_size - 1)))
        {
            pthread_cond_broadcast(&(pool->queue_not_full));
        }

        if(pool->cur_queue_size == 0)
        {
            pthread_cond_signal(&(pool->queue_empty));
        }

        pthread_mutex_unlock(&(pool->queue_lock));

        logger(INFO,"pool %p threads awake %d\n", pool, pool->threads_awake);
        /* perform the work */
        (*(my_work->handler_routine))(my_work->arg);
        logger(INFO,"pool %p threads done\n", pool);
        delete my_work;
    }
    return(NULL);
}

mysqlpp::Connection* DBPool::create()
{
    // logger(INFO,"Create DB conn %d\n",size() + 1);
    std::cout << m_database << std::endl;
    return new mysqlpp::Connection(/*ctxt.conf->database.c_str()*/m_database.c_str(), ctxt.conf->hostname.c_str(), ctxt.conf->username.c_str(), ctxt.conf->password.c_str());
}

void DBPool::destroy(mysqlpp::Connection* con)
{
    //logger(INFO,"Destroy DB conn %d\n",size());
    delete con;
}
