/* -------------------------------------------------------------------------
 * rc_server.cpp - rc server functions
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
#include "servers.h"
using std::string;

#define RC_MSG_LENGTH   1024

typedef struct rc_msg
{
    rc_msg(char* buffer) : command(buffer)
    {
    }
    string command;
    string ip;
} rc_msg_t;

extern void delete_card(std::string &req, int KC_ID);

void change_room_status(mysqlpp::Connection* conn, string ip, int room)
{
    std::ostringstream str;

    str << "UPDATE RoomStatus SET OnLine_RC=" << "'" << (ctxt.rooms[ip]->status == 0 ? "FALSE" : "TRUE") << "'" << ", OnLine_RCDate="
            << "'" << get_date_time << "'" << " WHERE RoomID=" << room;
    logger(INFO, "Room %d OnLine %s\n", room, ctxt.rooms[ip]->status == 0 ? "FALSE" : "TRUE");

    mysqlpp::Query qr = conn->query();
    mysqlpp::SimpleResult res = DB_EXECUTE(str.str().c_str());
}

void check_room_status(void*)
{
    std::map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.begin();
    time_t now = time(NULL);

    mysqlpp::Connection::thread_start();

    BEGIN_DB(ctxt.db_pool[0])
    for(; i != ctxt.rooms.end(); i++)
    {
        pthread_mutex_lock(&i->second->lock);
        if(difftime(now, i->second->lastping)> 120)
        {
            i->second->status = 0;

            if(i->second->oldstatus != i->second->status)
            {
                i->second->oldstatus = i->second->status;
                i->second->lastchange = now;

                change_room_status(conn, i->first, i->second->room);
            }
        }
        pthread_mutex_unlock(&i->second->lock);
    }
    END_DB(ctxt.db_pool[0])

    mysqlpp::Connection::thread_end();
}

void check_card_validity(void*)
{
    using namespace std;

    mysqlpp::Connection::thread_start();
    BEGIN_DB(ctxt.db_pool[0])
        ostringstream str;

        str << "SELECT HEX(KeyID) as HexKeyID FROM Keycard WHERE ValidityDateTime<='" << get_date_time << "' AND ValidityDateTime<>'0000-00-00 00:00:00'";

        mysqlpp::StoreQueryResult res = DB_STORE(str.str().c_str());

        for (size_t i = 0; i < res.num_rows(); ++i)
        {
            logger(INFO, "Deleting invalid card %s\n", res[i][0].c_str());

            std::ostringstream req;

            req << "KD|KI" << res[i][0] << "|EI0|";

            std::string _t = req.str();
            delete_card(_t, 0);
        }
    END_DB(ctxt.db_pool[0])
    mysqlpp::Connection::thread_end();
}

void set_temp(rc_msg_t* msg, uint16 tzone, uint16 season, uint16 actual, uint16 target, uint16 target_no_guest, uint16 mode,
        uint16 status)
{
    map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(msg->ip);
    if (i == ctxt.rooms.end())
        return;

    BEGIN_DB(ctxt.db_pool[0])
        std::ostringstream str;

        str << "SELECT TargetT,DefaultGuestT_EmptyRoom, Mode, DefaultFreeT, ActualT, Status FROM RoomTZone WHERE RoomID=" << ctxt.rooms[msg->ip]->room << " AND TZoneID=" << tzone+1;

        mysqlpp::StoreQueryResult res = DB_STORE(str.str().c_str());

        if (res.num_rows()> 0)
        {
            {
                std::ostringstream update_q;

                update_q << "UPDATE RoomTZone SET ActualT=" << actual <<",Status="<< status <<" WHERE RoomID=" << ctxt.rooms[msg->ip]->room <<" AND TZoneID=" << tzone + 1;

                qr.exec(update_q.str().c_str());
            }

//          std::ostringstream temp_q;
//          temp_q << "SELECT TargetT, ActualT, MODE,Status FROM RoomTZoneLog WHERE RoomID=" << ctxt.rooms[msg->ip]->room << " AND TZoneID=" << tzone + 1 << " ORDER BY ID DESC LIMIT 1";
//
//          mysqlpp::StoreQueryResult temp_res = DB_STORE(temp_q.str().c_str());
//
//          if(temp_res.num_rows()> 0)
//          if(((target != (int) temp_res[0][0]) || (actual != (int)temp_res[0][1]) || (mode != (int) temp_res[0][2]) || ( status != (int) temp_res[0][3])))
//          {
//              std::ostringstream insert_q;
//
//              insert_q << "INSERT INTO RoomTZoneLog (RoomID, TZoneID, Date, TargetT, ActualT, Mode, Status) VALUES("<<ctxt.rooms[msg->ip]->room<<", "<<tzone+1<<", '" << get_date_time << "', "<<target << ", "<< actual <<", " << mode << ", " << status << ")";
//
//              qr.exec(insert_q.str().c_str());
//          }
            int targetT = target;
            int targetTnoGuest = target_no_guest;

            int seson = get_season(qr);

            bool bSend = false;
            {
                std::ostringstream guest_q;

                guest_q << "SELECT GuestNo FROM Room WHERE RoomID=" << ctxt.rooms[msg->ip]->room;

                mysqlpp::StoreQueryResult res2 = DB_STORE(guest_q.str().c_str());

                if(target == 0)
                {
                    bSend = true;

                    if((int) res2[0][0] != 0)
                    {
                        targetT = res[0][0];
                        targetTnoGuest = res[0][1];
                    }
                    else
                    {
                        targetT = res[0][3];
                        targetTnoGuest = res[0][3];
                    }
                }
                else
                {
                    if((int) res2[0][0] != 0)
                    {
                        if(target != (int)res[0][0])
                        {
                            std::ostringstream update_temp;

                            update_temp << "UPDATE RoomTZone SET TargetT=" << target << " WHERE RoomID=" << ctxt.rooms[msg->ip]->room << " AND TZoneID=" << tzone + 1;
                            qr.exec(update_temp.str().c_str());
                            bSend = false;
                        }
                        if(target_no_guest != (int)res[0][1] || mode != (int) res[0][2] || seson != season)
                        {
                            bSend = true;
                            targetTnoGuest = res[0][1];
                        }
                    }
                    else
                    {
                        if(target_no_guest != (int)res[0][3] || mode != (int) res[0][2] || seson != season)
                        {
                            bSend = true;
                            targetTnoGuest = res[0][3];
                        }
                    }
                }

                if(bSend)
                {
                    std::ostringstream udp_send;

                    udp_send.fill('0');
                    udp_send << "CLMT:" << std::setw(2) << tzone << seson << std::setw(2) << targetT << std::setw(2) << targetTnoGuest << res[0][2] << "*";

                    send_to_rc(msg->ip, udp_send);
                }
            }
        }
        else
            logger(ERROR, "Unrecognized zone for room %d\n", ctxt.rooms[msg->ip]->room);

    END_DB(ctxt.db_pool[0])
}

void fail_set_temp(rc_msg_t* msg, uint16 aa, uint16 r)
{
    map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(msg->ip);
    if (i == ctxt.rooms.end())
        return;

    string reason;

    switch (r)
    {
    case 1:
        reason = "невалидна кл.зона";
        break;
    case 2:
        reason = "липсва връзка към кл.зона";
        break;
    case 3:
        reason = "неуспешна промяна на температурата";
        break;
    case 4:
        reason = "невалидна температура";
        break;
    case 5:
        reason = "невалиден режим на работа";
        break;
    }
    std::ostringstream message;

    message << "Възникна проблем в климатичния контрол на стая " << ctxt.rooms[msg->ip]->room << " (кл. зона " << aa + 1
            << ". Причина: " << reason;

    write_event(4, message);
}

void door_control(rc_msg_t* msg, uint16 nDoor, uint16 nSafe, uint16 nPanic, uint16 nR, uint16 nz1, uint16 nz2, uint16 nz3, uint16 nz4,
        uint16 nz5, uint16 nz6, uint16 ngreen, uint16 nred)
{
    map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(msg->ip);
    if (i == ctxt.rooms.end())
        return;

    BEGIN_DB(ctxt.db_pool[0])
        std::ostringstream str;

        str << "SELECT Bathroom,SafeOpen FROM RoomStatus WHERE RoomID=" << ctxt.rooms[msg->ip]->room;
        //  std::cout<< str << std::endl;

        mysqlpp::StoreQueryResult res = DB_STORE(str.str().c_str());

        string bathroom_panic, safe_open;

        if(res.num_rows()> 0)
        {
            bathroom_panic = res[0][0].c_str();
            safe_open = res[0][1].c_str();
        }
        else
        {
            bathroom_panic = "FALSE";
            safe_open = "FALSE";
        }

        std::ostringstream str_room_status;
        std::ostringstream str_access_control;

        string door = bool_to_str(nDoor);
        string safe = bool_to_str(nSafe);
        string panic = bool_to_str(nPanic);
        string r = bool_to_str(nR);
        string z1 = bool_to_str(nz1);
        string z2 = bool_to_str(nz2);
        string z3 = bool_to_str(nz3);
        string z4 = bool_to_str(nz4);
        string z5 = bool_to_str(nz5);
        string z6 = bool_to_str(nz6);
        string green = bool_to_str(ngreen);
        string red = bool_to_str(nred);

        str_room_status << "UPDATE RoomStatus SET DoorOpen='"<<door<<"', SafeOpen='"<<safe<<"', Bathroom='"<<panic<<"', Tampering='"<<r<<"', Zone1='"<<z1<<"', Zone2='"<<z2<<"', Zone3='"<<z3<<"', Zone4='"<<z4<<"', Zone5='"<<z5<<"', Zone6='"<<z6<<"', GreenIndic='"<<green<<"', RedIndic='"<<red<<"' WHERE RoomID="<< ctxt.rooms[msg->ip]->room;

        //      std::cout<< str_room_status.str() << std::endl;
        DB_EXECUTE(str_room_status.str().c_str());

        str_access_control << "INSERT INTO AccessControlLog (RoomID, Date, DoorOpen, SafeOpen, Bathroom, Tampering, Zone1, Zone2, Zone3, Zone4, Zone5, Zone6, GreenIndic, RedIndic) VALUES ("<<ctxt.rooms[msg->ip]->room<<", '"<<get_date_time<<"', '"<<door<<"', '"<<safe<<"', '"<< panic <<"', '"<<r<<"', '"<<z1<<"', '"<<z2<<"', '"<<z3<<"', '"<<z4<<"', '"<<z5<<"', '"<<z6<<"','"<<green<<"','"<<red<<"')";
        //      std::cout<< str_access_control.str() << std::endl;
        DB_EXECUTE(str_access_control.str().c_str());

        if(!bathroom_panic.compare("FALSE") && bathroom_panic.compare(panic))
        {
            std::ostringstream str_panic;
            str_panic << "Подаден е сигнал за паника от стая "<<ctxt.rooms[msg->ip]->room<<"!";

            write_event(5, str_panic);
        }

        if(!safe_open.compare("FALSE") && safe_open.compare(safe))
        {
            std::ostringstream str_panic;
            str_panic << "Прекъснат датчик за сейф от стая "<<ctxt.rooms[msg->ip]->room<<".Вероятна кражба!";

            write_event(6, str_panic);
        }
    END_DB(ctxt.db_pool[0])
}

void fail_prog_card(rc_msg_t* msg, string &param, uint16 error)
{
    map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(msg->ip);
    if (i == ctxt.rooms.end())
        return;

    std::string message;
    pthread_mutex_lock(&ctxt.rooms[msg->ip]->lock);
    switch (error)
    {
        case 1:
            message = "няма достатъчно памет";
            break;
        case 2:
            message = "грешка в паметта на контролера";
            break;
        case 3:
            message = "невалидна чексума";
            break;
        case 4:
        {
            message = "номерът вече съществува";
            std::map<std::string, bool>::iterator some = ctxt.rooms[msg->ip]->key_map_send.begin();

            for (; some != ctxt.rooms[msg->ip]->key_map_send.end(); some++)
            {
                const char* n;
                if ((n = strcasestr(some->first.c_str(), param.c_str())) != NULL)
                {
                    ctxt.rooms[msg->ip]->key_map_send.erase(some);
                    break;
                }
            }
            break;
        }
    }

    pthread_cond_broadcast(&ctxt.rooms[msg->ip]->queue_not_empty);
    pthread_mutex_unlock(&ctxt.rooms[msg->ip]->lock);

    std::ostringstream str_err_msg;

    str_err_msg << "Неуспешно програмиране на карта 0x" << param << " за стая " << ctxt.rooms[msg->ip]->room << ". Причина: "
            << message;

    write_event(4, str_err_msg);
}

void ok_prog_card(rc_msg_t* msg, string& card)
{
    map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(msg->ip);
    if (i == ctxt.rooms.end())
        return;

    pthread_mutex_lock(&ctxt.rooms[msg->ip]->lock);
    std::map<std::string, bool>::iterator some = ctxt.rooms[msg->ip]->key_map_send.find(card);

    if (some != ctxt.rooms[msg->ip]->key_map_send.end())
        ctxt.rooms[msg->ip]->key_map_send.erase(some);

    pthread_cond_broadcast(&ctxt.rooms[msg->ip]->queue_not_empty);
    pthread_mutex_unlock(&ctxt.rooms[msg->ip]->lock);
}

void ok_del_card(rc_msg_t* msg, string& card)
{
    map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(msg->ip);
    if (i == ctxt.rooms.end())
        return;

    pthread_mutex_lock(&ctxt.rooms[msg->ip]->lock);
    std::map<std::string, bool>::iterator some = ctxt.rooms[msg->ip]->key_map_send.find(card);

    if (some != ctxt.rooms[msg->ip]->key_map_send.end())
        ctxt.rooms[msg->ip]->key_map_send.erase(some);

    pthread_cond_broadcast(&ctxt.rooms[msg->ip]->queue_not_empty);
    pthread_mutex_unlock(&ctxt.rooms[msg->ip]->lock);
}

void fail_del_card(rc_msg_t* msg, string &param, uint16 error)
{
    map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(msg->ip);
    if (i == ctxt.rooms.end())
        return;

    std::string message;
    switch (error)
    {
        case 1:
            message = "картата не съществува";
            break;
        case 2:
            message = "грешка в паметта на контролера";
            break;
        case 3:
            message = "невалидна чексума";
            break;
    }

    pthread_mutex_lock(&ctxt.rooms[msg->ip]->lock);
    std::map<std::string, bool>::iterator some = ctxt.rooms[msg->ip]->key_map_send.begin();

    for (; some != ctxt.rooms[msg->ip]->key_map_send.end(); some++)
    {
        const char* n;
        if ((n = strcasestr(some->first.c_str(), param.c_str())) != NULL)
        {
            ctxt.rooms[msg->ip]->key_map_send.erase(some);
            break;
        }
    }
    pthread_mutex_unlock(&ctxt.rooms[msg->ip]->lock);

    std::ostringstream str_err_msg;

    str_err_msg << "Неуспешно изтриване на карта 0x" << param << " за стая " << ctxt.rooms[msg->ip]->room << ". Причина: " << message;

    write_event(4, str_err_msg);
}

void stop_radio(std::string &ip)
{
    int radio = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
//  if (ctxt.pmssock == -1)
//  {
//      logger(ERROR, "Can't create socket for radio server: %s\n", strerror(errno));
//
//      return;
//  }

    struct sockaddr_in isa;

    isa.sin_family = AF_INET;
    isa.sin_port = htons(ctxt.conf->radio_port);
    inet_aton(ctxt.conf->radio_host.c_str(), &isa.sin_addr);

    try
    {
//      do
        {
//          pollfd pfd;
//          pfd.fd = radio;
//          pfd.events = POLLOUT;
//
//          int ready = poll(&pfd, 1, 2000);

        //  if (ready != -1)
            {
                //      printf("%.2x\n", pfd.revents);
            //  if (pfd.revents == POLLHUP)
                {
                    int ret = connect(radio, (sockaddr*) &isa, sizeof(isa));
                    if (ret == -1)
                    {
                        logger(ERROR, "Can not connect to IP address %s: %s\n", ctxt.conf->radio_host.c_str(), strerror(errno));
                        //break;
                    }
                    else
                    {
                        std::ostringstream buffer;
                        buffer << "RADC|RA" << ctxt.rooms[ip]->room << "|ON0|\r\n";

                        send(radio, buffer.str().c_str(), buffer.str().length(), 0);
                        //break;
                    }

                }

            }
//          else
//          {
//              throw errno;
//          }

        } //while (ctxt.shutdown != 1);
    } catch (int& n)
    {
        logger(FATAL, "Error in radio server on IP address %s: %s\n", ctxt.conf->radio_host.c_str(), strerror(n));
    }

    shutdown(radio, SHUT_RDWR);
    close(radio);
}

void power_on(rc_msg_t* msg, std::string& param)
{
    map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(msg->ip);
    if (i == ctxt.rooms.end())
        return;

    BEGIN_DB(ctxt.db_pool[0])
        std::ostringstream get_key;

        get_key << "SELECT HEX(PowerKeyID) FROM RoomStatus WHERE RoomID=" << ctxt.rooms[msg->ip]->room;

        mysqlpp::StoreQueryResult res = DB_STORE(get_key.str().c_str());

        std::string cur_id = "0";

        if(res.num_rows()> 0)
        {
            cur_id = res[0][0].c_str();
        }

        if(cur_id.compare(param))
        {
            std::ostringstream update_room_status;
            std::ostringstream update_door_lock;

            update_room_status << "UPDATE RoomStatus SET PowerKeyID=0x"<<param<<" WHERE RoomID="<<ctxt.rooms[msg->ip]->room;
            DB_EXECUTE(update_room_status.str().c_str());

            int event;

            if(!param.compare("0000000000000000"))
            {
                event = 5;
                stop_radio(msg->ip);
            }
            else
            {
                event = 4;
            }

            update_door_lock << "INSERT INTO DoorLockLog (EventDate, RoomID, EventID, KeyID) VALUES('"<< get_date_time <<"', " << ctxt.rooms[msg->ip]->room << ", "<<event<<", 0x"<<param<<")";

            DB_EXECUTE(update_door_lock.str().c_str());
        }
        END_DB(ctxt.db_pool[0])
    }

void open_door(rc_msg* msg, std::string& param)
{
    map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(msg->ip);
    if (i == ctxt.rooms.end())
        return;

    BEGIN_DB(ctxt.db_pool[0])
        std::ostringstream update_door_lock;

        update_door_lock << "INSERT INTO DoorLockLog (EventDate, RoomID, EventID, KeyID) VALUES('"<< get_date_time <<"', " << ctxt.rooms[msg->ip]->room << ", 3, 0x"<<param<<")";

        DB_EXECUTE(update_door_lock.str().c_str());
    END_DB(ctxt.db_pool[0])

}

void is_alive(rc_msg_t* msg, string& aa)
{
    map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(msg->ip);
    if (i == ctxt.rooms.end())
        return;

    BEGIN_DB(ctxt.db_pool[0])
        pthread_mutex_lock(&ctxt.rooms[msg->ip]->lock);
        ctxt.rooms[msg->ip]->status = 1;
        ctxt.rooms[msg->ip]->lastping = time(NULL);
        if(ctxt.rooms[msg->ip]->oldstatus != 1)
        {
            ctxt.rooms[msg->ip]->oldstatus = 1;
            ctxt.rooms[msg->ip]->lastchange = time(NULL);

            change_room_status(conn, msg->ip, ctxt.rooms[msg->ip]->room);
        }
        pthread_mutex_unlock(&ctxt.rooms[msg->ip]->lock);

        std::ostringstream get_cards;

        get_cards << "SELECT HEX(KeyID), IsUnlock, IsPower FROM KeycardRoom WHERE RoomID="<< ctxt.rooms[msg->ip]->room <<" ORDER BY KeyID asc";

        mysqlpp::StoreQueryResult res = DB_STORE(get_cards.str().c_str());

        for(size_t i = 0; i < res.num_rows(); i++)
        {
            std::ostringstream key;

            key << res[i][0].c_str() << str_to_int(res[i][1].c_str()) << str_to_int(res[i][2].c_str());

            string _t = key.str();

            pthread_mutex_lock(&ctxt.rooms[msg->ip]->lock);
            ctxt.rooms[msg->ip]->key_map[_t] = false;
            pthread_mutex_unlock(&ctxt.rooms[msg->ip]->lock);
        }

        uint16 crc16 = 0;
        pthread_mutex_lock(&ctxt.rooms[msg->ip]->lock);
        std::map<string, bool>::iterator some = ctxt.rooms[msg->ip]->key_map.begin();
        for(; some != ctxt.rooms[msg->ip]->key_map.end(); some++)
        {
            const char* c = some->first.c_str();
            for(int i = 0; i < 18; i++)
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
        }

        bool status = ctxt.rooms[msg->ip]->prog;
        pthread_mutex_unlock(&ctxt.rooms[msg->ip]->lock);

        uint16 crc_command = strtol(aa.c_str(), (char**)NULL, 16);

        if(crc_command != crc16 && status == false)
        {
            logger(ERROR, "Different crc %X for rc %s. Must be %X", crc_command, msg->ip.c_str(), crc16);
            if(ctxt.rooms[msg->ip]->key_map_send.size() == 0)
            {
                pthread_mutex_lock(&ctxt.rooms[msg->ip]->lock);
                ctxt.rooms[msg->ip]->key_map_send[std::string("0000000000000000")] = true;
                std::map<string, bool>::iterator some = ctxt.rooms[msg->ip]->key_map.begin();
                for(; some != ctxt.rooms[msg->ip]->key_map.end(); some++)
                {
                    ctxt.rooms[msg->ip]->key_map_send[some->first] = some->second;
                }
                pthread_mutex_unlock(&ctxt.rooms[msg->ip]->lock);
            }

            int nTimeOuts = 0;

            //while(ctxt.rooms[msg->ip]->key_map_send.size() != 0)
            pthread_mutex_lock(&ctxt.rooms[msg->ip]->lock);
            std::map<string, bool>::iterator it = ctxt.rooms[msg->ip]->key_map_send.begin();
            std::map<string, bool> temp;
            for(;it != ctxt.rooms[msg->ip]->key_map_send.end(); it++)
            {
                temp[it->first] = it->second;
            }

            ctxt.rooms[msg->ip]->prog = true;
            ctxt.rooms[msg->ip]->tryprog ++;
            pthread_mutex_unlock(&ctxt.rooms[msg->ip]->lock);
            std::map<string, bool>::iterator some = temp.begin();
            for(;some != temp.end(); some++)
            {
                std::ostringstream udp_send;

                udp_send.fill('0');
                if(some->second == false)
                udp_send << "PROG:" << some->first << "*";
                else
                {
                    std::string _t = some->first;
                    //          _t[_t.length() - 1] = '\0';
                    //          _t[_t.length() - 1] = '\0';
                    udp_send <<"BLNK:" << _t << "*";
                }

                //CRC(udp_send);

                send_to_rc(msg->ip, udp_send);

                timeval now;
                gettimeofday(&now, NULL);
                sleep(1);
                //   ctxt.rooms[msg->ip]->key_map.erase(some);
                timespec timeout;
                timeout.tv_sec = now.tv_sec + 2;
                timeout.tv_nsec = now.tv_usec * 1000;
                int retcode = 0;

                //retcode = pthread_cond_timedwait(&ctxt.rooms[msg->ip]->queue_not_empty, &ctxt.rooms[msg->ip]->lock, &timeout);

                if(retcode == ETIMEDOUT)
                {
                    std::cout << "timeout\n";

                    nTimeOuts++;

                    if(nTimeOuts> 14)
                    {
                        pthread_mutex_unlock(&ctxt.rooms[msg->ip]->lock);
                        break;
                    }
                }
            }
            //  pthread_mutex_unlock(&ctxt.rooms[msg->ip]->lock);

            //      if(nTimeOuts > 14)

            {
                pthread_mutex_lock(&ctxt.rooms[msg->ip]->lock);
                while(ctxt.rooms[msg->ip]->key_map.size() != 0)
                {
                    std::map<string, bool>::iterator some = ctxt.rooms[msg->ip]->key_map.begin();
                    ctxt.rooms[msg->ip]->key_map.erase(some);
                }
                ctxt.rooms[msg->ip]->prog = false;
                pthread_mutex_unlock(&ctxt.rooms[msg->ip]->lock);
            }

            pthread_mutex_lock(&ctxt.rooms[msg->ip]->lock);
            if(ctxt.rooms[msg->ip]->tryprog> 9)
            {
                logger(ERROR, "Programming timeout for rc %s room %d\n", msg->ip.c_str(), ctxt.rooms[msg->ip]->room);
                ctxt.rooms[msg->ip]->tryprog = 0;
                while(ctxt.rooms[msg->ip]->key_map.size() != 0)
                {
                    std::map<string, bool>::iterator some = ctxt.rooms[msg->ip]->key_map.begin();
                    ctxt.rooms[msg->ip]->key_map.erase(some);
                }
                while(ctxt.rooms[msg->ip]->key_map_send.size() != 0)
                {
                    std::map<string, bool>::iterator some = ctxt.rooms[msg->ip]->key_map_send.begin();
                    ctxt.rooms[msg->ip]->key_map_send.erase(some);
                }
            }
            pthread_mutex_unlock(&ctxt.rooms[msg->ip]->lock);
        }
        else
        {
            pthread_mutex_lock(&ctxt.rooms[msg->ip]->lock);
            ctxt.rooms[msg->ip]->tryprog = 0;
            while(ctxt.rooms[msg->ip]->key_map.size() != 0)
            {
                std::map<string, bool>::iterator some = ctxt.rooms[msg->ip]->key_map.begin();
                ctxt.rooms[msg->ip]->key_map.erase(some);
            }
            while(ctxt.rooms[msg->ip]->key_map_send.size() != 0)
            {
                std::map<string, bool>::iterator some = ctxt.rooms[msg->ip]->key_map_send.begin();
                ctxt.rooms[msg->ip]->key_map_send.erase(some);
            }
            pthread_mutex_unlock(&ctxt.rooms[msg->ip]->lock);
        }
    END_DB(ctxt.db_pool[0])

        //  progs ++;
        //  if(progs == 10)
        //  {
        //      progs = 0;
        //
        //       std::ostringstream udp_send;
        //
        //                  udp_send.fill('0');
        //                                //udp_send << "PROG:" << some->first << "*";
        //                udp_send <<"BLNK:0000000000000000*";
        //            std::cout<<udp_send.str()<<std::endl;
        //
        //                  //CRC(udp_send);
        //
        //                  send_to_rc(msg->ip, udp_send);
        //
        //  }
}

void reset_rc(rc_msg_t* msg, string& param)
{
    map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(msg->ip);
    if (i == ctxt.rooms.end())
        return;

    std::ostringstream message;

    message << "Рестартиран е RC" << ctxt.rooms[msg->ip]->room << " (номер на рестарта: " << param;

    write_event(2, message);
}

void pannic_off(rc_msg_t* msg)
{
    map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(msg->ip);
    if (i == ctxt.rooms.end())
        return;

    BEGIN_DB(ctxt.db_pool[0])
        std::ostringstream room_status;

        room_status << "UPDATE RoomStatus SET  Bathroom='FALSE' WHERE RoomID=" << ctxt.rooms[msg->ip]->room;
        DB_EXECUTE(room_status.str().c_str());

        std::ostringstream access_log;

        access_log << "INSERT INTO AccessControlLog( RoomID, Date, DoorOpen, SafeOpen, Bathroom, GreenIndic, RedIndic, Tampering, Zone1, Zone2, Zone3, Zone4, Zone5, Zone6) SELECT " << ctxt.rooms[msg->ip]->room << ", '"<< get_date_time <<"', RoomStatus.DoorOpen, RoomStatus.SafeOpen, 'FALSE', RoomStatus.GreenIndic, RoomStatus.RedIndic, RoomStatus.Tampering, RoomStatus.Zone1, RoomStatus.Zone2, RoomStatus.Zone3, RoomStatus.Zone4, RoomStatus.Zone5, RoomStatus.Zone6 FROM RoomStatus WHERE RoomStatus.RoomID=" << ctxt.rooms[msg->ip]->room;
        DB_EXECUTE(access_log.str().c_str());
    END_DB(ctxt.db_pool[0])
}

void fail_pannic_off(rc_msg_t* msg, uint16 error)
{
    map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(msg->ip);
    if (i == ctxt.rooms.end())
        return;

    std::string message;
    switch (error)
    {
    case 0:
        message = "неизвестна";
        break;
    default:
        message = "";
        break;
    }

    std::ostringstream str_err_msg;

    str_err_msg << "Неуспешно изключване на аларма в стая " << ctxt.rooms[msg->ip]->room << ". Причина: " << message;

    write_event(4, str_err_msg);
}

void set_indc(rc_msg_t* msg, uint16 green, uint16 red)
{
    map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(msg->ip);
    if (i == ctxt.rooms.end())
        return;

    BEGIN_DB(ctxt.db_pool[0])
        std::ostringstream access_log;

        access_log << "INSERT INTO AccessControlLog ( RoomID, Date, DoorOpen, SafeOpen, Bathroom, GreenIndic, RedIndic, Tampering, Zone1, Zone2, Zone3, Zone4, Zone5, Zone6) SELECT RoomStatus.RoomID, '"<<get_date_time<<"', RoomStatus.DoorOpen, RoomStatus.SafeOpen, RoomStatus.Bathroom, '"<<green<<"','"<< red <<"', RoomStatus.Tampering, RoomStatus.Zone1, RoomStatus.Zone2, RoomStatus.Zone3, RoomStatus.Zone4, RoomStatus.Zone5, RoomStatus.Zone6 FROM RoomStatus WHERE RoomStatus.RoomID="<< ctxt.rooms[msg->ip]->room;

        DB_EXECUTE(access_log.str().c_str());
    END_DB(ctxt.db_pool[0])
}

void fail_set_indc(rc_msg_t* msg, uint16 error)
{
    map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(msg->ip);
    if (i == ctxt.rooms.end())
        return;

    std::string message;
    switch (error)
    {
    case 0:
        message = "неизвестна";
        break;
    default:
        message = "";
        break;
    }

    std::ostringstream str_err_msg;

    str_err_msg << "Неуспешно включване/изключване на индикатор в стая " << ctxt.rooms[msg->ip]->room << ". Причина: " << message;

    write_event(4, str_err_msg);
}

void rc_msg_handler(void* msg)
{
    rc_msg_t* rc_msg = static_cast<rc_msg_t*> (msg);

    pcrecpp::RE("(\\s+)").Replace("", &rc_msg->command);

    string command;
    string crc;
    pcrecpp::StringPiece input(rc_msg->command);
    pcrecpp::RE("([\\w\\:]+\\*)(\\w+)").FullMatch(input, &command, &crc);

    map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(rc_msg->ip);
    if (i == ctxt.rooms.end())
    {
        if(rc_msg)
            delete rc_msg;

        return;
    }

    mysqlpp::Connection::thread_start();

    logger(RECV, "%s crc: %s from: %s room %d\n", command.c_str(), crc.c_str(), rc_msg->ip.c_str(), ctxt.rooms[rc_msg->ip]->room);

    uint16 crc16 = get_crc(command);

    int crc_command = strtol(crc.c_str(), (char**) NULL, 16);

    if ((unsigned) crc_command == crc16)
    {
        pcrecpp::StringPiece message_real(command);
        uint16 aa, w, ct, tg, tt, m, s, z1, z2, z3, z4, z5, z6;
        std::string param;

        if (pcrecpp::RE("CLMT\\:(\\d{2})(\\d{1})(\\d{2})(\\d{2})(\\d{2})(\\d{1})(\\d+)\\*").FullMatch(message_real, &aa, &w, &ct, &tg,
                &tt, &m, &s))
        {
            set_temp(rc_msg, aa, w, ct, tg, tt, m, s);
        }
        else if (pcrecpp::RE("CLFL\\:(\\d{2})(\\d{1})\\*").FullMatch(message_real, &aa, &w))
        {
            fail_set_temp(rc_msg, aa, w);
        }
        else if (pcrecpp::RE(
                "SCRT\\:(\\d{1})(\\d{1})(\\d{1})(\\d{1})(\\d{1})(\\d{1})(\\d{1})(\\d{1})(\\d{1})(\\d{1})(\\d{1})(\\d{1})\\*").FullMatch(
                message_real, &aa, &w, &ct, &tt, &z1, &z2, &z3, &z4, &z5, &z6, &m, &s))
        {
            //  logger(INFO, "Command %s crc: %s from: %s\n", command.c_str(), crc.c_str(), rc_msg->ip.c_str());
            door_control(rc_msg, aa, w, ct, tt, z1, z2, z3, z4, z5, z6, m, s);
        }
        else if (pcrecpp::RE("NPNC\\:\\*").FullMatch(message_real))
        {
            pannic_off(rc_msg);
        }
        else if (pcrecpp::RE("NPNF\\:(\\d{1})\\*").FullMatch(message_real, &aa))
        {
            fail_pannic_off(rc_msg, aa);
        }
        else if (pcrecpp::RE("INDC\\:(\\d{1})(\\d{1})\\*").FullMatch(message_real, &tt, &tg))
        {
            set_indc(rc_msg, tt, tg);
        }
        else if (pcrecpp::RE("INDF\\:(\\d{1})\\*").FullMatch(message_real, &aa))
        {
            fail_set_indc(rc_msg, aa);
        }
        else if (pcrecpp::RE("PROG\\:(\\w{18})\\*").FullMatch(message_real, &param))
        {
            //logger(INFO, "Command %s crc: %s from: %s\n", command.c_str(), crc.c_str(), rc_msg->ip.c_str());
            ok_prog_card(rc_msg, param);
        }
        else if (pcrecpp::RE("PRFL\\:(\\w{16})(\\d{1})\\*").FullMatch(message_real, &param, &aa))
        {
            //  logger(INFO, "Command %s crc: %s from: %s\n", command.c_str(), crc.c_str(), rc_msg->ip.c_str());
            fail_prog_card(rc_msg, param, aa);
        }
        else if (pcrecpp::RE("BLNK\\:(\\w{16})\\*").FullMatch(message_real, &param))
        {
            //  logger(INFO, "Command %s crc: %s from: %s\n", command.c_str(), crc.c_str(), rc_msg->ip.c_str());
            ok_del_card(rc_msg, param);
        }
        else if (pcrecpp::RE("BLFL\\:(\\w{16})(\\d{1})\\*").FullMatch(message_real, &param, &aa))
        {
            //  logger(INFO, "Command %s crc: %s from: %s\n", command.c_str(), crc.c_str(), rc_msg->ip.c_str());
            fail_del_card(rc_msg, param, aa);
        }
        else if (pcrecpp::RE("OPEN\\:(\\w{16})\\*").FullMatch(message_real, &param))
        {
            //  logger(INFO, "Command %s crc: %s from: %s\n", command.c_str(), crc.c_str(), rc_msg->ip.c_str());
            open_door(rc_msg, param);
        }
        else if (pcrecpp::RE("POWR\\:(\\w{16})\\*").FullMatch(message_real, &param))
        {
            power_on(rc_msg, param);
        }
        else if (pcrecpp::RE("ISAL\\:(\\w{4})\\*").FullMatch(message_real, &param))
        {
            pthread_mutex_lock(&ctxt.rooms[rc_msg->ip]->lock);
            //          if(ctxt.rooms[rc_msg->ip]->key_map_send.size() != 0)
            //          {
            //              std::cout<< "fake ISAL! Ignorring sie of keys " << ctxt.rooms[rc_msg->ip]->key_map_send.size() << std::endl;
            //              pthread_mutex_unlock(&ctxt.rooms[rc_msg->ip]->lock);
            //
            //              delete rc_msg;
            //              return;
            //          }
            pthread_mutex_unlock(&ctxt.rooms[rc_msg->ip]->lock);

            is_alive(rc_msg, param);
        }
        else if (pcrecpp::RE("RSET\\:(\\w{10})\\*").FullMatch(message_real, &param))
        {
            //  logger(INFO, "Command %s crc: %s from: %s\n", command.c_str(), crc.c_str(), rc_msg->ip.c_str());
            reset_rc(rc_msg, param);
        }
        else
        {
            logger(ERROR, "Command %s crc: %s from: %s. Command Unknown!\n", command.c_str(), crc.c_str(), rc_msg->ip.c_str());
        }

    }
    else
        logger(ERROR, "Bad CRC\n");

    if(rc_msg)
        delete rc_msg;

    mysqlpp::Connection::thread_end();
}

void*
rc_server(void*)
{
    time_t lastcheck = time(NULL);
    int nCount = 0;

    sigset_t mask;

    sigfillset(&mask); /* Mask all allowed signals */
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    check_room_status(NULL);

    ctxt.rcsock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (ctxt.rcsock == -1)
    {
        logger(ERROR, "Can't create socket for rc server: %s\n", strerror(errno));
        ctxt.shutdown = 1;

        return NULL;
    }

    int reuse = 1;
    setsockopt(ctxt.rcsock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));

    struct sockaddr_in isa;

    isa.sin_family = AF_INET;
    isa.sin_port = htons(ctxt.conf->rc_port);
    inet_aton(ctxt.conf->rc_host.c_str(), &isa.sin_addr);

    try
    {
        if (bind(ctxt.rcsock, (struct sockaddr*) &isa, sizeof isa) == -1)
        {
            throw errno;
        }

        logger(INFO, "Successfully started rc server\n");

        do
        {
            pollfd pfd;
            pfd.fd = ctxt.rcsock;
            pfd.events = POLLIN;

            time_t now = time(NULL);

            if (difftime(now, lastcheck) > 120)
            {
                ctxt.pool->tpool_add_work(check_room_status, NULL );
                lastcheck = time(NULL);
                if (nCount == 14)
                {
                    ctxt.pool->tpool_add_work(check_card_validity, NULL);
                    nCount = 0;
                }
                else
                    nCount++;
            }

            int ready = poll(&pfd, 1, 2000);

            if (ready != -1)
            {
                if (pfd.revents == POLLIN)
                {
                    sockaddr_in client;
                    int received = 0;
                    socklen_t clientlen = sizeof(client);
                    char buffer[RC_MSG_LENGTH];

                    if ((received = recvfrom(ctxt.rcsock, buffer, RC_MSG_LENGTH, 0, (sockaddr *) &client, &clientlen)) < 0)
                    {
                        logger(ERROR, "Error receive data from IP address %s: %s\n", inet_ntoa(client.sin_addr), strerror(errno));
                    }
                    else
                    {
                        buffer[received - 1] = '\0';

                        rc_msg *msg = new rc_msg(buffer);
                        msg->ip = inet_ntoa(client.sin_addr);

                        ctxt.pool->tpool_add_work(rc_msg_handler, msg);
                    }
                }

                continue;
            }
            else
            {
                throw errno;
            }

        } while (ctxt.shutdown != 1);
    } catch (int& n)
    {
        logger(FATAL, "Error in rc server on IP address %s: %s\n", ctxt.conf->rc_host.c_str(), strerror(n));
        ctxt.shutdown = 1;
    }

    shutdown(ctxt.rcsock, SHUT_RDWR);
    close(ctxt.rcsock);
    logger(INFO, "Closing rc server\n");

    return NULL;
}
