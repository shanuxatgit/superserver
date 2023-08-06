/* -------------------------------------------------------------------------
 * mng_server.cpp - mng server functions
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
#include <time.h>
#define PRQ(N) std::cout<<N.str() << std::endl;

extern void check_in(std::string& req);
extern void check_out(std::string& req);

void panic(std::string &req)
{
    int RoomNo = 0;

    DECODE_BEGIN(req)
        EXTRACT("RN(\\d+)", RoomNo)
    DECODE_END()

    if(RoomNo == 0)
        return;

    BEGIN_DB(ctxt.db_pool[0])

        std::ostringstream message;

        message.fill('0');
        message << "NPNC:*";

//      PRQ(message);

        std::string ip = get_rc_ip(RoomNo);
        send_to_rc(ip, message);
    END_DB(ctxt.db_pool[0])
}

inline bool is_client_type(int type)
{
    bool bRes = false;

    for(uint16 i = 0; i < ctxt.conf->client_TypeId.size(); i ++)
    {
        if(ctxt.conf->client_TypeId[i] == type)
        {
            bRes = true;
            break;
        }
    }

    return bRes;
}

inline int get_card_type(mysqlpp::Connection* conn, std::string& key)
{
    std::ostringstream str;

    str << "SELECT KHTypeID  FROM Keycard WHERE KeyID=0x"<<key;

    string _t = str.str();

    mysqlpp::Query qr = conn->query(str.str().c_str());
    mysqlpp::StoreQueryResult res = DB_STORE();

    int keyType = 0;

    if(res.num_rows() > 0)
    {
        keyType = (int) res[0][0];
    }

    return keyType;
}

void delete_card(std::string &req, int KC_ID)
{
    std::string key; int empID = 0; int reqID = 0;

    DECODE_BEGIN(req)
        EXTRACT("KI(\\w{16})",  key)
        EXTRACT("EI(\\d+)",     empID)
        EXTRACT("R\\#(\\d+)",   reqID)
    DECODE_END()

    if(key.empty())
    {
        logger(ERROR, "Delete card error: invalid key\n");
        return;
    }

    req = normalize_string(req);
    BEGIN_DB(ctxt.db_pool[0])
        std::ostringstream data_for_client;
        bool bSend_outer_client = false;

        int i = get_card_type(conn, key);
        int RoomNo = 0;

        std::ostringstream room_no_q;
        room_no_q << "SELECT KHRoomID FROM Keycard WHERE KeyID=0x"  << key;
        mysqlpp::StoreQueryResult room_no_res = DB_STORE(room_no_q.str().c_str());

        if(room_no_res.num_rows() > 0)
        {
            RoomNo = room_no_res[0][0];
        }

        if(is_client_type(i))
        {
            bSend_outer_client = true;
            data_for_client << "KD|KI"<<key<<"|";
        }

        std::ostringstream load_rcs;
        std::vector<std::string> affected_rcs;

        load_rcs << "SELECT IP_RC FROM Room, KeycardRoom WHERE KeycardRoom.RoomID = Room.RoomID AND KeyID=0x"<<key;

        mysqlpp::StoreQueryResult res = DB_STORE(load_rcs.str().c_str());

        for(uint16 i = 0 ; i < res.num_rows(); i++)
        {
            affected_rcs.push_back(std::string(res[i][0].c_str()));
        }

        TRANSACTION_BEGIN(*conn);

            std::ostringstream key_req;
            key_req << "INSERT INTO KeycardReq (ReqType, KCID, ReqText, ReqDate, EmpID, ParentReqID) VALUES (1, "<< KC_ID <<", '"<< req <<"', '" << get_date_time << "', " << empID << ", "<< reqID <<")";
    //      PRQ(key_req);
            DB_EXECUTE(key_req.str().c_str());

            mysqlpp::StoreQueryResult res_req = DB_STORE("SELECT MAX(ReqID) FROM KeycardReq WHERE ReqType=1");

            int ReqID_DB = 0;

            if(res_req.num_rows() > 0)
                ReqID_DB = (int) res_req[0][0];

            std::ostringstream del_key;
            del_key << "DELETE FROM Keycard WHERE KeyID=0x" << key;
            DB_EXECUTE(del_key.str().c_str());

            std::ostringstream del_keyroom;
            del_keyroom << "DELETE FROM KeycardRoom WHERE KeyID=0x" << key;
            DB_EXECUTE(del_keyroom.str().c_str());

            std::ostringstream keycard_log;
            keycard_log << "UPDATE KeycardLog SET EndReqID=" << ReqID_DB << ", End='"<< get_date_time <<"' WHERE KeyID=0x" << key << " AND (EndReqID=0)";
            logger(INFO, "Update KeycarLog mng dell key %s\n", keycard_log.str().c_str());
            DB_EXECUTE(keycard_log.str().c_str());

            std::ostringstream keycard_room_log;
            keycard_room_log << "UPDATE KeycardRoomLog SET EndReqID=" << ReqID_DB << ", End='" << get_date_time << "' WHERE KeyID=0x" << key <<" AND (EndReqID=0)";
            DB_EXECUTE(keycard_room_log.str().c_str());

        TRANSACTION_END();

        if(ctxt.conf->using_autocheckin)
        {
            if(RoomNo != 0)
            {
                std::string RoomName;
                std::ostringstream keycard_q;
                keycard_q << "SELECT hex(KeyID) As KeyN FROM Keycard Where KHRoomID=" << RoomNo <<" AND KHTypeID IN(";
                if(ctxt.conf->guest_TypeId.size() > 0)
                {
                    for(uint16 i = 0; i < ctxt.conf->guest_TypeId.size() - 1; i++)
                    {
                        keycard_q << ctxt.conf->guest_TypeId[i] << ",";
                    }

                    keycard_q << ctxt.conf->guest_TypeId[ctxt.conf->guest_TypeId.size()-1] << ")";
                }
                else
                    keycard_q << "FALSE)";
            //  PRQ(keycard_q);

                mysqlpp::StoreQueryResult keycard_res = DB_STORE(keycard_q.str().c_str());

                if(keycard_res == 0)
                {
                    RoomName = get_room_name(RoomNo);


                    logger(INFO, "Auto CheckOut: RoomID = %d, RoomName = %s", RoomNo, RoomName.c_str());
                    std::ostringstream pms_msg;
                    pms_msg << "GO|RN" << RoomName;
                    std::string msg = pms_msg.str();

                    check_out(msg);
                }
            }
        }

        if(bOK)
        {
            if(bSend_outer_client)
            {
                pthread_mutex_lock(&ctxt.clients_lock);
                std::map<string, int>::iterator i = ctxt.clients.begin();

                for(; i != ctxt.clients.end(); i++)
                {
                    send_to_client(data_for_client, i->second);
                }

                pthread_mutex_unlock(&ctxt.clients_lock);
            }
        }

        std::ostringstream message;

        message.fill('0');
        //message << "BLNK:"<< std::hex << std::uppercase << std::setw(16) << key << "*";
        message << std::hex << std::uppercase << std::setw(16) << key;

        for(uint16 i = 0 ; i < affected_rcs.size(); i++)
        {
            pthread_mutex_lock(&ctxt.rooms[affected_rcs[i]]->lock);
            ctxt.rooms[affected_rcs[i]]->key_map_send[message.str()] = true;
            //send_to_rc(affected_rcs[i], message);
            pthread_mutex_unlock(&ctxt.rooms[affected_rcs[i]]->lock);
        }
    END_DB(ctxt.db_pool[0])
}


void new_card(std::string &req, int KC_ID)
{
    std::string key;
    int empID = 0;
    int reqID = 0;
    int HT = 0;
    std::string HolderName;
    int RoomNo = 0;
    int GuestNo = 0;
    std::string CardData;
    std::string ValidDateTime = "0000-00-00 00:00:00";

    DECODE_BEGIN(req)
        EXTRACT("KI(\\w+)",             key)
        EXTRACT("HT(\\d+)",             HT)
        EXTRACT("HN([\\x20-\\x7B\\x7D-\\xFF]+)",    HolderName)
        EXTRACT("RN(\\d+)",             RoomNo)
        EXTRACT("R\\#(\\d+)",           reqID)
        EXTRACT("EI(\\d+)",             empID)
        EXTRACT("G\\#(\\d+)",           GuestNo)
        EXTRACT("\\$(\\w+)",            CardData)
        EXTRACT("VD([\\w\\s\\-\\:]+)",  ValidDateTime)
    DECODE_END()

    if(key.empty())
    {
        logger(ERROR, "New card error: invalid key\n");
        return;
    }

    req = normalize_string(req);

    BEGIN_DB(ctxt.db_pool[0])
        std::ostringstream data_for_client;
        bool bSend_outer_client = false;

        if(is_client_type(HT))
        {
            bSend_outer_client = true;
            data_for_client << "KN|KI"<<key<<"|HN" << HolderName << "|RN" <<RoomNo<<"|";
        }

        HolderName = normalize_string(HolderName);

        std::ostringstream room_power_lock;

        room_power_lock << "SELECT RoomID, IsUnlock, IsPower FROM KeyHolderTypeZone WHERE KHTypeID="<< HT <<" AND RoomID<>" << RoomNo;
        PRQ(room_power_lock);

        std::vector<std::string> keys;
        std::vector<int> rcs;
        TRANSACTION_BEGIN(*conn);
            {
                std::ostringstream key_req;
                key_req << "INSERT INTO KeycardReq (ReqType, KCID, ReqText, ReqDate, EmpID, ParentReqID) VALUES (1, " << KC_ID <<", '"<< req << "', '" << get_date_time <<"', "<< empID <<", "<< reqID <<");";
                PRQ(key_req);
                DB_EXECUTE(key_req.str().c_str());
            }

            mysqlpp::StoreQueryResult res_req = DB_STORE("SELECT MAX(ReqID) FROM KeycardReq WHERE ReqType=1");
            int ReqID_DB = 0;

            if(res_req.num_rows() > 0)
                ReqID_DB = (int) res_req[0][0];

            std::ostringstream del_key;
            del_key << "INSERT INTO Keycard (KeyID, KHName, KHTypeID, KHRoomID, ValidityDateTime, Begin, BeginReqID, GuestNo, S2) VALUES (0x" << key << ", '"<< HolderName <<"', "<<HT<<", " << RoomNo << ", '"<< ValidDateTime << "', '"<< get_date_time <<"', "<<ReqID_DB<<", "<< GuestNo << ", '" << CardData <<"')";
            PRQ(del_key);
            DB_EXECUTE(del_key.str().c_str());

            std::ostringstream keycard_log;
            keycard_log << "INSERT INTO KeycardLog (KeyID, KHName, KHTypeID, KHRoomID, ValidityDateTime,Begin, BeginReqID, GuestNo, S2) VALUES (0x"<< key << ", '"<< HolderName <<"', " << HT <<", "<< RoomNo <<", '"<<ValidDateTime<<"','"<< get_date_time << "', "<< ReqID_DB <<", " << GuestNo << ", '" << CardData <<"')";
            PRQ(keycard_log);
            DB_EXECUTE(keycard_log.str().c_str());

            if(RoomNo != 0)
            {
                std::ostringstream key_room;
                key_room << "INSERT INTO KeycardRoom (KeyID, RoomID, Begin, BeginReqID, IsUnlock, IsPower) VALUES (0x"<< key <<", "<< RoomNo << ", '"<< get_date_time <<"', "<< ReqID_DB << ", 'TRUE', 'TRUE')";
                PRQ(key_room);
                DB_EXECUTE(key_room.str().c_str());

                std::ostringstream keyroom_log;
                keyroom_log << "INSERT INTO KeycardRoomLog (KeyID, RoomID, Begin, BeginReqID, IsUnlock, IsPower) VALUES (0x"<< key <<", "<< RoomNo <<", '"<< get_date_time <<"', " << ReqID_DB << ", 'TRUE', 'TRUE')";
                PRQ(keyroom_log);
                DB_EXECUTE(keyroom_log.str().c_str());

                std::ostringstream key_composed;
                key_composed.fill('0');
                key_composed << std::hex << std::uppercase << std::setw(16) << key <<"11";

                rcs.push_back(RoomNo);
                keys.push_back(key_composed.str());
            }

            mysqlpp::StoreQueryResult res = DB_STORE(room_power_lock.str().c_str());

            for(uint16 i = 0; i < res.num_rows(); i++)
            {
                std::ostringstream key_room;
                key_room << "INSERT INTO KeycardRoom (KeyID, RoomID, Begin, BeginReqID, IsUnlock, IsPower) VALUES (0x"<< key <<", "<< res[i][0] << ", '"<< get_date_time <<"', "<< ReqID_DB << ", '" << res[i][1] << "', '"<< res[i][2] << "')";
                PRQ(key_room);
                DB_EXECUTE(key_room.str().c_str());

                std::ostringstream keyroom_log;
                keyroom_log << "INSERT INTO KeycardRoomLog (KeyID, RoomID, Begin, BeginReqID, IsUnlock, IsPower) VALUES (0x"<< key <<", "<< res[i][0] <<", '"<< get_date_time <<"', " << ReqID_DB << ", '" << res[i][1] << "', '"<< res[i][2] << "')";
                PRQ(keyroom_log);
                DB_EXECUTE(keyroom_log.str().c_str());

                std::ostringstream key_composed;
                key_composed.fill('0');
                key_composed << std::hex << std::uppercase << std::setw(16) << key << str_to_int(res[i][1].c_str()) << str_to_int (res[i][2].c_str());

                rcs.push_back((int)res[i][0]);
                keys.push_back(key_composed.str());
            }

        TRANSACTION_END();

        if(ctxt.conf->using_autocheckin)
        {
            if(is_client_type(HT))
            {
                if(RoomNo != 0)
                {
                    std::ostringstream empty_room;

                    empty_room <<"SELECT GuestNo FROM Room WHERE RoomID="<< RoomNo;
                    mysqlpp::StoreQueryResult empty_room_res = DB_STORE(empty_room.str().c_str());

                    if(empty_room_res.num_rows() > 0)
                    {
                        int GuestNo = empty_room_res[0][0];

                        if(GuestNo == 0)
                        {
                            std::string RoomName = get_room_name(RoomNo);

                            std::ostringstream pms_msg;
                            logger(INFO, "Auto CheckIn: RoomID = %d, RoomName = %s", RoomNo,RoomName.c_str());
                            pms_msg << "GI|RN" << RoomName << "|G#1|GN"<< HolderName << "|GLEN";
                            std::string msg = pms_msg.str();
                            check_in(msg);
                        }
                    }
                }
            }
        }

        if(bOK)
        {
            if(bSend_outer_client)
            {
                pthread_mutex_lock(&ctxt.clients_lock);
                std::map<string, int>::iterator i = ctxt.clients.begin();

                for(; i != ctxt.clients.end(); i++)
                {
                    send_to_client(data_for_client, i->second);
                }

                pthread_mutex_unlock(&ctxt.clients_lock);
            }
        }

        for(uint16 i = 0; i < rcs.size(); i++)
        {
            std::ostringstream message;

            message.fill('0');
            message << "PROG:"<< keys[i] << "*";

            std::string ip = get_rc_ip((int)rcs[i]);
            std::cout<<ip<< "       ";
            PRQ(message);

            pthread_mutex_lock(&ctxt.rooms[ip]->lock);
            if(ctxt.rooms[ip]->key_map_send.size() == 0)
                send_to_rc(ip, message);

            ctxt.rooms[ip]->key_map_send[keys[i]] = false;
            pthread_mutex_unlock(&ctxt.rooms[ip]->lock);
            //send_to_rc(ip, message);
        }
    END_DB(ctxt.db_pool[0])
}

void new_rc(std::string &req, int KC_ID)
{
    std::string key;
    int empID = 0;
    int HT = 0;
    int RoomNo = 0;
    int nFP;
    int nFU;

    DECODE_BEGIN(req)
        EXTRACT("HT(\\d+)", HT)
        EXTRACT("RN(\\d+)", RoomNo)
        EXTRACT("FU(\\d+)", nFU)
        EXTRACT("EI(\\d+)", empID)
        EXTRACT("FP(\\d+)", nFP)
    DECODE_END()

    std::string FU;
    std::string FP;

    FU = bool_to_str(nFU);
    FP = bool_to_str(nFP);

    BEGIN_DB(ctxt.db_pool[0])
        std::ostringstream keyholder_type;
        keyholder_type << "SELECT KHTypeID FROM KeyHolderTypeZone WHERE KHTypeID="<< HT <<" AND RoomID=" << RoomNo;

        mysqlpp::StoreQueryResult res = DB_STORE(keyholder_type.str().c_str());

        TRANSACTION_BEGIN(*conn);

            {
                std::ostringstream key_req;
                key_req << "INSERT INTO KeycardReq (ReqType, KCID, ReqText, ReqDate, EmpID, ParentReqID) VALUES (1, " << KC_ID <<", '"<< req << "', '" << get_date_time <<"', "<< empID <<", 0);";
                DB_EXECUTE(key_req.str().c_str());
            }

            mysqlpp::StoreQueryResult res_maxID = DB_STORE("SELECT MAX(ReqID) FROM KeycardReq WHERE ReqType=1");

            int reqID = 0;

            if(res_maxID.num_rows() > 0)
            {
                reqID = res_maxID[0][0];
            }

            if(res.num_rows() > 0)
            {
                {
                    std::ostringstream query;
                    query << "UPDATE KeyHolderTypeZone SET IsUnlock='"<< FU <<"', IsPower='" << FP << "' WHERE KHTypeID=" << HT << " AND RoomID=" << RoomNo;
                    DB_EXECUTE(query.str().c_str());
                }
                {
                    std::ostringstream query;
                    query << "UPDATE Keycard INNER JOIN KeycardRoom ON Keycard.KeyID = KeycardRoom.KeyID SET KeycardRoom.IsUnlock='"<< FU <<"', KeycardRoom.IsPower='" << FP <<"' WHERE (Keycard.KHTypeID=" << HT <<") AND (KeycardRoom.RoomID="<< RoomNo<< ") AND (Keycard.KHRoomID<>" << RoomNo << ")";
                    DB_EXECUTE(query.str().c_str());
                }
                {
                    std::ostringstream query;
                    query << "UPDATE Keycard INNER JOIN KeycardRoomLog ON Keycard.KeyID = KeycardRoomLog.KeyID SET KeycardRoomLog.EndReqID=" << reqID <<", KeycardRoomLog.End='"<< get_date_time <<"' WHERE (Keycard.KHTypeID="<< HT << ") AND (KeycardRoomLog.RoomID=" << RoomNo << ") AND (Keycard.KHRoomID<>"<< RoomNo <<") AND (KeycardRoomLog.EndReqID=0)";
                    DB_EXECUTE(query.str().c_str());
                }
                {
                    std::ostringstream query;
                    query << "INSERT INTO KeycardRoomLog (KeyID, RoomID, Begin, BeginReqID, IsUnlock, IsPower) SELECT Keycard.KeyID, "<< RoomNo <<", '"<< get_date_time <<"', "<< reqID <<", '" << FU << "', '" << FP << "' FROM Keycard WHERE (Keycard.KHTypeID=" << HT <<") AND (Keycard.KHRoomID<>" << RoomNo << ")";
                    DB_EXECUTE(query.str().c_str());
                }
            } else {
                {
                    std::ostringstream query;
                    query << "INSERT INTO KeyHolderTypeZone (KHTypeID, RoomID, IsUnlock, IsPower) VALUES (" << HT <<", " << RoomNo <<", '" << FU <<"', '" << FP << "')";
                    DB_EXECUTE(query.str().c_str());
                }
                {
                    std::ostringstream query;
                    query << "INSERT INTO KeycardRoom (KeyID, RoomID, Begin, BeginReqID, IsUnlock, IsPower) SELECT Keycard.KeyID, "<<RoomNo<<", '"<<get_date_time<<"', "<<reqID<<", '"<<FU<<"', '"<<FP<<"' FROM Keycard WHERE (Keycard.KHTypeID="<<HT<<") AND (Keycard.KHRoomID<>"<<RoomNo<<")";
                    DB_EXECUTE(query.str().c_str());

                }
                {
                    std::ostringstream query;
                    query << "INSERT INTO KeycardRoomLog (KeyID, RoomID, Begin, BeginReqID, IsUnlock, IsPower) SELECT Keycard.KeyID, "<<RoomNo<<", '"<<get_date_time<<"', "<<reqID<<", '"<<FU<<"', '"<<FP<<"' FROM Keycard WHERE (Keycard.KHTypeID="<<HT<<") AND (Keycard.KHRoomID<>"<<RoomNo<<")";
                    DB_EXECUTE(query.str().c_str());
                }
            }

        TRANSACTION_END();
    END_DB(ctxt.db_pool[0])
}

void delete_rc(std::string &req, int KC_ID)
{
    int empID = 0;
    int HT = 0;
    int RoomNo = 0;

    DECODE_BEGIN(req)
        EXTRACT("HT(\\d+)", HT)
        EXTRACT("RN(\\d+)", RoomNo)
        EXTRACT("EI(\\d+)", empID)
    DECODE_END()

    std::string FU;
    std::string FP;

    BEGIN_DB(ctxt.db_pool[0])

        TRANSACTION_BEGIN(*conn);

            {
                std::ostringstream key_req;
                key_req << "INSERT INTO KeycardReq (ReqType, KCID, ReqText, ReqDate, EmpID, ParentReqID) VALUES (1, " << KC_ID <<", '"<< req << "', '" << get_date_time <<"', "<< empID <<", 0);";
                DB_EXECUTE(key_req.str().c_str());
            }

            mysqlpp::StoreQueryResult res_maxID = DB_STORE("SELECT MAX(ReqID) FROM KeycardReq WHERE ReqType=1");

            int reqID = 0;

            if(res_maxID.num_rows() > 0)
            {
                reqID = res_maxID[0][0];
            }

            {
                std::ostringstream query;
                query << "UPDATE Keycard INNER JOIN KeycardRoomLog ON Keycard.KeyID = KeycardRoomLog.KeyID SET KeycardRoomLog.EndReqID="<<reqID<<", KeycardRoomLog.End='"<<get_date_time<<"' WHERE (Keycard.KHTypeID="<<HT<<") AND (KeycardRoomLog.RoomID="<<RoomNo<<") AND (Keycard.KHRoomID<>"<<RoomNo<<") AND (KeycardRoomLog.EndReqID=0)";
                DB_EXECUTE(query.str().c_str());
            }
            {
                std::ostringstream query;
                query << "DELETE KeycardRoom.* FROM KeycardRoom INNER JOIN Keycard ON KeycardRoom.KeyID = Keycard.KeyID WHERE (KeycardRoom.RoomID="<<RoomNo<<") AND (Keycard.KHTypeID="<<HT<<") AND (Keycard.KHRoomID<>"<<RoomNo<<")";
                DB_EXECUTE(query.str().c_str());
            }
            {
                std::ostringstream query;
                query << "DELETE FROM KeyHolderTypeZone WHERE KHTypeID="<<HT<<" AND RoomID="<<RoomNo;
                DB_EXECUTE(query.str().c_str());
            }

        TRANSACTION_END();
    END_DB(ctxt.db_pool[0])
}

void new_profile(std::string &req, int KC_ID)
{
    int empID = 0;
    int HT = 0;
    std::string PName;

    DECODE_BEGIN(req)

        EXTRACT("HT(\\d+)",             HT)
        EXTRACT("HN([\\x20-\\x7B\\x7D-\\xFF]+)",    PName)
        EXTRACT("EI(\\d+)",             empID)
    DECODE_END()

    BEGIN_DB(ctxt.db_pool[0])

        if(HT == 0)
        {
            mysqlpp::StoreQueryResult res = DB_STORE("SELECT MAX(KHTypeID) FROM KeyHolderType;");

            if(res.num_rows() > 0)
            {
                HT = (int) res[0][0];
                HT ++;
            }
            else
                HT = 1;
        }

        TRANSACTION_BEGIN(*conn);

            std::ostringstream khtype;
            khtype << "SELECT KHTypeID FROM KeyHolderType WHERE KHTypeID="<<HT;
            mysqlpp::StoreQueryResult res_maxID = DB_STORE(khtype.str().c_str());

            req = normalize_string(req);
            PName = normalize_string(PName);

            {
                std::ostringstream key_req;
                key_req << "INSERT INTO KeycardReq (ReqType, KCID, ReqText, ReqDate, EmpID, ParentReqID) VALUES (1, " << KC_ID <<", '"<< req << "', '" << get_date_time <<"', "<< empID <<", 0);";
                DB_EXECUTE(key_req.str().c_str());
            }

            if(res_maxID.num_rows() > 0)
            {
                std::ostringstream query;
                query << "UPDATE KeyHolderType SET KHTypeName='"<<PName<<"' WHERE KHTypeID="<<HT;
                DB_EXECUTE(query.str().c_str());
            } else {
                std::ostringstream query;
                query << "INSERT INTO KeyHolderType (KHTypeID, KHTypeName) VALUES ("<<HT<<", '"<<PName<<"')";
                DB_EXECUTE(query.str().c_str());
            }

        TRANSACTION_END();
    END_DB(ctxt.db_pool[0])
}

void delete_profile(std::string &req, int KC_ID)
{
    int empID = 0;
    int HT = 0;

    DECODE_BEGIN(req)
        EXTRACT("HT(\\d+)", HT)
        EXTRACT("EI(\\d+)", empID)
    DECODE_END()

    req = normalize_string(req);

    BEGIN_DB(ctxt.db_pool[0])

        TRANSACTION_BEGIN(*conn);

            {
                std::ostringstream key_req;
                key_req << "INSERT INTO KeycardReq (ReqType, KCID, ReqText, ReqDate, EmpID, ParentReqID) VALUES (1, " << KC_ID <<", '"<< req << "', '" << get_date_time <<"', "<< empID <<", 0);";
                DB_EXECUTE(key_req.str().c_str());
            }

            {
                std::ostringstream query;
                query << "DELETE FROM KeyHolderType WHERE KHTypeID="<<HT;
                DB_EXECUTE(query.str().c_str());
            }

        TRANSACTION_END();
    END_DB(ctxt.db_pool[0])
}

void climate(std::string &req)
{
    int RoomNo = 0;

    int TZone = 0;

    int TFDef = 20;
    int TGDef = 20;
    int TNoGDef = 20;
    int Season = 0;    //-->0-Summer; 1-Winter
    int Regime = 0;     //-->0-Without heater; 1-automatic; 2-With heater

    DECODE_BEGIN(req)
        EXTRACT("RN(\\d+)", RoomNo)
        EXTRACT("TZ(\\d+)", TZone)
        EXTRACT("TF(\\d+)", TFDef)
        EXTRACT("TG(\\d+)", TGDef)
        EXTRACT("TE(\\d+)", TNoGDef)
        EXTRACT("RG(\\d+)", Regime)
    DECODE_END()

    BEGIN_DB(ctxt.db_pool[0])

        std::ostringstream guest_no;
        guest_no << "SELECT GuestNo FROM Room WHERE RoomID=" << RoomNo;

        mysqlpp::StoreQueryResult res = DB_STORE(guest_no.str().c_str());
        std::ostringstream clmt;
        Season = get_season(qr);

        clmt.fill('0');
        if((int)res[0][0] == 0)
        {
            std::ostringstream key_req;
            key_req << "UPDATE RoomTZone SET DefaultFreeT="<<TFDef<<", DefaultGuestT="<<TGDef<<", TargetT="<<TFDef<<", DefaultGuestT_EmptyRoom="<<TNoGDef<<", Mode="<<Regime<<" WHERE RoomID="<<RoomNo<<" AND TZoneID="<<TZone;
            DB_EXECUTE(key_req.str().c_str());

            clmt << "CLMT:" << std::setw(2) <<(TZone - 1) << Season << std::setw(2) << TFDef << std::setw(2) << TFDef << Regime <<"*";
        }
        else
        {
            std::ostringstream key_req;
            key_req << "UPDATE RoomTZone SET DefaultFreeT="<<TFDef<<", DefaultGuestT="<<TGDef<<", DefaultGuestT_EmptyRoom="<<TNoGDef<<" ,Mode="<<Regime<<" WHERE RoomID="<<RoomNo<<" AND TZoneID="<<TZone;
            DB_EXECUTE(key_req.str().c_str());

            std::ostringstream target;
            target << "SELECT TargetT FROM RoomTZone WHERE RoomID="<<RoomNo<<" AND TZoneID=" << TZone;
            mysqlpp::StoreQueryResult res_temp = DB_STORE(target.str().c_str());

            int TargetT = 0;

            if(res_temp.num_rows() > 0)
            {
                TargetT = (int) res_temp[0][0];
            }

            if(TargetT == 0) TargetT = TGDef;

            clmt << "CLMT:" << std::setw(2) <<(TZone - 1) << Season << std::setw(2) << TargetT << std::setw(2) << TNoGDef << Regime <<"*";
        }

        std::string ip =get_rc_ip(RoomNo);
        send_to_rc(ip, clmt);
    END_DB(ctxt.db_pool[0])
}

inline int get_KCWS(std::string& ip)
{
    std::map<string, int>::iterator i = ctxt.ks.find(ip);

    if(i == ctxt.ks.end())
        return 0;

    return i->second;
}

void mng_msg_handler(void* msg)
{
    msg_t* _msg = static_cast<msg_t*> (msg);

    mysqlpp::Connection::thread_start();

    pcrecpp::RE("(\\s{2})").Replace("", &_msg->command);

    std::vector<std::string> msg_decomposed;
    pcrecpp::StringPiece input(_msg->command);
    std::string command;

    int KC_ID = get_KCWS(_msg->ip);

    while(pcrecpp::RE("([\\x20-\\x7B\\x7D-\\xFF]+)\\|").Consume(&input, &command))
    {
        msg_decomposed.push_back(command);

    }

    if(msg_decomposed.size() != 0)
    {

    if(msg_decomposed[0] == "TR")
    {
        climate(_msg->command);

    } else if(msg_decomposed[0] == "KN")
    {
        new_card(_msg->command, KC_ID);

    } else if(msg_decomposed[0] == "KD")
    {
        delete_card(_msg->command, KC_ID);

    } else if(msg_decomposed[0] == "HA")
    {
        new_profile(_msg->command, KC_ID);

    }else if(msg_decomposed[0] == "HR")
    {
        delete_profile(_msg->command, KC_ID);

    } else if(msg_decomposed[0] == "ZA")
    {
        new_rc(_msg->command, KC_ID);

    } else if(msg_decomposed[0] == "ZR")
    {
        delete_rc(_msg->command, KC_ID);

    } else if(msg_decomposed[0] == "PR")
    {
        panic(_msg->command);
    }
    }

    if(_msg)
    {
        delete _msg;
        _msg = NULL;
    }

    mysqlpp::Connection::thread_end();
}

void mng_server_accept(void* msg)
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

    if(is_KCWS(ip))
    {
        if(ctxt.pmsbuffer != "")
        {
            send(cfd, ctxt.pmsbuffer.c_str(), ctxt.pmsbuffer.length(), 0);
            ctxt.pmsbuffer.erase();
        }
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

void*
mng_server(void* )
{
    return server(ctxt.mngsock, ctxt.conf->mng_port, ctxt.conf->mng_host, mng_msg_handler, mng_server_accept, &context_t::add_mng, &context_t::del_mng);
}
