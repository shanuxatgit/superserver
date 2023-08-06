/* -------------------------------------------------------------------------
 * common.h - common data
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

#ifndef __COMMON_H
#define __COMMON_H

#include <string>
#include <vector>
#include <errno.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
//#define MYSQLPP_MYSQL_HEADERS_BURIED
#include <mysql++/mysql++.h>
#include <map>
#include <pcre/pcrecpp.h>
#include <sstream>
#include <poll.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include "linked_ptr.h"
#include "log.h"
#include "tpool.h"
using std::string;
using std::vector;
using std::map;
using std::pair;

#define CONF_NAME "../configs/base_server.cfg"
#define TCP_MSG_LENGTH  32766
#define logger ctxt.log->lprintf

#define dbg(N) std::cout<<#N<<"="<<N<<std::endl;

typedef unsigned short uint16;

//#define PRQ(N) std::cout<<N.str()<<std::endl;

//static void cp2utf1(char *out, const char *in);
string cp2utf(string s); 


typedef struct config{
    config() {}
    ~config(){}
    int using_pms;
    int using_rc;
    int using_cards;
    int using_enddate;
    int using_eraseonlast;
    int using_autocheckin;
    int link_aktive;
    int using_parental;
    int using_tvrights;
    int default_playlist;
    int using_micros;
    int using_eltur;
    int eltur_vodid;
    int sendMsgWithXL;
    int	sendXRwithoutGN;
    string nameEncoding;

// for DB
    string username;
    string password;
    string database;
    string hostname;
    int db_port;
// socket connections
// rc_host;
    string rc_host;
    int rc_port;

    string rc_broadcast;

    string mng_host;
    int mng_port;

    string php_host;
    int php_port;

    string his_host;
    int his_port;

    string client_host;
    int client_port;

    string pms_host;
    int pms_port;

    string inet_host;
    string radio_host;
    int radio_port;
// for alarm server ?
    string s_hostname;
    int s_port;
    int st_port;
    string st_serv_ip;
    int st_serv_N;
    string path;
// paths

    //pl_path = /var/www/htdocs/page/new/music/rooms/
// log info
    string log_file;
// /var/log/mylog.log
// custom settings
    int auto_inet;
    int second_interface;
    string pms_host_2;
    int pms_port_2;
    vector<int> client_TypeId;
    vector<int> guest_TypeId;

    int read_conf();
}conf_t;

typedef struct room_status{
#if 0
    room_status(int rm);
#endif
    room_status(int rm, const DBPool& db);
    ~room_status() {};

    int room;

    char status;
    char oldstatus;

    time_t lastchange;
    time_t lastping;

    char tryprog;
    bool prog;

    const DBPool& db_name;

    pthread_mutex_t lock;
    pthread_cond_t queue_not_empty;

    std::map<string, bool> key_map;
    std::map<string, bool> key_map_send;
}room_status_t;

typedef struct tag_synh_room_data {
    tag_synh_room_data(std::string& Gname, int Glang) : GuestName(Gname), GuestLang(Glang) {}
    std::string GuestName;
    int GuestLang;
}synh_room_data_t;

typedef struct tag_php_status{
    tag_php_status(int nhandle, int ntype, int itype) : handle(nhandle), type(ntype), inettype(itype)
    {
        datetime = time(NULL);
    }

    int handle;
    time_t datetime;
    int type;
    int inettype;
}php_status_t;

typedef struct context{
    context() : conf(NULL), log(NULL), pool(NULL), pool_tcp(NULL), rcthread(0), mngthread(0), phpthread(0), pmsthread(0), histhread(0),
                clientthread(0), shutdown(0), pmsanswered(0), SF_RoomNo(0), db_num(1)
    {
        int rtn;
        if((rtn = pthread_mutex_init(&(clients_lock),NULL)) != 0) {
            shutdown = 1;
        }

        if((rtn = pthread_mutex_init(&(php_lock),NULL)) != 0) {
            shutdown = 1;
        }

        if((rtn = pthread_mutex_init(&(his_lock),NULL)) != 0) {
            shutdown = 1;
        }

        if((rtn = pthread_mutex_init(&(mng_lock),NULL)) != 0) {
            shutdown = 1;
        }

        if((rtn = pthread_mutex_init(&(synh_lock),NULL)) != 0) {
            shutdown = 1;
        }

        if((rtn = pthread_mutex_init(&(pms_lock),NULL)) != 0) {
            shutdown = 1;
        }
        if((rtn = pthread_mutex_init(&(pms_lock2),NULL)) != 0) {
            shutdown = 1;
        }
    }

    conf_t*     conf;
    log_t*      log;
    tpool_t*    pool;
    tpool_t*    pool_tcp;

    int         rcsock; //room controler socket
    int         mngsock;
    int         phpsock;
    int         pmssock;
    int         pmssock2;
    int         hissock;
    int         clientsock;
    int         testsocket;

    pthread_t   rcthread;
    pthread_t   mngthread;
    pthread_t   phpthread;
    pthread_t   pmsthread;
    pthread_t   pmsthread2;
    pthread_t   histhread;
    pthread_t   clientthread;
    pthread_t   testthread;

    int shutdown;
    int pmsanswered;
    int pmsanswered2;
    int SF_RoomNo;

    std::string pmsbuffer;

    std::map<std::string, linked_ptr<room_status_t> > rooms;
    std::map<int, std::string> rooms_ip;
    std::map<std::string, int> rooms_id;
    std::map<int, std::string> rooms_name;
    std::map<std::string, int> ks;

    pthread_mutex_t clients_lock;
    std::map<std::string, int> clients;

    pthread_mutex_t his_lock;
    std::map<std::string, int> his;

    pthread_mutex_t mng_lock;
    std::map<std::string, int> mng;

    pthread_mutex_t php_lock;
    std::map<int, linked_ptr<php_status_t> > php;

    pthread_mutex_t synh_lock;
    std::map<int, linked_ptr<synh_room_data_t> > synh_data;

    pthread_mutex_t pms_lock;
    pthread_mutex_t pms_lock2;

    DBPool db_pool[10];
    std::map<std::string, const DBPool& > db_rel;
    size_t db_num;
    void add_to_pmsbuffer(std::ostringstream& msg)
    {

        pmsbuffer += msg.str();

        int len = pmsbuffer.length();

        if(len > 8192)
        {
            int pos = pmsbuffer.find("\r\n", 2048);

            pmsbuffer = pmsbuffer.substr(pos + 2, len);
        }
    }

    void add_his(std::string& ip, int cfd)
    {
        pthread_mutex_lock(&his_lock);
        his.insert(std::make_pair(ip, cfd));
        pthread_mutex_unlock(&his_lock);
    }

    void del_his(std::string& ip)
    {
        pthread_mutex_lock(&his_lock);
        std::map<string, int>::iterator i = his.find(ip);

        if(i != his.end())
            his.erase(i);

        pthread_mutex_unlock(&his_lock);
    }

    void add_mng(std::string& ip, int cfd)
    {
        pthread_mutex_lock(&mng_lock);
        mng.insert(std::make_pair(ip, cfd));
        pthread_mutex_unlock(&mng_lock);
    }

    void del_mng(std::string& ip)
    {
        pthread_mutex_lock(&mng_lock);
        std::map<string, int>::iterator i = mng.find(ip);

        if(i != mng.end())
            mng.erase(i);

        pthread_mutex_unlock(&mng_lock);
    }

    void add_client(std::string& ip, int cfd)
    {
        pthread_mutex_lock(&clients_lock);
        clients.insert(std::make_pair(ip, cfd));
        pthread_mutex_unlock(&clients_lock);
    }

    void del_client(std::string& ip)
    {
        pthread_mutex_lock(&clients_lock);
        std::map<string, int>::iterator i = clients.find(ip);

        if(i != clients.end())
            clients.erase(i);

        pthread_mutex_unlock(&clients_lock);
    }

    void insert_php(int handle, int RoomNo, int Type, int Inet)
    {
        pthread_mutex_lock(&php_lock);
        php.insert(std::make_pair(RoomNo, new php_status_t(handle, Type, Inet)));
        pthread_mutex_unlock(&php_lock);
    }

    /**
     *  function logic is in reverce
     *  return false if room is in and true if there is timeout or no info for room
     */
    bool is_in_php(int RoomNo)
    {
        bool bRet = false;

        pthread_mutex_lock(&php_lock);

        map<int, linked_ptr<php_status_t> >::iterator i = php.find(RoomNo);

        if(i != php.end())
        {
            time_t now = time(NULL);

            if(difftime(now, php[RoomNo]->datetime) > 60)
            {
                php.erase(i);

                bRet = true;
            }
        } else
            bRet = true;

        pthread_mutex_unlock(&php_lock);

        return bRet;
    }

    void delete_php(int RoomNo)
    {
        pthread_mutex_lock(&php_lock);
        map<int, linked_ptr<php_status_t> >::iterator i = php.find(RoomNo);

        if(i != php.end())
        {
            php.erase(i);
        }

        pthread_mutex_unlock(&php_lock);
    }
}context_t;

typedef struct typemsg{
    typemsg(const char* buffer) : command(buffer), save_handle(NULL), del_handle(NULL) {}
    string command;
    string ip;
    int sd;
    void (*message_loop)(void*);
    void (context_t::*save_handle)(std::string&, int);
    void (context_t::*del_handle)(std::string&);
}msg_t;

extern context_t ctxt;

#define TRANSACTION_BEGIN(C) \
        bool bOK = true;\
        \
        try{ \
            mysqlpp::Transaction trans(C);


#define TRANSACTION_END() \
            trans.commit(); \
        } \
        catch (const mysqlpp::BadQuery& er) { \
            logger(ERROR, "Query error: %s", er.what()); \
            bOK = false; \
        } \
        catch (const mysqlpp::BadConversion& er) { \
            logger(ERROR, "Conversion error: %s" , er.what()); \
            bOK = false; \
        } \
        catch (const mysqlpp::Exception& er) { \
            logger(ERROR, "Error: %s\n", er.what()); \
            bOK = false; \
        } \

#define DECODE_BEGIN(C) \
    std::vector<std::string> message; \
    pcrecpp::StringPiece input(C); \
    std::string command; \
     \
    while(pcrecpp::RE("([\\x20-\\x7B\\x7D-\\xFF]+)[\\|]*", pcrecpp::RE_Options().set_dotall(true)).Consume(&input, &command)) \
    { \
        message.push_back(command); \
        \
    } \
    \
    for(uint16 i = 0 ; i < message.size(); i++)\
    {\
        pcrecpp::StringPiece message_real(message[i]); \
	//std::cout << message[i] << std::endl;\

#define EXTRACT(PAT, C) \
    if(pcrecpp::RE(PAT).FullMatch(message_real, &C)) \
    { \
    } \

#define DECODE_END() }

#define BEGIN_DB(POOL) \
    { \
    mysqlpp::Connection* conn = NULL; \
    try{ \
        conn = (POOL).grab(); \
    if (conn && conn->connected()) \
    { \
        mysqlpp::Query qr = conn->query(); \

#define END_DB_RETURN(POOL) \
    (POOL).release(conn); \

#define END_DB(POOL) \
        (POOL).release(conn); \
    } \
    else \
        std::cout << "problem connecting" << std::endl; \
    }catch(mysqlpp::Exception err) \
    { \
        std::cout << "exception" << std::endl; \
        if (conn && conn->connected()) \
        { \
            (POOL).release(conn); \
        } \
        logger(ERROR, "Cannot open new connection to DB %s", err.what()); \
    } \
    } \

#define DB_EXECUTE(N) qr.execute(N)

#define DB_STORE(N) qr.store(N)


#define CRC(message) \
    std::string _t = message.str(); \
    message << std::setw(4) << std::hex << std::uppercase <<get_crc(_t) << "\r\n"; \

inline unsigned short get_crc(string& cmd)
{
    unsigned short crc16 = 0;
    int len = cmd.length();
    const char* c = cmd.c_str();

    for(int i = 0; i < len; i++)
    {
        crc16 = crc16 ^ static_cast<unsigned short>(c[i] << 8);

        for(int bit = 0; bit < 8; bit++)
        {
            if(crc16 & 0x8000)
            {
                crc16 = (crc16 << 1) ^ 0x1021;
            }
            else
            {
                crc16 = (crc16 << 1);
            }
        }
    }

    return crc16;
}

inline std::ostream& get_date(std::ostream& str)
{
    time_t now = time(NULL);
    tm local;

    localtime_r(&now, &local);
    str.fill('0');
    return str << std::setw(2) << local.tm_year - 100 << std::setw(2) << local.tm_mon+1 << std::setw(2) << local.tm_mday;
}

inline std::ostream& get_time(std::ostream& str)
{
    time_t now = time(NULL);
    tm local;

    localtime_r(&now, &local);

    str.fill('0');
    return str << std::setw(2) << local.tm_hour << std::setw(2) << local.tm_min << std::setw(2) << local.tm_sec;
}

inline std::ostream& get_date_time(std::ostream& str)
{
//  std::ostringstream str1;
    time_t now = time(NULL);
    tm local;

    localtime_r(&now, &local);
    str.fill('0');
    return str << 1900 + local.tm_year << "-"<< std::setw(2) << local.tm_mon + 1 << "-" << std::setw(2) << local.tm_mday << " " << std::setw(2) << local.tm_hour << ":" << std::setw(2) << local.tm_min << ":" << std::setw(2) << local.tm_sec;

    //str << str1.str();
}

inline void write_event(int type, std::ostringstream& reason)
{
    BEGIN_DB(ctxt.db_pool[0])
        std::ostringstream str;

        str << "INSERT INTO ServerEventLog (TypeID, EventText, Date) VALUES("<<type<<", '"<<reason.str()<<"', '"<< get_date_time <<"')";
        DB_EXECUTE(str.str().c_str());
    END_DB(ctxt.db_pool[0])
}

inline string bool_to_str(int n)
{
    return n == 0 ? string("FALSE") : string("TRUE");
}

inline int str_to_int(const char* str)
{
    if(strcmp(str, "TRUE"))
        return 0;
    else
        return 1;
}

inline void send_to_rc(std::string& ip, std::ostringstream& udp_send)
{
    struct sockaddr_in isa;

    logger(SEND, "rc %s: %s room %d\n", ip.c_str(), udp_send.str().c_str(), ctxt.rooms[ip]->room);
    CRC(udp_send);

    pollfd pfd;
    pfd.fd = ctxt.rcsock;
    pfd.events = POLLOUT;

    isa.sin_family = AF_INET;
    isa.sin_port = htons(ctxt.conf->rc_port);
    inet_aton(ip.c_str(), &isa.sin_addr);

    int ready = poll(&pfd, 1, 2000);

    if(ready != -1)
    {
        if(pfd.revents == POLLOUT){
            sendto(ctxt.rcsock, udp_send.str().c_str(), udp_send.str().length(), 0, (sockaddr *) &isa, sizeof(isa));
        }
    }
    else
    {
        printf("error sending data\n");
    }
}

inline bool is_KCWS(std::string& ip)
{
    bool bRet = true;
    std::map<string, int>::iterator i = ctxt.ks.find(ip);

    if(i == ctxt.ks.end())
        bRet = false;

    return bRet;
}

inline bool send_to_mng(std::ostringstream& msg, int id)
{
    std::map<std::string, int>::iterator i = ctxt.ks.begin();

    for(; i != ctxt.ks.end(); i++)
    {
        std::cout<<i->second<<std::endl;
        if(i->second == id)
        {
            break;
        }
    }

    if(i == ctxt.ks.end())
        return false;

    std::cout<<i->first<<std::endl;

    std::map<std::string, int>::iterator mng = ctxt.mng.find(i->first);

    bool bRet = false;
    if(mng != ctxt.mng.end())
    {
        logger(SEND, "mng: %s\n", msg.str().c_str());
        msg << "\r\n";

        pollfd pfd;
        pfd.fd = mng->second;
        pfd.events = POLLHUP;

        int ready = poll(&pfd, 1, 2000);

        if(ready != -1)
        {
            if((pfd.revents & POLLHUP) == POLLHUP){
                bRet = false;
                std::cout<<"fail\n";
            } else {
                bRet = true;
                std::cout<<"success\n";
                send(mng->second, msg.str().c_str(), msg.str().length(), 0);
            }
        }
    }

    std::cout<<bRet<<" bRet"<<std::endl;

    return bRet;
}

inline int send_to_client(std::ostringstream& msg, int cfd )
{
    logger(SEND, "client: %s\n", msg.str().c_str());
    std::ostringstream final;
    final << "\2" << msg.str() << "\3\r\n";

    pollfd pfd;
    pfd.fd = cfd;
    pfd.events = POLLOUT;

    if(cfd)
    {
        int ready = poll(&pfd, 1, 2000);

        if(ready != -1)
        {
            if((pfd.revents & POLLHUP) == POLLHUP){
                logger(ERROR, "client connection closed");
                return 0;
//              break;
//          }
//          else if((pfd.revents & POLLOUT) == POLLOUT){
//              continue;
            } else {
                int n = send(cfd, final.str().c_str(), final.str().length(), 0);
                if(n == 0)
                    return n;
            }
        }
        else{
            logger(ERROR, "Error sending to client: %s", errno);
            return 0;
//          break;
        }
    }
    return 1;
}

inline void send_to_pms(std::ostringstream& msg, int pmssock = -1)
{
    if(pmssock == -1)
        pmssock = ctxt.pmssock;

//    logger(SEND, "pms: %d %s\n", pmssock, msg.str().c_str());
    std::ostringstream final;
    //todor
    if(ctxt.conf->using_eltur){
	final << msg.str() << "\r\n";
    } else {
	final << "\2" << msg.str() << "\3";//\r\n";
    }
    logger(SEND, "pms: %s\n", final.str().c_str());

    pollfd pfd;
    pfd.fd = pmssock;
    pfd.events = POLLOUT;

//  while(1)
    {
        int ready = poll(&pfd, 1, 2000);

        if(ready != -1)
        {
            if((pfd.revents & POLLHUP) == POLLHUP){
                logger(ERROR, "PMS connection closed");
//              break;
//          }
//          else if((pfd.revents & POLLOUT) == POLLOUT){
//              continue;
            } else {

                send(pmssock, final.str().c_str(), final.str().length(), 0);

            }
        }
        else{
            logger(ERROR, "Error sending to PMS: %s", errno);
//          break;
        }
    }
}

inline void send_to_pms(const std::string& msg, int pmssock = -1)
{
    if(pmssock == -1)
        pmssock = ctxt.pmssock;

    logger(SEND, "pms: %d %s\n", pmssock, msg.c_str());
    std::ostringstream final;
    if(ctxt.conf->using_eltur){
	final << msg << "\r\n";
    } else {
	final << "\2" << msg << "\3";//\r\n";
    }


//    final << "\2" << msg << "\3";//\r\n";

    pollfd pfd;
    pfd.fd = pmssock;
    pfd.events = POLLOUT;

//  while(1)
    {
        int ready = poll(&pfd, 1, 2000);

        if(ready != -1)
        {
            if((pfd.revents & POLLHUP) == POLLHUP){
                logger(ERROR, "PMS connection closed");
//              break;
//          }
//          else if((pfd.revents & POLLOUT) == POLLOUT){
//              continue;
            } else {

                send(pmssock, final.str().c_str(), final.str().length(), 0);

            }
        }
        else{
            logger(ERROR, "Error sending to PMS: %s", errno);
//          break;
        }
    }
}

void check_out(std::string& req);

inline std::string& normalize_string(std::string& str)
{
    pcrecpp::RE("(\\')").GlobalReplace("\\'\\'", &str);

    return str;
}

inline std::string get_rc_ip(int RoomNo)
{
    // std::ostringstream rcs_ip;
    // rcs_ip << "SELECT IP_RC FROM Room WHERE RoomID=" << RoomNo;
    // mysqlpp::StoreQueryResult res1 = DB_STORE(rcs_ip.str().c_str());

    // return res1[0][0].c_str();
    return ctxt.rooms_ip[RoomNo];
}

inline int get_h_type(mysqlpp::Query& qr, int RoomNo)
{
    bool bOK = false;
    int HType = 0;

    if(RoomNo > 0)
    {
        std::string ip = get_rc_ip(RoomNo);

        if(ip.empty())
        {
            std::cout<< "empty ip starting directly\n";
            HType = 1;
            bOK = false;
        }else if(ctxt.conf->using_rc)
        {
            pthread_mutex_lock( &ctxt.rooms[ip]->lock);
            if(ctxt.rooms[ip]->status == 1)
            {
                bOK = true;
            }
            pthread_mutex_unlock( &ctxt.rooms[ip]->lock);
        }
        else
        {
            bOK = true;
        }
    }

    if(bOK)
    {
        std::ostringstream type_q;
        type_q << "SELECT Keycard.KHTypeID FROM ((RoomStatus LEFT  JOIN KeycardRoom ON (KeycardRoom.RoomID = RoomStatus.RoomID))INNER JOIN Keycard ON (Keycard.KeyID = KeycardRoom.KeyID) AND (Keycard.KeyID = RoomStatus.PowerKeyID)) WHERE RoomStatus.RoomID=" << RoomNo;
        mysqlpp::StoreQueryResult type_res = DB_STORE(type_q.str().c_str());

        if(type_res.num_rows() > 0)
        {
            HType = type_res[0][0];
        }
    }

    return HType;
}

inline std::string get_room_name(int RoomNo)
{
    // std::ostringstream room_name_q;
    // room_name_q << "SELECT RoomName FROM Room WHERE RoomID =" << RoomNo;
    // mysqlpp::StoreQueryResult room_name_res = DB_STORE(room_name_q.str().c_str());

    // std::string sRoomName;

    // if(room_name_res.num_rows() > 0)
    // {
    //     sRoomName = room_name_res[0][0].c_str();
    // }

    // return sRoomName;
    return ctxt.rooms_name[RoomNo];
}
/**
 * password is in admno
 */

inline std::string gen_inet(int Amount, int RoomNo, int Official, const std::string& Reason, int EmpID)
{
    char admno[8]={0,};

    BEGIN_DB(ctxt.db_pool[0])
        std::ostringstream inet_time_q;

        inet_time_q << "SELECT InetTime, Price FROM Internet WHERE InetType=" << Amount;
        mysqlpp::StoreQueryResult inet_time_res = DB_STORE(inet_time_q.str().c_str());

        int net_time = inet_time_res[0][0];
        int price = 0;

        if(Official == 0)
            price = (int) inet_time_res[0][1];

        srand(time(NULL));

        int i = rand();

        std::ostringstream s;
        s << i;

        char *c = crypt("MT", &s.str().c_str()[2]);

        memcpy(admno, &c[1], 7);
        admno[7] = '\0';

        TRANSACTION_BEGIN(*conn)
            std::ostringstream inet_pay_q;
            inet_pay_q << "INSERT INTO InternetPay(RoomID, InetType, PayDate, User, Pwd, IsCronStarted, InetTime, Price, IsActive) VALUES ("<<RoomNo<<", "<< Amount <<", '"<< get_date_time <<"', 'room" << RoomNo <<"', '"<<admno<<"', 'FALSE', "<<net_time<<", "<<price<<", 'TRUE')";
            DB_EXECUTE(inet_pay_q.str().c_str());

            if(Official == 1)
            {
                std::ostringstream official_log;

                official_log << "INSERT INTO OfficialOperationLog (ObjectID, OperationID, RoomID, Date, EmpID, Allowed, OperationDate, Reason ) VALUES("<< Amount << ",2,"<< RoomNo << ",'" << get_date_time << "'," << EmpID << ",'TRUE','"<< get_date_time <<"','"<< Reason << "')";
            }
        TRANSACTION_END()
    END_DB(ctxt.db_pool[0])
    logger(INFO, "gen_inet: pass %s RoomNo %d\n", admno, RoomNo);
    return admno;
}

inline void make_gen(int RoomNo, int Amount, int PostingType, int cfd, const std::string& SenderName, int Official, const std::string& Reason, int EmpID, int serviceID = 0)
{
    if(RoomNo == 0)
        return;

    BEGIN_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))

        std::string ip;

        int HType = 1;

        if(SenderName == "PHP")
            HType = get_h_type(qr, RoomNo);

        if(HType == 1 || ctxt.conf->using_cards == 0)
        {
            if(ctxt.conf->using_pms && Official == 0)
            {
                if(PostingType == 0)
                {
                    std::ostringstream guest_info_q;
                    guest_info_q << "SELECT GuestNo FROM RoomGuest WHERE RoomID=" << RoomNo;
                    mysqlpp::StoreQueryResult guest_info_res = DB_STORE(guest_info_q.str().c_str());

                    if(guest_info_res.num_rows() != 0)
                    {
                        if(ctxt.is_in_php(RoomNo))
                        {
                            ctxt.insert_php(cfd, RoomNo, PostingType, Amount);
                            std::string sRoomName = get_room_name(RoomNo);

                            std::ostringstream pms_message;
			    if ( ctxt.conf->using_eltur )
			    {
				    Amount = Amount / 100;
	                            pms_message << "AS|RN" << sRoomName<<"|SO"<< serviceID <<"|TA"<< Amount <<"|DA"<< get_date << "|TI"<< get_time <<"|";
			    }
			    else
			    { 
	                            pms_message << "PS|RN" << sRoomName<<"|PTC|TA"<< Amount <<"|DA"<< get_date << "|TI"<< get_time <<"|";
			    }
                            if(ctxt.conf->second_interface)
                                send_to_pms(pms_message, ctxt.pmssock2);
                            else
                                send_to_pms(pms_message);
                        }
                    }
                    else
                    {
                        //if(ctxt.is_in_php(RoomNo))
                        {
                            //ctxt.insert_php(cfd, RoomNo, PostingType, Amount);
                            std::ostringstream msg;
                            msg << "PA|RN"<<RoomNo<<"|ASNO|\r\n";
                            send(cfd, msg.str().c_str(), msg.str().length(), 0);
                        }
                    }
                }
                else
                {
                    std::ostringstream internet;
                    internet << "SELECT Price FROM Internet WHERE InetType=" << Amount;
                    mysqlpp::StoreQueryResult internet_res = DB_STORE(internet.str().c_str());

                    int price = 0;

                    if(internet_res.num_rows() > 0)
                    {
                        price = (int)internet_res[0][0];
                        price /= 100;
                    }

                    if(ctxt.is_in_php(RoomNo))
                    {
                        ctxt.insert_php(cfd, RoomNo, PostingType, Amount);
                        std::string sRoomName = get_room_name(RoomNo);

                        std::ostringstream pms_message;
                        pms_message << "PS|RN" << sRoomName<<"|PTC|TA"<< price <<"|DA"<< get_date << "|TI"<< get_time <<"|";

                        if(ctxt.conf->second_interface)
                            send_to_pms(pms_message, ctxt.pmssock2);
                        else
                            send_to_pms(pms_message);
                    }
                }
            }
            else
            {
                if(PostingType == 0)
                {
                    std::ostringstream message;
                    message << "PA|RN" << RoomNo << "|ASOK|\r\n";

                    send(cfd, message.str().c_str(), message.str().length(), 0);
                }
                else
                {
                    std::string admno = gen_inet(Amount, RoomNo, Official, Reason, EmpID);
                    /*std::ostringstream inet_time_q;

                    inet_time_q << "SELECT InetTime, Price FROM Internet WHERE InetType=" << Amount;
                    mysqlpp::StoreQueryResult inet_time_res = DB_STORE(inet_time_q.str().c_str());

                    int net_time = inet_time_res[0][0];
                    int price = 0;

                    if(Official == 0)
                        price = (int) inet_time_res[0][1];

                    srand(time(NULL));

                    int i = rand();

                    std::ostringstream s;
                    s << i;

                    char *c = crypt("MT", &s.str().c_str()[2]);

                    char admno[7];
                    memcpy(admno, &c[1], 7);

                    TRANSACTION_BEGIN(conn)
                        std::ostringstream inet_pay_q;
                        inet_pay_q << "INSERT INTO InternetPay(RoomID, InetType, PayDate, User, Pwd, IsCronStarted, InetTime, Price, IsActive) VALUES ("<<RoomNo<<", "<< Amount <<", '"<< get_date_time <<"', 'room" << RoomNo <<"', '"<<admno<<"', 'FALSE', "<<net_time<<", "<<price<<", 'TRUE')";
                        DB_EXECUTE(inet_pay_q.str().c_str());

                        if(Official == 1)
                        {
                            std::ostringstream official_log;

                            official_log << "INSERT INTO OfficialOperationLog (ObjectID, OperationID, RoomID, Date, EmpID, Allowed, OperationDate, Reason ) VALUES("<< Amount << ",2,"<< RoomNo << ",'" << get_date_time << "'," << EmpID << ",'TRUE','"<< get_date_time <<"','"<< Reason << "')";
                        }
                    TRANSACTION_END()*/

                    if(!admno.empty())
                    {
                        std::ostringstream message;

                        message << "PA|RN" << RoomNo << "|NMroom" << RoomNo <<"|PD"<<admno<<"|ASOK|\r\n";
                        send(cfd, message.str().c_str(), message.str().length(), 0);
                    }
                    else
                    {
                        std::ostringstream message;

                        message << "PA|RN" << RoomNo << "|NM|PD|ASNO|\r\n";
                        send(cfd, message.str().c_str(), message.str().length(), 0);
                    }

                }
            }
        }

    END_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
}

inline int get_season(mysqlpp::Query &qr)
{
    mysqlpp::StoreQueryResult res_seson = DB_STORE("SELECT Season FROM SystemInfo");
    return (int)res_seson[0][0];
}

inline void process_command(std::string& req)
{
    std::ostringstream command;
    logger(INFO, "Send command to external script newsystem.pl: %s", command.str().c_str()); 

    //command << "perl /usr/src/thread/srv_parse.pl \""<< req <<"\" &";
    command << "perl /usr/src/superserver/newsystem.pl \""<< req <<"\" &";
    system(command.str().c_str());
}

void server_accept(void* msg);

void* server( int& sock, int port, std::string& ip,
              void (*message_loop)(void*),
              void (*accept_loop)(void*) = server_accept,
              void (context_t::*save_handle)(std::string&, int) = NULL,
              void (context_t::*del_handle)(std::string&) = NULL );

#endif //__COMMON_H
