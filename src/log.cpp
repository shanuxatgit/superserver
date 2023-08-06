/* -------------------------------------------------------------------------
 * log.cpp - logging functions
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

#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "log.h"
#include "util.h"

/* Allows printf()-like interface to file descriptors without the
 * complications that arise from mixing stdio and low level calls
 * FIXME: Needs date and time before logfile entries.
 */
int log_t::lprintf( unsigned int level, const char *fmt, ... ) {
    int rc;
    va_list ap;
    time_t now;
    char date[50];
    static char line[LOGLINE_MAX];
    static char threadnum[10];
    int cnt;
    static const char *levels[8] = {   "[(bad)] ",
                                       "[debug] ",
                                       "[info ] ",
                                       "[warn ] ",
                                       "[error] ",
                                       "[fatal] ",
                                       "[send ] ",
                                       "[recv ] "};

//    if(!log) return -1;

    /* If this is debug info, and we're not logging it, return */
    if( !(flags & LOG_DEBUG) && level == DEBUG ) return 0;

    /* Prepare the date string */
    if( !(flags & LOG_NODATE) ) {
        now = time(NULL);
        strcpy(date, ctime(&now));
        date[strlen(date) - 6] = ' ';
        date[strlen(date) - 5] = '\0';
    }

    if( !(flags & LOG_NOTID) ) {
        sprintf(threadnum, "(%lu) ", pthread_self());
    }

    cnt = snprintf(line, sizeof(line), "%s%s%s", flags & LOG_NODATE ? "" : date,
                                                 flags & LOG_NOLVL  ? "" : (level > RECV ? levels[0] : levels[level]),
                                                 flags & LOG_NOTID  ? "" : threadnum);

    va_start(ap, fmt);
    vsnprintf(line+cnt, sizeof(line)-cnt, fmt, ap);
    va_end(ap);

    line[sizeof(line)-1] = '\0';

    if( !(flags & LOG_NOLF) ) {
        chomp(line);
        strcpy(line+strlen(line), "\n");
    }

    sem_wait(&sem);
    rc = write(fd, line, strlen(line));

    if( (flags & LOG_STDERR) == LOG_STDERR && fd != 2)
        write(2, line, strlen(line));

    sem_post(&sem);

    if( !rc ) errno = 0;
    return rc;
}

void log_t::rotate(int, const char* pszFileName)
{
    sem_wait(&sem);
    close(fd);
    if( !strcmp(pszFileName,"-") ) {
        fd = 2;
    } else {
        fd = open(pszFileName, O_WRONLY|O_CREAT|O_NOCTTY | (flags & LOG_TRUNC ? O_TRUNC : O_APPEND) , 0666);
    }

    try {
        if( fd == -1 ) {
            fprintf(stderr, "log_open: Opening logfile %s: %s", pszFileName, strerror(errno));
        }
        if( sem_init(&sem, 0, 1) == -1 ) {
            fprintf(stderr, "log_open: Could not initialize log semaphore.");
            throw 1;
        }
    }
    catch(...)
    {
        close(fd);
    }
    sem_post(&sem);
}

log_t::log_t(int nLogFlags, const char* pszFileName) : flags(nLogFlags){
   // log_t *log = (log_t*)malloc(sizeof(log_t));
 //   flags = LOG_FLAGS;
    if( !strcmp(pszFileName,"-") ) {
        fd = 2;
    } else {
        fd = open(pszFileName, O_WRONLY|O_CREAT|O_NOCTTY | (flags & LOG_TRUNC ? O_TRUNC : O_APPEND) , 0666);
    }

    try {
        if( fd == -1 ) {
            fprintf(stderr, "log_open: Opening logfile %s: %s", pszFileName, strerror(errno));
        }
        if( sem_init(&sem, 0, 1) == -1 ) {
            fprintf(stderr, "log_open: Could not initialize log semaphore.");
            throw 1;
        }
    }
    catch(...)
    {
        close(fd);
    }
}

log_t::~log_t() {
    sem_wait(&sem);
    sem_destroy(&sem);
    close(fd);
 //   free(log);
}

