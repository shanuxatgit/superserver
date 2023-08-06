/* -------------------------------------------------------------------------
 * util.h - miscellaneous utility functions
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

#ifndef __UTIL_H
#define __UTIL_H

#include <stdio.h>
#include <sys/types.h>
#include <stdarg.h>
#include <ctype.h>

#undef __EI
#define __EI extern __inline__

/*
 * Provides a strncasecmp for non-BSD C libraries
 */
int xstrncasecmp( const char *s1, const char *s2, int n );


__EI
int xstrcasecmp( const char *s1, const char *s2 ) {
    return xstrncasecmp( s1, s2, 0 );
}

/*
 * fprintf()-like function but uses a file descriptor or socket descriptor
 * instead of a FILE*.
 */
int fdprintf(int fd, char *fmt, ...);

/*
 * receives a line of data from fd, and puts up to len bytes into buf.
 * Returns NULL if there was no data, or buf on success.
 */
char *recvline( char *buf, int len, int fd );

/*
 * Ensures there is no extra data waiting to be received on fd.  If there is,
 * it is discarded, and the number of discarded bytes is returned.
 */
int recvflush( int fd );

/*
 * Reads exactly len bytes from fd and returns the data in a dynamically
 * allocated buffer
 */
char *readloop( int fd, size_t len );

/*
 * Takes in a char * and returns a dynamically allocated array of pointers to
 * the start of each line. The pointer after the last is set to NULL to
 * indicate that there are no more pointers after it. Every occurance of '\n'
 * in the input string is CHANGED to '\0' to ease the reading of the strings.
 */
//char **splitlines( char *buf );

/*
 * Removes any trailing whitespace from str by moving up the null pointer.
 */


__EI
char *chomp( char *str )
{
   int i=strlen(str)-1;
    while( i >= 0 ){
        if( isspace((int)str[i]) ) str[i--]='\0'; else break;
    }
    return str;
}

/*
 * Pass in a hostname, a pointer to a user supplied hostent struct, a
 * sufficiently-sized buffer for user by gethostbyname_r(), and the length of
 * the buffer, and resolve() fills in the hostent struct for you, returning a
 * pointer to it on success, or NULL on failure.
 */
struct hostent *
resolve( const char *name, struct hostent *hostbuf, char *buf, size_t len );

#endif
