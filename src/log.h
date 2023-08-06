/* -------------------------------------------------------------------------
 * log.h - logging defs
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

#ifndef __LOG_H
#define __LOG_H

#include <stdio.h>
#include <semaphore.h>

#define LOGLINE_MAX 1024

#define DEBUG 1
#define INFO  2
#define WARN  3
#define ERROR 4
#define FATAL 5
#define SEND  6
#define RECV  7

#define LOG_TRUNC   1<<0
#define LOG_NODATE  1<<1
#define LOG_NOLF    1<<2
#define LOG_NOLVL   1<<3
#define LOG_DEBUG   1<<4
#define LOG_STDERR  1<<5
#define LOG_NOTID   1<<6

#define LOG_FILENAME "bms_con.log" //stderr
#define LOG_FLAGS	LOG_STDERR	//flags - The bitwise 'or' of zero or more of the following flags:
 						//          LOG_TRUNC   - Truncates the logfile on opening
 						//          LOG_NODATE  - Omits the date from each line of the log
 						//          LOG_NOLF    - Keeps from inserting a trailing '\n' when you don't.
 						//          LOG_NOLVL   - Keeps from inserting a log level indicator.
 						//          LOG_STDERR  - Sends log data to stderr as well as to the log.
 						//                        (this not implemented yet)

//typedef struct {
    //int fd;
    //sem_t sem;
    //int flags;
//} log_t;

class log_t {
	int fd;
	sem_t sem;
	int flags;

public:
	log_t(int nLogFlags, const char* pszFileName);
	~log_t();

	int lprintf( unsigned int level, const char *fmt, ... );
	void rotate(int nLogFlags, const char* pszFileName);
};


/*
 * Logs to the logfile using printf()-like format strings.
 *
 * log_t - The value you got back from a log_open() call.
 * level - can be one of: DEBUG, INFO, WARN, ERROR, FATAL
 * fmt   - is a printf()-like format string, followed by the parameters.
 *
 * Returns 0 on success, or -1 on failure.
 */
//int lprintf( log_t *log, unsigned int level, char *fmt, ... );


/*
 * Initializes the logfile to be written to with fprintf().
 *
 * fname - The name of the logfile to write to
 * flags - The bitwise 'or' of zero or more of the following flags:
 *          LOG_TRUNC   - Truncates the logfile on opening
 *          LOG_NODATE  - Omits the date from each line of the log
 *          LOG_NOLF    - Keeps from inserting a trailing '\n' when you don't.
 *          LOG_NOLVL   - Keeps from inserting a log level indicator.
 *          LOG_STDERR  - Sends log data to stderr as well as to the log.
 *                        (this not implemented yet)
 *
 * Returns NULL on failure, and a valid log_t (value > 0) on success.
 */
//log_t *log_open( char *fname, int flags );

/*
 * Closes a logfile when it's no longer needed
 *
 * log  - The log_t corresponding to the log you want to close
 */
//void log_close( log_t *log );

#endif
