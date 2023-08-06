/* -------------------------------------------------------------------------
 * util.cpp - miscellaneous utility functions
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
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#undef __EI
#include "util.h"
#include "log.h"
#include "common.h"
#ifdef DEBUG
#define dprintf logger
#else
#define dprintf(...)
#endif
//#include "common.h"

extern log_t *logn;

//char *chomp( char *str ){
    //int i=strlen(str)-1;
    //while( i >= 0 ){
        //if( isspace((int)str[i]) ) str[i--]='\0'; else break;
    //}
    //return str;
//}


int xstrncasecmp( const char *s1, const char *s2, int n ){
    const char *p1=s1, *p2=s2;
    int ctr=0;

    if( s1 == s2 ) return 0;

    while( *p1 && tolower(*p1) == tolower(*p2) ) {
        if( n ) {
            ctr++;
            if( ctr >= n ) return 0;
        }
        p1++; p2++;
    }
    if( !*p1 && *p2 ) return -1;
    else if( !*p2 && *p1 ) return 1;
    else return tolower(*p1)-tolower(*p2);
}

int fdprintf(int fd, char *fmt, ...) {
    FILE *fp = fdopen(dup(fd), "w");
    int rc;
    va_list ap;

    va_start(ap, fmt);
    rc=vfprintf(fp, fmt, ap);
    fflush(fp);
    fclose(fp);
    va_end(ap);
    return rc;
}

char *recvline( char *buf, int len, int fd ){
    char c=0;
    int ctr=0;

    while( ctr < len-2 ){
        if( c != '\n' && read(fd,&c,1) == 1 ){
            buf[ctr]=c;
            ctr++;
        } else {
            break;
        }
    }
    if( ctr == len-2 ){
        while( c != '\n' && read(fd,&c,1) == 1 ) ctr++;
        logger( WARN,
                "recvline: line exceeded buffer space by %d bytes.",
                ctr-len);
    }
    buf[ctr]='\0';

    if( *buf == '\0' ) return NULL;
    else return buf;
}

int recvflush( int s ) {
    fd_set fds;
    char c=0;
    int rc, cnt=0;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(s, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    while( (rc=select(s+1, &fds, NULL, NULL, &tv)) ) {
        if( rc == -1 ) {
            logger( WARN, "recvflush: select() failed: %s",
                    strerror(errno));
            break;
        }
        if( (rc=read(s, &c, 1)) < 1 ) {
            if( rc == 0 ) {
                logger( WARN, "recvflush: connection reset by peer");
            } else {
                logger( WARN, "recvflush: read() failed: %s",
                        strerror(errno));
            }
            break;
        }
        cnt++;
        tv.tv_sec = 0;
        tv.tv_usec = 0;
    }

    dprintf(DEBUG, "recvflush: returning %d.", cnt);
    return cnt;
}

/*
 * Reads exactly len bytes from fd and returns the data in a dynamically
 * allocated buffer
 */
char *readloop( int fd, size_t len ) {
    size_t cnt=0;
    int rc;
 //   char *buf = (char*) malloc(len+1);
    char* buf= new char[len+1];

    if(!buf) return NULL;

    dprintf( DEBUG, "readloop: attempting to read %d bytes from fd #%d",
            len, fd);
    while( cnt < len ) {
        if( (rc=read(fd,buf+cnt,len-cnt)) <= 0 ) {
            if( rc < 0 ) {
                if( errno == EINTR ) continue;
                logger( WARN, "Reading from sock fd %d: %s.", fd,
                        strerror(errno));
            } else {
                logger( WARN,
                        "Reading from sock fd %d: Read %d bytes, expected %d",
                        fd, cnt, len);
            }
//            free(buf);
            delete[] buf;
            return NULL;
        }
        dprintf( DEBUG, "readloop: read() returned %d.", rc);
        cnt += rc;
    }
    buf[len] = '\0';

    return buf;
}

//char **splitlines( char *buf ) {
    //int cnt = 1;
    //char **tmp, **ret = NULL;
    //char *cp;

    //if( !buf || !*buf ) return NULL;
////    if( (ret=(char**)malloc(1 + cnt * sizeof(char**))) == NULL ) return NULL;
    //if((ret = new char**[1+cnt*]) == NULL) return NULL;
    //ret[0] = buf;

    //for( cp=buf; *cp; cp++ ) {
        //if( *cp != '\n' ) continue;
        //if( (tmp=(char**)realloc(ret, (++cnt + 1) * sizeof(char**))) == NULL ) {
            //free(ret);
            //return NULL;
        //}
        //ret=tmp;

        //*cp = '\0';
        //ret[cnt-1] = cp + 1;
    //}

    //ret[cnt] = NULL;
    //return ret;
//}

/* Thread-safe resolve */
struct hostent *
resolve( const char *name, struct hostent *hostbuf, char *buf, size_t len )
{
    struct hostent *hp;
    int herr, rc=0, i;

    for( i=0; i<3; i++ ){
        rc=gethostbyname_r(name, hostbuf, buf, len, &hp, &herr);
        if( !rc ){
            return hp;
        } else if( herr == TRY_AGAIN ){
            continue;
        } else {
            break;
        }
    }
    errno=rc;
    return NULL;
}

