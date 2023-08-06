/* -------------------------------------------------------------------------
 * php_server.cpp - php server functions
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
#include <fcntl.h>


void fast_checkout(std::string &req, int cfd)
{
    int RoomNo = 0;
    int GuestNo = 0;
    int Balance;
    DECODE_BEGIN(req)
        EXTRACT("RN(\\d+)", RoomNo)
        EXTRACT("G\\#(\\d+)", GuestNo)
        EXTRACT("BA(\\d+)", Balance)
    DECODE_END()
    logger(INFO, "================ From fast_chekout func RoomID for XC is: %d %d %d\n", RoomNo, GuestNo, Balance);

    if(ctxt.rooms_ip[RoomNo].empty())
        return;

    BEGIN_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
        std::ostringstream guest_q;
        guest_q << "SELECT GuestNo FROM Room WHERE RoomID=" << RoomNo;
        mysqlpp::StoreQueryResult guest_res = DB_STORE(guest_q.str().c_str());

        if(guest_res.num_rows() == 0)
        {
            END_DB_RETURN(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
            return;
        }

        int GuestNo = guest_res[0][0];

        if(ctxt.is_in_php(RoomNo))
        {
            ctxt.insert_php(cfd, RoomNo, 0, 0);
            std::ostringstream msg;

            std::string RoomName = get_room_name(RoomNo);
            msg << "XC|RN"<<RoomName<<"|G#"<< GuestNo << "|BA"<< Balance <<"|DA" << get_date << "|TI" << get_time <<"|";

            if(ctxt.conf->second_interface)
                send_to_pms(msg, ctxt.pmssock2);
            else
                send_to_pms(msg);
        }
    END_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))

}
void wake_up(std::string &req, int cfd)
{
    int RoomNo = 0;
    int wDate;
    int wTime;
    DECODE_BEGIN(req)
        EXTRACT("RN(\\d+)", RoomNo)
        EXTRACT("DA(\\d+)", wDate)
        EXTRACT("TI(\\d+)", wTime)
    DECODE_END()

    // BEGIN_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
        std::ostringstream msg;

        msg << "WA|RN" << RoomNo << "|ASOK|\r\n";
        send(cfd, msg.str().c_str(), msg.str().length(), 0);

        if(ctxt.conf->using_pms){

            std::ostringstream msg_pms;
            std::string RoomName = get_room_name(RoomNo);
            msg_pms << "WR|RN" << RoomName << "|DA" << wDate << "|TI" << wTime <<"|";
            if(ctxt.conf->second_interface)
                send_to_pms(msg_pms, ctxt.pmssock2);
            else
                send_to_pms(msg_pms);
        }
    // END_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))

    process_command(req);
}


void test_func(std::string &req, int)
{
    int RoomNo = 0;

    DECODE_BEGIN(req)
        EXTRACT("RN(\\d+)", RoomNo)
    DECODE_END()
    logger(INFO, "================ From test func RoomID for XL is: %d\n", RoomNo);

}

void billing(std::string &req, int cfd)
{
    int RoomNo = 0;

    DECODE_BEGIN(req)
        EXTRACT("RN(\\d+)", RoomNo)
    DECODE_END()

    if(ctxt.rooms_ip[RoomNo].empty())
        return;

    BEGIN_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
        std::ostringstream guest_q;
        guest_q << "SELECT GuestNo FROM Room WHERE RoomID=" << RoomNo;
        mysqlpp::StoreQueryResult guest_res = DB_STORE(guest_q.str().c_str());

        if(guest_res.num_rows() == 0)
        {
            END_DB_RETURN(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
            return;
        }

        int GuestNo = guest_res[0][0];

        if(ctxt.is_in_php(RoomNo))
        {
            ctxt.insert_php(cfd, RoomNo, 0, 0);
            std::ostringstream msg;

            std::string RoomName = get_room_name(RoomNo);
	    if (ctxt.conf->sendXRwithoutGN){
            	msg << "XR|RN"<<RoomName<<"|";
	    } else {
            	msg << "XR|RN"<<RoomName<<"|G#"<< GuestNo << "|";
	    }
            if(ctxt.conf->second_interface)
                send_to_pms(msg, ctxt.pmssock2);
            else
                send_to_pms(msg);
        }
    END_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
}

void posting(std::string &req, int cfd)
{
    int RoomNo = 0;
    int Amount = 0;
    int PostingType = 0;

    DECODE_BEGIN(req)
        EXTRACT("RN(\\d+)", RoomNo)
        EXTRACT("TA(\\d+)", Amount)
        EXTRACT("TP(\\d+)", PostingType)
    DECODE_END()

    if(RoomNo == 0)
        return;

    make_gen(RoomNo, Amount, PostingType, cfd, "PHP", 0, "", 0, ctxt.conf->eltur_vodid);
}



void other_posting(std::string &req, int cfd)
{
    int RoomNo = 0;
    int Amount = 0;
    int PostingType = 0;

    DECODE_BEGIN(req)
        EXTRACT("RN(\\d+)", RoomNo)
        EXTRACT("TA(\\d+)", Amount)
        EXTRACT("TP(\\d+)", PostingType)
    DECODE_END()

    if(RoomNo == 0)
        return;

    if(ctxt.is_in_php(RoomNo))
    {
        ctxt.insert_php(cfd, RoomNo, 0, 0);
        std::ostringstream msg;
        req.replace( req.find("PO"), 2, "PS");

        if(ctxt.conf->using_pms){

            if(ctxt.conf->second_interface)
                send_to_pms(req, ctxt.pmssock2);
            else
                send_to_pms(req);
        }
    }
}

void who_is (std::string &req, int cfd)
{
    int RoomNo = 0;

    DECODE_BEGIN(req)
        EXTRACT("RN(\\d+)", RoomNo)
    DECODE_END()

    BEGIN_DB(ctxt.db_pool[0])

        int HType = get_h_type(qr, RoomNo);

        std::ostringstream msg;

        msg << "WI|RN" << RoomNo << "|TV";

        if(HType == 0)
            msg << "TN|\r\n";
        else if(HType == 1)
            msg << "TU|\r\n";
        else
            msg << "TM|\r\n";

        send(cfd, msg.str().c_str(), msg.str().length(), 0);
    END_DB(ctxt.db_pool[0])
}

void target_temp(std::string &req, int cfd)
{
    int RoomNo = 0;
    int Amount = 0;

    DECODE_BEGIN(req)
        EXTRACT("RN(\\d+)", RoomNo)
        EXTRACT("MP(\\d+)", Amount)
    DECODE_END()

    if(Amount >= 15 and Amount <= 30)
    {
        if(ctxt.rooms_ip[RoomNo].empty())
            return;

        BEGIN_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))

            std::ostringstream temp_zone;
            temp_zone << "UPDATE RoomTZone SET TargetT = " << Amount <<" WHERE RoomID =" << RoomNo;
            mysqlpp::SimpleResult res = DB_EXECUTE(temp_zone.str().c_str());

            std::ostringstream msg;
            msg << "TS|RN" << RoomNo;
            //if(res.rows() > 0)
            {

                int season = get_season(qr);

                std::ostringstream temp_q;
                temp_q << "SELECT TZoneID, DefaultGuestT_EmptyRoom,Mode FROM RoomTZone WHERE RoomID=" << RoomNo;
                mysqlpp::StoreQueryResult temp_res = DB_STORE(temp_q.str().c_str());

                for(uint16 i = 0; i < temp_res.num_rows(); i++)
                {
                    std::ostringstream msg_rc;

                    msg_rc.fill('0');
                    msg_rc << "CLMT:"<< std::setw(2) << temp_res[i][0] - 1 << season << std::setw(2) << Amount << std::setw(2) << temp_res[i][1] << temp_res[i][2] << "*";

                    std::string ip = get_rc_ip(RoomNo);
                    send_to_rc(ip, msg_rc);
                }

                msg << "|ASOK|\r\n";
            }
            send(cfd, msg.str().c_str(), msg.str().length(), 0);
        END_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
    }
}


void indicator(std::string &req, int cfd)
{
    int RoomNo = 0;
    int GreenIndic = 0;
    int RedIndic = 0;

    DECODE_BEGIN(req)
        EXTRACT("RN(\\d+)", RoomNo)
        EXTRACT("GI(\\d+)", GreenIndic)
        EXTRACT("RI(\\d+)", RedIndic)
    DECODE_END()

    if(ctxt.rooms_ip[RoomNo].empty())
        return;

    BEGIN_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))

        std::ostringstream room_status_q;
        room_status_q << "UPDATE RoomStatus SET GreenIndic=" << GreenIndic << ", RedIndic=" << RedIndic << " WHERE RoomID=" << RoomNo;
        mysqlpp::SimpleResult res = DB_EXECUTE(room_status_q.str().c_str());

        std::ostringstream msg;
        msg << "CS|RN" << RoomNo;

        //if(res.rows() > 0)
        {
            std::ostringstream msg_rc;

            msg_rc.fill('0');
            msg_rc << "INDC:"<< GreenIndic << RedIndic << "*";

            std::string ip = get_rc_ip(RoomNo);
            send_to_rc(ip, msg_rc);

            msg << "|ASOK|\r\n";
        }
    //  else
    //      msg << "|ASUR|\r\n";

        send(cfd, msg.str().c_str(), msg.str().length(), 0);
    END_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
}

#define DIRTY_VACANT        1
#define CLEAN_VACANT        3
#define INSPECTED_VACANT    5


void status(std::string &req, int cfd)
{
    int RoomNo = 0;
    int PHPRoomStatus = 0;
    int RoomStatus = 0;
    int printerPort = 0;
    int guestNo = 0;
    std::string phpClearText;
    std::string doNotDisturb;

    DECODE_BEGIN(req)
        EXTRACT("RN(\\d+)",                 RoomNo)
        EXTRACT("RS(\\d+)",                 PHPRoomStatus)
        EXTRACT("PP(\\d+)",             printerPort)
                EXTRACT("(CT[\\x20-\\x7B\\x7D-\\xFF]+)",        phpClearText)
                EXTRACT("DN([\\x20-\\x7B\\x7D-\\xFF]+)",        doNotDisturb)
    DECODE_END()

    switch (PHPRoomStatus) {
        case 1: RoomStatus = DIRTY_VACANT; break;
        case 2: RoomStatus = CLEAN_VACANT ;break;
        case 3: RoomStatus = INSPECTED_VACANT ;break;
    }

    if(ctxt.rooms_ip[RoomNo].empty())
        return;


    BEGIN_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))

        std::ostringstream guest_q;
        guest_q << "SELECT GuestNo FROM Room WHERE RoomID=" << RoomNo;
        mysqlpp::StoreQueryResult guest_res = DB_STORE(guest_q.str().c_str());

        std::ostringstream msg;
        msg << "CS|RN" << RoomNo;

        if(guest_res.num_rows() > 0)
        {
            if((int) guest_res[0][0] != 0){
                RoomStatus++;
		guestNo = (int) guest_res[0][0];
	    }
        }

            if(RoomStatus >=1 && RoomStatus <= 6)
            {
                std::ostringstream room_status_q;
                room_status_q << "UPDATE RoomStatus SET Status=" << RoomStatus << " WHERE RoomID=" << RoomNo;
                mysqlpp::SimpleResult room_status_res = DB_EXECUTE(room_status_q.str().c_str());

            //  if(room_status_res.rows() > 0)
                {
                    if(ctxt.conf->using_pms)
                    {
                        std::string sRoomName = get_room_name(RoomNo);
                        std::ostringstream msg_pms;
                        if(printerPort){
 			    if(!ctxt.conf->sendMsgWithXL){
                             	msg_pms << "RE|RN" << sRoomName << "|RS" << RoomStatus << "|PP" << printerPort << "|" << phpClearText << "|";
 			    } else {
                             	msg_pms << "XL|RN" << sRoomName << "|$J" << printerPort << "|MI" << RoomStatus << "|G#" << guestNo << "|MT" << phpClearText << "|";
 			    }
                        } else {
                            msg_pms << "RE|RN" << sRoomName << "|RS" << RoomStatus << "|";
                        }
                        if(ctxt.conf->second_interface)
                            send_to_pms(msg_pms, ctxt.pmssock2);
                        else
                            send_to_pms(msg_pms);
                    }

                    msg << "|ASOK|\r\n";
                }
                //else
                //{
                //  msg << "|ASNR|\r\n";
                //  logger(ERROR, "Error setting room stattus in RE room %d", RoomNo);
                //}
            }
            else
                msg << "|ASNR|\r\n";

        send(cfd, msg.str().c_str(), msg.str().length(), 0);
    END_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))

}

void posting_simple_mini(std::string &req, int cfd)
{
    int RoomNo = 0;
    int MA = 0;
    int MN = 0;

    DECODE_BEGIN(req)
        EXTRACT("RN(\\d+)", RoomNo)
        EXTRACT("MA(\\d+)", MA)
        EXTRACT("M\\#(\\d+)", MN)
    DECODE_END()

    // BEGIN_DB(ctxt.db_pool[0])

        if(ctxt.is_in_php(RoomNo))
        {
            ctxt.insert_php(cfd, RoomNo, 0, 0);
            std::string sRoomName = get_room_name(RoomNo);

            std::ostringstream msg;
            msg << "PS|RN" << sRoomName << "|PTM|MA" << MA << "|M#" << MN << "|DA" << get_date << "|TI" << get_time <<"|";

            if(ctxt.conf->second_interface)
                send_to_pms(msg, ctxt.pmssock2);
            else
                send_to_pms(msg);
        }
    // END_DB(ctxt.db_pool[0])
}

void php_msg_handler(void* msg)
{
    msg_t* _msg = static_cast<msg_t*> (msg);

    mysqlpp::Connection::thread_start();

    pcrecpp::RE("(\\s{2})").Replace("", &_msg->command);

    std::vector<std::string> msg_decomposed;
    pcrecpp::StringPiece input(_msg->command);
    std::string command;

//  int KC_ID = get_KCWS(_msg->ip);

    while(pcrecpp::RE("([\\x20-\\x7B\\x7D-\\xFF]+)\\|").Consume(&input, &command))
    {
        msg_decomposed.push_back(command);
    }

    if(msg_decomposed.size() != 0)
    {


        if(msg_decomposed[0] == "XR")
        {
            billing(_msg->command, _msg->sd);

        } else if(msg_decomposed[0] == "PS")
        {
            posting(_msg->command, _msg->sd);

        } else if(msg_decomposed[0] == "WH")
        {
            who_is(_msg->command, _msg->sd);

        } else if(msg_decomposed[0] == "TT")
        {
            target_temp(_msg->command, _msg->sd);

        }else if(msg_decomposed[0] == "WD")
        {
            wake_up(_msg->command, _msg->sd);

        } else if(msg_decomposed[0] == "WC")
        {
            wake_up(_msg->command, _msg->sd);

        } else if(msg_decomposed[0] == "CI")
        {
            indicator(_msg->command, _msg->sd);

        } else if(msg_decomposed[0] == "RE")
        {
            status(_msg->command, _msg->sd);

        } else if(msg_decomposed[0] == "MS")
        {
            posting_simple_mini(_msg->command, _msg->sd);

        } else if(msg_decomposed[0] == "XL")
        {
                    test_func(_msg->command, _msg->sd);

        } else if(msg_decomposed[0] == "XC")
        {
                    fast_checkout(_msg->command, _msg->sd);
        } else if(msg_decomposed[0] == "PO")
        {
                    other_posting(_msg->command, _msg->sd);
        }

    }

    if(_msg)
    {
        delete _msg;
        _msg = NULL;
    }

    mysqlpp::Connection::thread_end();
}

void*
php_server(void* )
{
    return server(ctxt.phpsock, ctxt.conf->php_port, ctxt.conf->php_host, php_msg_handler);
}
