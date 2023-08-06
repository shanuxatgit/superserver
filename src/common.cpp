/* -------------------------------------------------------------------------
 * common.cpp - common data
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

#include "common.h"
#include "conf.h"
#include <pcrecpp.h>
#include <time.h>
#define VARNAME(N) #N
#define PRINT(N) N,#N
#define PRT(N) printf("%s = %d\n", #N, N);
#define PRTSTR(N) printf("%s = %s\n", #N, N.c_str());

static void cp2utf1(char *out, const char *in);


int conf_t::read_conf()
{
    ConfigFile config( CONF_NAME );

    config.readInto(PRINT(using_pms), 1);

    config.readInto(PRINT(using_rc), 1);

    config.readInto(PRINT(using_cards), 0);

    config.readInto(PRINT(using_enddate), 0);

    config.readInto(PRINT(using_eraseonlast), 0);

    config.readInto(PRINT(using_autocheckin), 0);

    config.readInto(PRINT(link_aktive), 0);

    config.readInto(PRINT(using_parental), 0);

    config.readInto(PRINT(using_tvrights), 0);

    config.readInto(PRINT(default_playlist), 1);

    config.readInto(PRINT(using_micros), 0);

    config.readInto(PRINT(using_eltur), 0);
 
   config.readInto(PRINT(eltur_vodid), 0);

   config.readInto(PRINT(sendMsgWithXL), 0);

   config.readInto(PRINT(sendXRwithoutGN), 0);

    config.readInto(PRINT(nameEncoding),string("CP1251"));

    // for DB

    config.readInto(PRINT(username),string("root"));

    config.readInto(PRINT(password), string("password"));
    config.readInto(PRINT(database), string("VOD"));
    config.readInto(PRINT(hostname), string("127.0.0.1"));
    config.readInto(PRINT(db_port), 3305);
    // socket connections
    // rc_host;
    config.readInto(PRINT(rc_host), string("0.0.0.0"));
    config.readInto(PRINT(rc_port), 6543);

    config.readInto(PRINT(rc_broadcast), string("255.255.255.255"));

    config.readInto(PRINT(mng_host), string("0.0.0.0"));
    config.readInto(PRINT(mng_port), 6544);

    config.readInto(PRINT(php_host), string("0.0.0.0"));
    config.readInto(PRINT(php_port), 6546);

    config.readInto(PRINT(his_host), string("0.0.0.0"));
    config.readInto(PRINT(his_port), 6547);

    config.readInto(PRINT(client_host), string("0.0.0.0"));
    config.readInto(PRINT(client_port), 6548);

    config.readInto(PRINT(pms_host), string("0.0.0.0"));
    config.readInto(PRINT(pms_port), 40001);

    config.readInto(PRINT(inet_host), string("0.0.0.0"));
    config.readInto(PRINT(radio_host), string("0.0.0.0"));
    config.readInto(PRINT(radio_port), 6601);
    // for alarm server ?
    config.readInto(PRINT(s_hostname), string("0.0.0.0"));
    config.readInto(PRINT(s_port), 1234);

    config.readInto(PRINT(st_port), 7002);
    config.readInto(PRINT(st_serv_ip), string("0.0.0.0"));
    config.readInto(PRINT(st_serv_N), 0);

    config.readInto(PRINT(path), string(""));
    config.readInto(PRINT(log_file), string("bms_con.log"));

    config.readInto(PRINT(auto_inet), 1);
    config.readInto(PRINT(second_interface), 0);
    config.readInto(PRINT(pms_host_2), string("0.0.0.0"));
    config.readInto(PRINT(pms_port_2), 40002);

    string _t;
    config.readInto(_t, VARNAME(client_TypeId), string("1"));

     pcrecpp::RE re("(\\d+)(\\|)?");
     pcrecpp::StringPiece input(_t);
     int _n;

     while (re.Consume(&input, &_n)) {
         client_TypeId.push_back(_n);
     }

     config.readInto(_t, VARNAME(guest_TypeId), string("1"));
     input.clear();
     input.set(_t.c_str());

     while (re.Consume(&input, &_n)) {
         guest_TypeId.push_back(_n);
    }
//  client_TypeId;
//  guest_TypeId;

     return 0;
}

#if 0
room_status::room_status(int rm) : room(rm), status(0), oldstatus(2), tryprog(0), prog(false)
{
    time_t _t = time(NULL);
    lastchange = lastping = _t;

    int rtn;

    if((rtn = pthread_mutex_init(&(lock),NULL)) != 0) {
        logger(FATAL,"pthread_mutex_init %s",strerror(rtn));
    }

     if((rtn = pthread_cond_init(&(queue_not_empty),NULL)) != 0) {
        logger(FATAL,"pthread_cond_init %s",strerror(rtn));
     }
}
#endif

room_status::room_status(int rm, const DBPool& db) : room(rm), status(0), oldstatus(2), tryprog(0), prog(false), db_name(db)
{
    time_t _t = time(NULL);
    lastchange = lastping = _t;

    int rtn;

    if((rtn = pthread_mutex_init(&(lock),NULL)) != 0) {
        logger(FATAL,"pthread_mutex_init %s",strerror(rtn));
    }

     if((rtn = pthread_cond_init(&(queue_not_empty),NULL)) != 0) {
        logger(FATAL,"pthread_cond_init %s",strerror(rtn));
     }
}

void server_accept(void* msg)
{
    msg_t* _msg = static_cast<msg_t*> (msg);
    time_t lastcheck;

    int cfd = _msg->sd;
    string ip = _msg->ip;
    void (*message_loop)(void*) = _msg->message_loop;
    void (context_t::*save_handle)(std::string&, int) = _msg->save_handle;
    void (context_t::*del_handle)(std::string&) = _msg->del_handle;

    if(save_handle)
    {
        (ctxt.*save_handle)(ip, cfd);
    }

    if(_msg)
    {
        delete _msg;
        _msg = NULL;
    }

    lastcheck = time(NULL);

    do{
        pollfd pfd;
        pfd.fd = cfd;
        pfd.events = POLLIN;

        time_t now = time(NULL);
        if(difftime(now,lastcheck) > 3600)
            break;

        int ready = poll(&pfd, 1, 2000);

        if(ready != -1)
        {
            if((pfd.revents & POLLHUP) == POLLHUP)
            {
                logger(INFO, "Connection from %s closed\n", ip.c_str());
                break;
            }

            if((pfd.revents & POLLIN) == POLLIN)
            {
                char buffer[TCP_MSG_LENGTH];

                ssize_t received = recv(cfd, buffer, TCP_MSG_LENGTH, 0);

                if(received > 0)
                {
                    lastcheck = time(NULL);
                    buffer[received] = '\0';
                    msg_t* new_msg = new msg_t(buffer);
                    new_msg->ip = ip;
                    new_msg->sd = cfd;

                    logger(RECV, "msg %s from ip: %s length: %d\n", buffer, ip.c_str(), received);
                    ctxt.pool_tcp->tpool_add_work(message_loop, new_msg);
                }
                else
                {
                    logger(INFO, "Connection from %s closed\n", ip.c_str());
                    break;
                }
            }
        }
        else
        {
            break;
        }

    } while (ctxt.shutdown != 1);

    if(del_handle)
    {
        (ctxt.*del_handle)(ip);
    }

    if(_msg)
    {
        delete _msg;
        _msg = NULL;
    }

    shutdown(cfd, SHUT_RDWR);
    close(cfd);
}

void* server( int& sock, int port, std::string& ip,
                     void (*message_loop)(void*),
                     void (*accept_loop)(void*),
                     void (context_t::*save_handle)(std::string&, int),
                     void (context_t::*del_handle)(std::string&))
{
    sigset_t mask;

    sigfillset(&mask); /* Mask all allowed signals */
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock == -1){
        logger(ERROR, "Can't create socket for server: %s\n", strerror(errno));
        ctxt.shutdown = 1;

        return NULL;
    }

    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));

    struct sockaddr_in isa;

    isa.sin_family = AF_INET;
    isa.sin_port = htons(port);
    inet_aton(ip.c_str(), &isa.sin_addr);

    try{
        if(bind(sock, (struct sockaddr*)&isa, sizeof isa) == -1){
            throw errno;
        }

        if(listen(sock, 1) == -1){
            throw errno;
        }

        logger(INFO, "Successfully started server %s:%d\n", ip.c_str(), port);

        do{
            pollfd pfd;
            pfd.fd = sock;
            pfd.events = POLLIN;

            int ready = poll(&pfd, 1, 2000);

            if(ready != -1)
            {
                if(pfd.revents == POLLIN){
                    sockaddr_in client;
                    socklen_t clientlen = sizeof(client);

                    int cfd = accept(sock, (sockaddr *)&client, &clientlen);

                    msg_t * msg = new msg_t("\0");
                    msg->ip = inet_ntoa(client.sin_addr);
                    msg->sd = cfd;
                    msg->message_loop = message_loop;
                    msg->save_handle = save_handle;
                    msg->del_handle = del_handle;

                    logger(INFO, "Accept connection from %s\n", msg->ip.c_str(), port);
                    ctxt.pool_tcp->tpool_add_work(accept_loop, msg);
                }

                continue;
            }
            else
            {
                throw errno;
            }

        } while (ctxt.shutdown != 1);
    } catch(int& n)
    {
        logger(FATAL, "Error in server on IP address %s:%d: %s\n", ip.c_str(), port, strerror(n));
        ctxt.shutdown = 1;
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);
    logger(INFO, "Closing server\n");

    return NULL;
}

static void cp2utf1(char *out, const char *in) {
    static const int table[128] = {                    
        0x82D0,0x83D0,0x9A80E2,0x93D1,0x9E80E2,0xA680E2,0xA080E2,0xA180E2,
        0xAC82E2,0xB080E2,0x89D0,0xB980E2,0x8AD0,0x8CD0,0x8BD0,0x8FD0,    
        0x92D1,0x9880E2,0x9980E2,0x9C80E2,0x9D80E2,0xA280E2,0x9380E2,0x9480E2,
        0,0xA284E2,0x99D1,0xBA80E2,0x9AD1,0x9CD1,0x9BD1,0x9FD1,               
        0xA0C2,0x8ED0,0x9ED1,0x88D0,0xA4C2,0x90D2,0xA6C2,0xA7C2,              
        0x81D0,0xA9C2,0x84D0,0xABC2,0xACC2,0xADC2,0xAEC2,0x87D0,              
        0xB0C2,0xB1C2,0x86D0,0x96D1,0x91D2,0xB5C2,0xB6C2,0xB7C2,              
        0x91D1,0x9684E2,0x94D1,0xBBC2,0x98D1,0x85D0,0x95D1,0x97D1,            
        0x90D0,0x91D0,0x92D0,0x93D0,0x94D0,0x95D0,0x96D0,0x97D0,
        0x98D0,0x99D0,0x9AD0,0x9BD0,0x9CD0,0x9DD0,0x9ED0,0x9FD0,
        0xA0D0,0xA1D0,0xA2D0,0xA3D0,0xA4D0,0xA5D0,0xA6D0,0xA7D0,
        0xA8D0,0xA9D0,0xAAD0,0xABD0,0xACD0,0xADD0,0xAED0,0xAFD0,
        0xB0D0,0xB1D0,0xB2D0,0xB3D0,0xB4D0,0xB5D0,0xB6D0,0xB7D0,
        0xB8D0,0xB9D0,0xBAD0,0xBBD0,0xBCD0,0xBDD0,0xBED0,0xBFD0,
        0x80D1,0x81D1,0x82D1,0x83D1,0x84D1,0x85D1,0x86D1,0x87D1,
        0x88D1,0x89D1,0x8AD1,0x8BD1,0x8CD1,0x8DD1,0x8ED1,0x8FD1
    };
    while (*in)
        if (*in & 0x80) {
            int v = table[(int)(0x7f & *in++)];
            if (!v)
                continue;
            *out++ = (char)v;
            *out++ = (char)(v >> 8);
            if (v >>= 16)
                *out++ = (char)v;
        }
        else
            *out++ = *in++;
    *out = 0;
}
string cp2utf(string s) {
    unsigned int c,i;
    string ns;
    for(i=0; i<s.size(); i++) {
	c=s[i];
        char buf[4], in[2] = {0, 0};
        *in = c;
        cp2utf1(buf, in);
        ns+=string(buf);
    }
   return ns;
}
