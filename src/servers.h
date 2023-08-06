/* -------------------------------------------------------------------------
 * servers.h - servers function declarations
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

#ifndef SERVERS_H_
#define SERVERS_H_

void* rc_server(void* context);
void* mng_server(void* context);
void* php_server(void* context);
void* pms_server(void* context);
void* pms_server_2(void* context);
void* his_server(void* context);
void* client_server(void* context);
void* test_server(void* context);

#endif /*SERVERS_H_*/
