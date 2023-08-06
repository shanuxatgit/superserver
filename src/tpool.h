/* -------------------------------------------------------------------------
 * tpool.h - thread pool defs
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

#ifndef _TPOOL_H_
#define _TPOOL_H_

#include  <stdio.h>
#include  <pthread.h>
#include <mysql++/mysql++.h>
/*
 * a generic thread pool creation routines
 */

#define POOL_WORKER_THREADS 250
#define POOL_QUEUE_SIZE     3000
#define POOL_BLOCK_ONFULL   0
#define POOL_THREADS_FINISH 1

typedef struct tpool_work{
  void (*handler_routine)(void*);
  void *arg;
  struct tpool_work *next;
} tpool_work_t;

class tpool_t {
    int num_threads;
    int max_queue_size;

    int do_not_block_when_full;
    pthread_t *threads;
    int cur_queue_size;
    tpool_work_t *queue_head;
    tpool_work_t *queue_tail;
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_not_full;
    pthread_cond_t queue_not_empty;
    pthread_cond_t queue_empty;
    pthread_cond_t all_work_done;
    int queue_closed;
    int shutdown;
    int threads_awake;

public:
    tpool_t(int, int);
    ~tpool_t();

    int tpool_add_work(void  (*routine)(void*), void *arg);

    friend void *tpool_thread(void *tpool);
};

class DBPool : public mysqlpp::ConnectionPool {
    string m_database;
public:
    DBPool() {

    }

    DBPool(string &database) {
        m_database = database;
    }

    ~DBPool(){
        clear();
    }

    void set_db(string &database) {
        m_database = database;
    }

    string get_db() {
        return m_database;
    }
protected:
    virtual mysqlpp::Connection* create();

    /// \brief Destroy a connection
    ///
    /// Subclasses must override this.
    ///
    /// This is for destroying the objects returned by create().
    /// Because we can't know what the derived class did to create the
    /// connection we can't reliably know how to destroy it.
    virtual void destroy(mysqlpp::Connection* con);

    /// \brief Returns the maximum number of seconds a connection is
    /// able to remain idle before it is dropped.
    ///
    /// Subclasses must override this as it encodes a policy issue,
    /// something that MySQL++ can't declare by fiat.
    ///
    /// \retval number of seconds before an idle connection is destroyed
    /// due to lack of use
    virtual unsigned int max_idle_time()
    {
        return 3600;
    }
};
#endif /* _TPOOL_H_ */
