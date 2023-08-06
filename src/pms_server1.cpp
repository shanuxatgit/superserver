/* -------------------------------------------------------------------------
 * pms_server.cpp - pms  server functions
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

string randomString(unsigned int len)
{
   srand(time(0));
   string str = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
   int pos;
   while(str.size() != len) {
    pos = ((rand() % (str.size() - 1)));
    str.erase (pos, 1);
   }
   return str;
}

vector<string> explode( const string &delimiter, const string &str)
{
    vector<string> arr;

    int strleng = str.length();
    int delleng = delimiter.length();
    if (delleng==0)
        return arr;//no change

    int i=0;
    int k=0;
    while( i<strleng )
    {
        int j=0;
        while (i+j<strleng && j<delleng && str[i+j]==delimiter[j])
            j++;
        if (j==delleng)//found delimiter
        {
            arr.push_back(  str.substr(k, i-k) );
            i+=delleng;
            k=i;
        }
        else
        {
            i++;
        }
    }
    arr.push_back(  str.substr(k, i-k) );
    return arr;
}





void update_log(mysqlpp::Query& qr, string type, string report, std::string& host, int bmsID, bool ok)
{
//  Otvariane i zatvariane na Case
//  // Proverka za otvoren ticket
    std::ostringstream case_q;
    case_q << "SELECT case_id FROM system_log_case WHERE ip='" << host <<"' AND case_status='open' AND case_type='"<< type <<"' AND storage_id="<< bmsID;

    mysqlpp::StoreQueryResult rescase = DB_STORE(case_q.str().c_str());
    int case_id = 0;
    if(rescase.num_rows())
        case_id = (int)rescase[0][0];
//  $case_id=$sql->getOne("SELECT case_id FROM system_log_case WHERE ip='IP' AND case_status='open' AND case_type='TYPE'");
//  // Proverka za problem
//     -- IMA PROBLEM
    if(!ok)
    {
//     {
//          -- proverka ako case_id >0
        if(case_id == 0)
        {
            std::ostringstream case_open_q;
            case_open_q << "INSERT INTO system_log_case SET case_type='" << type <<"', case_status='open', case_start='" << get_date_time <<"', ip='"<<host<<"', storage_id="<<bmsID;
            DB_EXECUTE(case_open_q.str().c_str());
            mysqlpp::StoreQueryResult insert_id = DB_STORE("SELECT LAST_INSERT_ID()");
            case_id = (int) insert_id[0][0];
//              -- ne
//              {
//                                  INSERT INTO system_log_case
//                                          SET
//                                              case_type='TYPE',
//                                              case_status='open',
//                                              case_start=NOW(),
//                                              ip='IP'
//                        $case_id=$sql->getOne("SELECT LAST_INSERT_ID()");
//              }
        }
        std::ostringstream systemlog_q;
        systemlog_q << "INSERT INTO system_log SET id = NULL, case_id= '" << case_id << "', log_time= '"<< get_date_time <<"', log_action='notify', log_message='"<< report <<"'";
        DB_EXECUTE(systemlog_q.str().c_str());
    } else {
//       }else{-- NIAMA PROBLEM
//              -- proverka ako ima otvoren da se zatvori problema
//              if (intval($case_id)>0){
        if(case_id > 0)
        {
            std::ostringstream close_q;
            close_q << "UPDATE system_log_case SET case_status='close', sync='no', case_end='" << get_date_time << "' WHERE case_id='" << case_id <<"'";
            DB_EXECUTE(close_q.str().c_str());
        }
//              $sql->query("UPDATE system_log_case SET case_status='close', case_end=NOW() WHERE case_id='".intval($case_id)."'");
//          }
    }
}

inline int get_room_id(std::string& RoomName)
{
    // std::ostringstream room_name_q;

    // normalize_string(RoomName);
    // room_name_q << "SELECT RoomID FROM Room WHERE RoomName='" << RoomName<<"'";
    // mysqlpp::StoreQueryResult room_name_res = DB_STORE(room_name_q.str().c_str());

    // int RoomNo = 0;

    // if(room_name_res.num_rows() > 0)
    // {
    //     RoomNo = (int)room_name_res[0][0];
    // }

    // return RoomNo;
    return ctxt.rooms_id[RoomName];
}

void link_start(std::string&)
{
    ctxt.pmsanswered = 1;
    {
        std::ostringstream la;
        la << "LD|DA"  << get_date << "|TI" << get_time << "|V#2.05|IFVI|";
        send_to_pms(la);
        sleep(1);
    }

    send_to_pms("LR|RIDR|FLDATI|");
    sleep(1);

    send_to_pms("LR|RIDS|FLDATI|");
    sleep(1);

    send_to_pms("LR|RIDE|FLDATI|");
    sleep(1);

    if(ctxt.conf->using_cards)
    {
        send_to_pms("LR|RIKR|FLWSKCRNKTK#G#GNGDDT$2|");
        sleep(1);

        send_to_pms("LR|RIKD|FLWSKCRNG#|");
        sleep(1);

        send_to_pms("LR|RIKA|FLASKCWS|");
        sleep(1);
    }
    if(!ctxt.conf->second_interface)
    {
        send_to_pms("LR|RIXL|FLRNG#MIMTDATI|");
        sleep(1);

        send_to_pms("LR|RIXD|FLRNG#MI|");
        sleep(1);

        send_to_pms("LR|RIXR|FLRNG#|");
        sleep(1);

        send_to_pms("LR|RIXI|FLRNG#DCBIBDF#FDDATI|");
        sleep(1);

        send_to_pms("LR|RIXB|FLRNG#BADATI|");
        sleep(1);

        send_to_pms("LR|RIXC|FLRNG#ASBADATI|");
        sleep(1);

        send_to_pms("LR|RIWR|FLRNDATI|");
        sleep(1);

        send_to_pms("LR|RIWC|FLRNDATI|");
        sleep(1);

        send_to_pms("LR|RIWA|FLASRNDATI|");
        sleep(1);
    }

    send_to_pms("LR|RIGI|FLRNG#GNGLA0GSTVGGSF|");
    sleep(1);

    send_to_pms("LR|RIGO|FLRNG#GSSF|");
    sleep(1);

    send_to_pms("LR|RIGC|FLRNG#GNGLA0TVGS|");//send_to_pms("LR|RIGC|FLRNG#GNGLROGS|");
    sleep(1);

    if(!ctxt.conf->second_interface)
    {
        if(ctxt.conf->using_micros){
            logger(INFO, "Using MICROS value %d\n", ctxt.conf->using_micros);

            if(ctxt.conf->using_micros == 2){
                send_to_pms("LR|RIPS|FLRNPTTASCSODATI|");
            } else {
                send_to_pms("LR|RIPS|FLRNPTTASOS1S2S3S4S5S6S7S8S9DATI|");
            }
        } else {
            send_to_pms("LR|RIPS|FLRNPTTAMAM#DATI|");
        }
        /*
                send_to_pms("LR|RIPS|FLRNPTTAMAM#DATI|");
                sleep(1);
        */
        send_to_pms("LR|RIPA|FLRNDATIAS|");
        sleep(1);

        //send_to_pms("LR|RIRE|FLRNRS|");
        send_to_pms("LR|RIRE|FLRNRSPPCT|");
        sleep(1);
    }

    {
        std::ostringstream la;
        la << "LA|DA"  << get_date << "|TI" << get_time << "|";
        send_to_pms(la);
    }
}

void link_start_2(std::string&)
{
    ctxt.pmsanswered2 = 1;
    {
        std::ostringstream la;
        la << "LD|DA"  << get_date << "|TI" << get_time << "|V#2.05|IFVI|";
        send_to_pms(la,ctxt.pmssock2);
        sleep(1);
    }

    send_to_pms("LR|RIXL|FLRNG#MIMTDATI|",ctxt.pmssock2);
    sleep(1);

    send_to_pms("LR|RIXD|FLRNG#MI|",ctxt.pmssock2);
    sleep(1);

    send_to_pms("LR|RIXR|FLRNG#|",ctxt.pmssock2);
    sleep(1);

    send_to_pms("LR|RIXI|FLRNG#DCBIBDF#FDDATI|",ctxt.pmssock2);
    sleep(1);

    send_to_pms("LR|RIXB|FLRNG#BADATI|",ctxt.pmssock2);
    sleep(1);
    if(ctxt.conf->using_micros){
        logger(INFO, "Using MICROS value %d\n", ctxt.conf->using_micros);

        if(ctxt.conf->using_micros == 2){
            send_to_pms("LR|RIPS|FLRNPTTASCSODATI|");
        } else {
            send_to_pms("LR|RIPS|FLRNPTTASOS1S2S3S4S5S6S7S8S9DATI|",ctxt.pmssock2);
        }
    } else {
        send_to_pms("LR|RIPS|FLRNPTTAMAM#DATI|",ctxt.pmssock2);
    }
    sleep(1);

    send_to_pms("LR|RIPA|FLRNDATIAS|",ctxt.pmssock2);
    sleep(1);

    send_to_pms("LR|RIRE|FLRNRS|",ctxt.pmssock2);

    sleep(1);

    {
        std::ostringstream la;
        la << "LA|DA"  << get_date << "|TI" << get_time << "|";
        send_to_pms(la,ctxt.pmssock2);
    }
}

void link_alive(std::string&)
{
    std::ostringstream date_time;

    date_time << "DA"<< get_date << "|TI"<< get_time << "|";

    if(ctxt.pmsanswered == 0)
    {
        std::ostringstream la;
        la << "LA|" << date_time.str();
        send_to_pms(la);

        std::ostringstream dr;
        dr << "DR|" << date_time.str();
        send_to_pms(dr);

        ctxt.pmsanswered = 2;

    } else if(ctxt.pmsanswered == 1)
    {
        std::ostringstream dr;
        dr << "DR|" << date_time.str();
        send_to_pms(dr);

        ctxt.pmsanswered = 2;

    }
    //else {
    //}
}

void link_alive_2(std::string&)
{
    std::ostringstream date_time;

    date_time << "DA"<< get_date << "|TI"<< get_time << "|";

    if(ctxt.pmsanswered2 == 0)
    {
        std::ostringstream la;
        la << "LA|" << date_time.str();
        send_to_pms(la,ctxt.pmssock2);

        ctxt.pmsanswered2 = 2;

    } else if(ctxt.pmsanswered2 == 1)
    {
        ctxt.pmsanswered2 = 2;

    }
    //else {
    //}
}

void key_request(std::string& req)
{
    std::string WS = "1";
    int KC = 1;
    int RNo = 0;
    std::string RoomName;
    std::string KeyType;
    std::string KeyNum ="1";
    int GuestNo = 0;
    std::string GuestName;
    std::string CardData;
    int ReqID = 0;

    std::string DateTo;
    std::string TimeTo;

    DECODE_BEGIN(req)
        EXTRACT("WS([\\x20-\\x7B\\x7D-\\xFF]+)", WS)
        EXTRACT("KC(\\d+)",                 KC)
        EXTRACT("RN([\\x20-\\x7B\\x7D-\\xFF]+)",    RoomName)
        EXTRACT("KT(\\w+)",                 KeyType)
        EXTRACT("K\\#(\\w+)",               KeyNum)
        EXTRACT("G\\#(\\d+)",               GuestNo)
        EXTRACT("GN([\\x20-\\x7B\\x7D-\\xFF]+)",    GuestName)
        EXTRACT("\\$2(\\w+)",               CardData)
        EXTRACT("GD(\\w+)",                 DateTo)
        EXTRACT("DT([\\w\\:]+)",            TimeTo)
    DECODE_END()

    BEGIN_DB(ctxt.db_pool[0])
        RNo = get_room_id(RoomName);

        std::ostringstream pms_answer;
        pms_answer.fill('0');
        pms_answer << "KA|ASOK|KC"<< std::setw(2)<< KC << "|WS" << WS << "|";
        send_to_pms(pms_answer);

        if(RNo == 0)
        {
            END_DB_RETURN(ctxt.db_pool[0]);
            return;
        }

        normalize_string(req);

        std::ostringstream keycard_req_q;
        keycard_req_q << "INSERT INTO KeycardReq (ReqType, KCID, ReqText, ReqDate) VALUES (2, "<<KC<<", '"<< req <<"', '"<<get_date_time<<"')";
        DB_EXECUTE(keycard_req_q.str().c_str());

        mysqlpp::StoreQueryResult req_id_max = DB_STORE("SELECT MAX(ReqID) FROM KeycardReq WHERE ReqType=2;");

        if(req_id_max.num_rows() > 0)
        {
            ReqID = req_id_max[0][0];
        }

        std::ostringstream sData;
        sData << "KR|RN" << RNo <<"|K#"<< KeyNum << "|G#" << GuestNo <<"|$2"<<CardData<<"|HN"<<GuestName<<"|HT1|R#"<<ReqID;

        if(ctxt.conf->using_enddate)
        {
            if(DateTo != "")
                sData << "|GD"<<DateTo<<"|DT"<<TimeTo;
        }

        bool AS = send_to_mng(sData, KC);

        if(!AS)
        {
            ctxt.add_to_pmsbuffer(sData);
        }

    END_DB(ctxt.db_pool[0])
}

void key_delete(std::string& req)
{
    std::string WS;
    int KC = 1;
    int RNo = 0;
    std::string RoomName;
    int GuestNo = 0;
    int ReqID = 0;

    DECODE_BEGIN(req)
        EXTRACT("WS([\\x20-\\x7B\\x7D-\\xFF]+)", WS)
        EXTRACT("KC(\\d+)",                 KC)
        EXTRACT("RN([\\x20-\\x7B\\x7D-\\xFF]+)",    RoomName)
        EXTRACT("G\\#(\\d+)",               GuestNo)
    DECODE_END()

    BEGIN_DB(ctxt.db_pool[0])
        RNo = get_room_id(RoomName);

        std::ostringstream pms_answer;
        pms_answer << "KA|ASOK|KC"<< KC << "|WS" << WS << "|";
        send_to_pms(pms_answer);

        if(KC == 0)
        {
            END_DB_RETURN(ctxt.db_pool[0])
            return;
        }
        normalize_string(req);

        std::ostringstream keycard_req_q;
        keycard_req_q << "INSERT INTO KeycardReq (ReqType, KCID, ReqText, ReqDate) VALUES (2, "<<KC<<", '"<< req <<"', '"<<get_date_time<<"')";
        DB_EXECUTE(keycard_req_q.str().c_str());

        mysqlpp::StoreQueryResult req_id_max = DB_STORE("SELECT MAX(ReqID) FROM KeycardReq WHERE ReqType=2;");

        if(req_id_max.num_rows() > 0)
        {
            ReqID = req_id_max[0][0];
        }
        else
        {
            END_DB_RETURN(ctxt.db_pool[0])
            return;
        }



        std::ostringstream key_data_q;
        key_data_q << "SELECT hex(KeyID) As KeyN FROM Keycard Where KHRoomID="<<RNo<<" AND GuestNo=" << GuestNo;
        mysqlpp::StoreQueryResult key_data_req = DB_STORE(key_data_q.str().c_str());

        if(key_data_req.num_rows() > 0)
        {
            std::vector<int> RCs;
            for(uint16 i = 0; i < key_data_req.num_rows(); i ++)
            {
                std::ostringstream room_q;
                room_q << "SELECT RoomID FROM KeycardRoom WHERE KeyID=0x" << key_data_req[i][0].c_str();
                mysqlpp::StoreQueryResult room_res = DB_STORE(room_q.str().c_str());

                if(room_res.num_rows() > 0)
                {
                    for(uint16 j = 0; j < room_res.num_rows(); j ++)
                    {
                        RCs.push_back((int)room_res[j][0]);
                    }
                }

                TRANSACTION_BEGIN(*conn);
                    std::ostringstream keykard_req_q;
                    keykard_req_q << "INSERT INTO KeycardReq (ReqType, KCID, ReqText, ReqDate, EmpID, ParentReqID) VALUES (1, " << KC << ", '"<<req<<", '"<< get_date_time <<"', 0, "<< ReqID <<")";
                    DB_EXECUTE(keycard_req_q.str().c_str());

                    mysqlpp::StoreQueryResult req_max_res = DB_STORE("SELECT MAX(ReqID) FROM KeycardReq WHERE ReqType=1;");

                    int ReqNo = 0;

                    if(req_max_res.num_rows() > 0)
                    {
                        ReqNo = (int)req_max_res[0][0];
                    }

                    {
                        std::ostringstream del_q;
                        del_q << "DELETE FROM Keycard WHERE KeyID=0x" << key_data_req[i][0].c_str();
                        DB_EXECUTE(del_q.str().c_str());
                    }

                    {
                        std::ostringstream del_q;
                        del_q << "DELETE FROM KeycardRoom WHERE KeyID=0x" << key_data_req[i][0].c_str();
                        DB_EXECUTE(del_q.str().c_str());
                    }

                    {
                        std::ostringstream del_q;
                        del_q << "UPDATE KeycardLog SET EndReqID=" << ReqNo <<", End='"<< get_date_time <<"' WHERE KeyID=0x" << key_data_req[i][0].c_str() <<" AND (EndReqID=0)";
                        logger(INFO, "Update KeycarLog pms dell key %s\n", del_q.str().c_str());
                        DB_EXECUTE(del_q.str().c_str());
                    }

                    {
                        std::ostringstream del_q;
                        del_q << "UPDATE KeycardRoomLog SET EndReqID="<<ReqNo<<", End='" << get_date_time << "' WHERE KeyID=0x" << key_data_req[i][0].c_str() <<" AND (EndReqID=0)";
                        DB_EXECUTE(del_q.str().c_str());
                    }

                TRANSACTION_END();

                if(bOK)
                {
                    std::ostringstream blnk;

                    blnk.fill('0');
                    //blnk << "BLNK:" << std::hex << std::uppercase << std::setw(16) << key_data_req[i][0].c_str() << "*";
                    blnk << std::hex << std::uppercase << std::setw(16) << key_data_req[i][0].c_str();

                    for(uint16 j = 0; j < RCs.size(); j++)
                    {
                        std::string ip = get_rc_ip(RCs[j]);

                        pthread_mutex_lock(&ctxt.rooms[ip]->lock);
                        ctxt.rooms[ip]->key_map_send[blnk.str()] = true;
                                    //send_to_rc(affected_rcs[i], message);
                        pthread_mutex_unlock(&ctxt.rooms[ip]->lock);
                        //send_to_rc(ip, blnk);
                    }
                }
            }
        }

    END_DB(ctxt.db_pool[0])
}

inline int get_guest_in_room(mysqlpp::Query& qr, int RoomNo)
{
    int GuestNo = 0;

    std::ostringstream query;
    query << "SELECT GuestNo FROM Room WHERE RoomID=" << RoomNo;
    mysqlpp::StoreQueryResult query_res = DB_STORE(query.str().c_str());

    if(query_res.num_rows() > 0)
        GuestNo = (int) query_res[0][0];

    return GuestNo;
}

inline int guest_to_int(mysqlpp::Query& qr, std::string& lang)
{
        logger(INFO, "Fidelio try to set LANG: %s\n", lang.c_str());

        if(lang == "EA")
                lang = "EN";
        else if(lang == "SP")
                lang = "ES";
	else if(lang == "ZH-T")
                lang = "CN";
        else if(lang == "GE")
                lang = "DE";

        int langID = 0;

        std::ostringstream query;
//        query << "SELECT LangID FROM Lang WHERE Suffix = '" << lang << "' AND IsActive = 'TRUE'";
        query << "SELECT LangID FROM Lang WHERE Suffix = '" << lang << "'";
        mysqlpp::StoreQueryResult query_res = DB_STORE(query.str().c_str());

        if(query_res.num_rows() > 0)
                langID = (int) query_res[0][0];

        logger(INFO, "Selected LangID is: %d\n", langID);

        return langID;
}
/*
inline int guest_to_int(std::string& lang)
{
    if(lang == "EA")
        return 0;
    else if(lang == "BG")
        return 1;
    else if(lang == "GE")
        return 2;

    return 0;
}
*/
static void init_synch_room_data(int RoomNo, int GuestNo, std::string GuestName, int GuestLang, std::string GuestRights, int groupCode, int alhTVRights)
{
    pthread_mutex_lock(&ctxt.synh_lock);
    bool bSendToRC=0;
    std::cout<< "RoomNo: " << RoomNo<<"   ctxt.SF_RoomNo: " << ctxt.SF_RoomNo << std::endl;
    if (RoomNo != ctxt.SF_RoomNo)
    {
        if (ctxt.SF_RoomNo != 0)
        {
            std::vector<int> TZones;
            std::vector<int> TargetT;
            std::vector<int> GuestT_EmptyRoom;
            std::vector<int> Mode;

            BEGIN_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[ctxt.SF_RoomNo]]->db_name))

                TRANSACTION_BEGIN(*conn)
			std::ostringstream setNames;
                        setNames << "SET NAMES " << ctxt.conf->nameEncoding;
                        DB_EXECUTE(setNames.str().c_str());



                    //std::cout<<"being transaction\n";
                    {
                        std::ostringstream del_q;
                        del_q << "DELETE FROM RoomGuest WHERE RoomID=" << ctxt.SF_RoomNo;
                        DB_EXECUTE(del_q.str().c_str());
                    }


        //          pthread_mutex_lock(&ctxt.synh_lock);
                    std::map<int, linked_ptr<synh_room_data_t> >::iterator i = ctxt.synh_data.begin();

                    for(; i != ctxt.synh_data.end(); i++)
                    {
                        std::ostringstream room_quest_q;
                        room_quest_q << "INSERT INTO RoomGuest (RoomID, GuestNo, GuestName) VALUES("<< ctxt.SF_RoomNo <<" ,"<< i->first <<",'"<<i->second->GuestName<<"')";
                        DB_EXECUTE(room_quest_q.str().c_str());
                    }
        //          pthread_mutex_unlock(&ctxt.synh_lock);

                    std::ostringstream guest_no_q;
                    guest_no_q << "SELECT GuestNo, GuestName FROM Room WHERE RoomID=" << ctxt.SF_RoomNo;
                    mysqlpp::StoreQueryResult guest_no_res = DB_STORE(guest_no_q.str().c_str());

		    logger(INFO, "Check-IN Query: [%d]", ctxt.SF_RoomNo);

                    int bAddTitular = 0;
                    if(guest_no_res.num_rows() > 0)
                    {
                        int GuestNo_T = (int)guest_no_res[0][0];

                        if(GuestNo_T != 0)
                        {
                            std::ostringstream guest_info_q;
                            guest_info_q << "SELECT GuestNo, GuestName FROM RoomGuest WHERE RoomID=" << ctxt.SF_RoomNo<<" AND GuestNo="<<GuestNo_T;
                            mysqlpp::StoreQueryResult guest_info_res = DB_STORE(guest_info_q.str().c_str());
                            //std::cout << " num rows " << guest_info_res.num_rows() << std::endl;
                            if(guest_info_res.num_rows() == 0)
                            {
                                bAddTitular = 2;
                            } else {

                            }
                        } else {
                            bAddTitular = 1;
                        }
                    }

                    if(bAddTitular)
                    {
                        std::ostringstream guest_no_titul_q;
                        guest_no_titul_q << "SELECT GuestNo FROM RoomGuest WHERE RoomID=" << ctxt.SF_RoomNo <<" ORDER BY GuestNo LIMIT 1";
                        mysqlpp::StoreQueryResult guest_no_titul_res = DB_STORE(guest_no_titul_q.str().c_str());

                        if(guest_no_titul_res.num_rows() > 0)
                        {
                            int GuestNo_New = (int) guest_no_titul_res[0][0];
                //          std::ostringstream update_room_q;
                //          pthread_mutex_lock(&ctxt.synh_lock);
                //          update_room_q << "UPDATE Room SET GuestNo=" << GuestNo_New <<", GuestName='"<< ctxt.synh_data[GuestNo_New]->GuestName <<"', LangID=" << ctxt.synh_data[GuestNo_New]->GuestLang << ",  LastChange='" << get_date_time << "', WakeUp='0000-00-00 00:00:01' WHERE RoomID=" << ctxt.SF_RoomNo;
                //          pthread_mutex_unlock(&ctxt.synh_lock);
                //          DB_EXECUTE(update_room_q.str().c_str());

                            {
                                std::ostringstream pin_set_q;
                                pin_set_q << "SELECT PIN FROM SystemInfo";
                                mysqlpp::StoreQueryResult pin_set_res = DB_STORE(pin_set_q.str().c_str());

                                if(pin_set_res[0][0] != "")
                                {
                                    //std::cout<<"try to set default pin\n";
                                    std::ostringstream update_q;
                                    update_q << "UPDATE Room SET GuestNo=" << GuestNo_New <<", GuestName='"<< ctxt.synh_data[GuestNo_New]->GuestName <<"', LangID=" << ctxt.synh_data[GuestNo_New]->GuestLang << ", PIN='"<< pin_set_res[0][0] <<"',  LastChange='" << get_date_time << "', WakeUp='0000-00-00 00:00:01' WHERE RoomID=" << ctxt.SF_RoomNo;
				    logger(INFO, "Check-IN Query: [%s]", update_q.str().c_str());
                                    DB_EXECUTE(update_q.str().c_str());
                                }
                                else
                                {
                                    //std::cout<<"no default pin\n";
                                    std::ostringstream update_q;
                                    update_q << "UPDATE Room SET GuestNo=" << GuestNo_New <<", GuestName='"<< ctxt.synh_data[GuestNo_New]->GuestName <<"', LangID=" << ctxt.synh_data[GuestNo_New]->GuestLang << ",  LastChange='" << get_date_time << "', WakeUp='0000-00-00 00:00:01', GuestRights = '" << GuestRights << "' WHERE RoomID=" << ctxt.SF_RoomNo;
                                    //std::cout<<"bla bla\n";
			            logger(INFO, "Check-IN Query: [%s]", update_q.str().c_str());
                                    DB_EXECUTE(update_q.str().c_str());
                                }
                                //todor start
	                    	int playList = 1;   // t_todo add default playlist config in base_server.cfg
        		        if(ctxt.conf->default_playlist)
                        		playList = ctxt.conf->default_playlist;

                    		std::ostringstream queryPl;

                    		queryPl << "SELECT PlayListID FROM TVRights WHERE Type = 'lang' AND Value = '" << GuestLang << "'";
                        	mysqlpp::StoreQueryResult query_resPl = DB_STORE(queryPl.str().c_str());

                    		if(query_resPl.num_rows() > 0)
                        	playList = (int) query_resPl[0][0];
                    		std::cout << "DEBUG Playlist: [" << playList << "]" << std::endl;

                   		 std::ostringstream update_qPl;
                    		update_qPl << "UPDATE RoomWS SET PlayList = '" << playList << "' WHERE RoomID = " << ctxt.SF_RoomNo;
                    		DB_EXECUTE(update_qPl.str().c_str());

                    		//std::cout << "DEBUG TVRights Config: [" << ctxt.conf->using_tvrights << "]" << std::endl;
                    		if(ctxt.conf->using_tvrights){
                        		std::ostringstream update_qGr;
                        		update_qGr << "UPDATE Room SET GuestRights = '"<< GuestRights <<"' WHERE RoomID = " << ctxt.SF_RoomNo;
                        		DB_EXECUTE(update_qGr.str().c_str());
                    		}
                    		logger(INFO, "Child Flag: %d\n", alhTVRights);
                   		std::string tvRights = "TU";
                    		if(alhTVRights)
                        		tvRights = "TX";

                    		std::ostringstream update_qGC;
                    		update_qGC << "UPDATE Room SET GroupCode = '"<< groupCode <<"', GuestRights = '"<< tvRights <<"' WHERE RoomID = " << ctxt.SF_RoomNo;
                                DB_EXECUTE(update_qGC.str().c_str());

                    		//end


                                if(ctxt.conf->using_parental)
                                {
                                    std::ostringstream ppin_set_q;
                                    ppin_set_q << "SELECT PPIN FROM SystemInfo";
                                    mysqlpp::StoreQueryResult ppin_set_res = DB_STORE(ppin_set_q.str().c_str());
                                    //std::cout<< "synh room " << ppin_set_res[0][0] << std::endl;
                                    //if(ppin_set_res[0][0] != "")
                                    {
                                        std::ostringstream update_q;
                                        update_q << "UPDATE Room SET Parental='FALSE', PPIN='"<< ppin_set_res[0][0] <<"' WHERE RoomID=" << ctxt.SF_RoomNo;
                                        DB_EXECUTE(update_q.str().c_str());
                                    }
                                }
                            }

                        }

                        //TODO add parental on synh
                        std::ostringstream room_zone_q;
                        room_zone_q << "UPDATE RoomTZone SET TargetT=DefaultGuestT WHERE RoomID=" << ctxt.SF_RoomNo;
                        DB_EXECUTE(room_zone_q.str().c_str());

                        std::ostringstream temp_q;
                        temp_q << "SELECT TZoneID, DefaultGuestT,DefaultGuestT_EmptyRoom,Mode FROM RoomTZone WHERE RoomID=" << ctxt.SF_RoomNo;
                        mysqlpp::StoreQueryResult temp_res = DB_STORE(temp_q.str().c_str());

                        for(uint16 i = 0; i < temp_res.num_rows(); i++)
                        {
                            TZones.push_back(temp_res[i][0]);
                            TargetT.push_back(temp_res[i][1]);
                            GuestT_EmptyRoom.push_back(temp_res[i][2]);
                            Mode.push_back(temp_res[i][3]);
                        }

                        {
                            std::ostringstream update_q;
                            update_q << "INSERT INTO RoomGuestLog (RoomID, CheckIn, GuestName) SELECT "<< ctxt.SF_RoomNo <<", '"<< get_date_time <<"', '"<< GuestName <<"'";
                            DB_EXECUTE(update_q.str().c_str());
                        }

                        bSendToRC = true;
                    }
                    //std::cout<<"end transaction\n";
                TRANSACTION_END()

                if(bOK)
                {
                    if(bSendToRC && ctxt.conf->using_rc)
                    {
                        int Season = get_season(qr);

                        for(uint16 i = 0; i < TZones.size(); i++)
                        {
                            std::ostringstream clmt;
                            clmt.fill('0');
                            clmt << "CLMT:" << std::setw(2) << TZones[i] - 1 << Season << std::setw(2) << TargetT[i] << std::setw(2) << GuestT_EmptyRoom[i] << Mode[i] << "*";

                            std::string ip = get_rc_ip(ctxt.SF_RoomNo);
                            send_to_rc(ip, clmt);
                        }
                    }
                }
                else
                {
                    std::ostringstream msg;
                    msg << "Неуспешна заявка за настаняване на гост в стая "<<ctxt.SF_RoomNo;

                    write_event(4, msg);
                    ctxt.SF_RoomNo = RoomNo;
                }

            END_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[ctxt.SF_RoomNo]]->db_name))
        }

    //  pthread_mutex_lock(&ctxt.synh_lock);
        ctxt.synh_data.clear();
    //  pthread_mutex_unlock(&ctxt.synh_lock);
    }

    normalize_string(GuestName);

    if(GuestNo == 0 || GuestName.empty())
    {
//      std::ostringstream msg;
//      msg << "Неуспешна заявка за напускане на гост в стая "<<RoomNo;
//
//      write_event(4, msg);
    }
    else
    {
    //  pthread_mutex_lock(&ctxt.synh_lock);
        ctxt.synh_data.insert(std::make_pair(GuestNo, new synh_room_data_t(GuestName, GuestLang)));
    //  pthread_mutex_unlock(&ctxt.synh_lock);
    }

    ctxt.SF_RoomNo = RoomNo;
    pthread_mutex_unlock(&ctxt.synh_lock);
}


void check_in(std::string& req)
{
    std::string RNo = "0";
    int GuestNo = 0;
    int alhTVRights = 0;
    int groupCode = 0;
    std::string GuestN = "";
    std::string GuestName = "";
    std::string GLang = "0";
    std::string tvRights = "TU";
    std::string GuestShare;
    std::vector<int> TZones;
    std::vector<int> TargetT;
    std::vector<int> GuestT_EmptyRoom;
    std::vector<int> Mode;

    int bSinhronInProcess = 0;

    std::string Pass = randomString(8);

    DECODE_BEGIN(req)
       // EXTRACT("RN(\\w+)",                 RNo)
        EXTRACT("RN([\\x20-\\x7B\\x7D-\\xFF]+)",	RNo)
       // EXTRACT("G\\#(\\d+)",               		GuestNo)
        EXTRACT("G\\#([\\x20-\\x7B\\x7D-\\xFF]+)",    	GuestN)
        EXTRACT("GN([\\x20-\\x7B\\x7D-\\xFF]+)",    	GuestName)
        //EXTRACT("GL(\\w+)",             GLang)
        EXTRACT("GL([\\x20-\\x7B\\x7D-\\xFF]+)",        GLang)
        EXTRACT("TV(\\w+)",             		tvRights)
        EXTRACT("A0(\\d+)",                             alhTVRights)
        EXTRACT("GG(\\d+)",                             groupCode)
        EXTRACT("GS(\\w+)",                 		GuestShare)

        if(pcrecpp::RE("SF").FullMatch(message_real))
        {
            bSinhronInProcess = 1;
        }
    DECODE_END()
	//GI|RN103|G#002555452760|GSN|GFVICTOR|GNMEDVEDEV|GLEA|GG|GTMR.|GV|NPN|GA170413|DA170601|TI165038|SF|

	GuestNo = atoi(GuestN.c_str());

         std::cout<<"DEBUG: "<< GuestNo <<", GuestName: "<< GuestName <<std::endl;

	if(ctxt.conf->using_eltur){
                vector<string> v = explode(",,,", GuestName);
                //for(int i=0; i<v.size(); i++)
                //      std::cout <<i << " ["<< v[i] <<"] " <<std::endl;
                vector<string> g = explode(":::", v[0]);
                //for(int j=0; j<g.size(); j++)
                //      std::cout <<j << " ["<< g[j] <<"] " <<std::endl;
                std::cout << "Guest Lang: " << GLang << std::endl;
                if(GLang == "BG")
                {
                        //GuestName = cp2utf(g[0]);
                        GuestName = g[0];
                } 
                else 
                {
                        GuestName = g[1];
                }
		
                GuestNo = atoi( g[3].c_str() );
        }


        if (GuestShare == "Y")
        {
	    logger(INFO, "Ingore GI packet! Guest share flag is set (|GSY|)");
            return;
        }



        int RoomNo = get_room_id(RNo);
        std::cout<<"RoomNO="<<RNo<<std::endl;
        std::cout<<"RoomID="<<RoomNo<<std::endl;
        if(RoomNo == 0)
        {
            return;
        }

    BEGIN_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))

        int GuestLang = guest_to_int(qr, GLang);
        std::cout<<"GuestLang="<<GuestLang<<std::endl;
        normalize_string(GuestName);

        std::string ip = get_rc_ip(RoomNo);
        std::map<string, linked_ptr<room_status_t> >::iterator i = ctxt.rooms.find(ip);
        if(i == ctxt.rooms.end())
        {
            END_DB_RETURN(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
            return;
        }

        if(GuestNo == 0 || GuestName.empty())
        {
            std::ostringstream msg;
            msg << "Неуспешна заявка за настаняване на гост в стая "<<RoomNo;
            std::cout<<"Error: GuestNo or GuestName is Empty: "<< GuestNo <<", "<< GuestName <<", "<<msg.str()<<std::endl;

            write_event(4, msg);

            END_DB_RETURN(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
            return;
        }

        if(bSinhronInProcess)
        {
            //std::cout<<"init synch data\n";
            init_synch_room_data(RoomNo, GuestNo, GuestName, GuestLang, tvRights, groupCode, alhTVRights);
        }
        else
        {
            logger(INFO, "Child Flag: %d\n", alhTVRights);

            if(alhTVRights){
                tvRights = "TX";
            } else {
                tvRights = "TU";
            }
            //std::cout<<"guest in no synch\n";
            bool bFirst = true;
            if(get_guest_in_room(qr, RoomNo) != 0) bFirst = false;

            TRANSACTION_BEGIN(*conn)

		std::ostringstream setNames;
                setNames << "SET NAMES " << ctxt.conf->nameEncoding;
                DB_EXECUTE(setNames.str().c_str());



                std::ostringstream room_guest_q;
                room_guest_q << "INSERT INTO RoomGuest (RoomID, GuestNo, GuestName) VALUES(" << RoomNo <<"," << GuestNo << ",'" << GuestName << "')";
                DB_EXECUTE(room_guest_q.str().c_str());

                if(bFirst)
                {
                    //start
                    int playList = 1;   // t_todo add default playlist config in base_server.cfg
                    if(ctxt.conf->default_playlist)
                        playList = ctxt.conf->default_playlist;

                    std::ostringstream queryPl;
                        queryPl << "SELECT PlayListID FROM TVRights WHERE Type = 'lang' AND Value = '" << GuestLang << "'";
                        mysqlpp::StoreQueryResult query_resPl = DB_STORE(queryPl.str().c_str());
                    if(query_resPl.num_rows() > 0)
                        playList = (int) query_resPl[0][0];

                    std::ostringstream update_qPl;
                    update_qPl << "UPDATE RoomWS SET PlayList = '" << playList << "' WHERE RoomID = " << RoomNo;
                    DB_EXECUTE(update_qPl.str().c_str());
                    //end

                    std::ostringstream tzone_q;
                    tzone_q << "SELECT TZoneID, DefaultGuestT,DefaultGuestT_EmptyRoom,Mode FROM RoomTZone WHERE RoomID=" << RoomNo;
                    mysqlpp::StoreQueryResult tzone_res = DB_STORE(tzone_q.str().c_str());

                    {

                        std::ostringstream pin_set_q;
                        pin_set_q << "SELECT PIN FROM SystemInfo";
                        mysqlpp::StoreQueryResult pin_set_res = DB_STORE(pin_set_q.str().c_str());

                        logger(INFO, "Random Pass: %s\n", Pass.c_str());

                        if(pin_set_res[0][0] != "")
                        {
                            std::ostringstream update_q;
                            update_q << "UPDATE Room SET GuestNo=" << GuestNo <<", GuestName='" << GuestName << "', LangID=" << GuestLang << ",PIN='"<< pin_set_res[0][0] <<"', LastChange='" << get_date_time << "', WakeUp='0000-00-00 00:00:01', Password='"<< Pass <<"', GroupCode = '"<< groupCode <<"' WHERE RoomID=" << RoomNo;
                            DB_EXECUTE(update_q.str().c_str());
			    if(ctxt.conf->using_tvrights){
                         	update_q.str("");
 				update_q << "UPDATE Room SET GuestRights = '"<< tvRights <<"' WHERE RoomID = " << ctxt.SF_RoomNo;
                       		DB_EXECUTE(update_q.str().c_str());
                     	    }

                        }
                        else
                        {
                            //std::cout<<"no default pin\n";
                            std::ostringstream update_q;
                            update_q << "UPDATE Room SET GuestNo=" << GuestNo <<", GuestName='" << GuestName << "', LangID=" << GuestLang << ", LastChange='" << get_date_time << "', WakeUp='0000-00-00 00:00:01', Password='"<< Pass <<"', GroupCode = '" << groupCode  <<"' WHERE RoomID=" << RoomNo;
                            //std::cout<<"bla bla\n";
                            DB_EXECUTE(update_q.str().c_str());
			    if(ctxt.conf->using_tvrights){
                         	update_q.str("");
 				update_q << "UPDATE Room SET GuestRights = '"<< tvRights <<"' WHERE RoomID = " << ctxt.SF_RoomNo;
                       		DB_EXECUTE(update_q.str().c_str());
                     	    }

                        }

                        if(ctxt.conf->using_parental)
                        {
                            std::ostringstream ppin_set_q;
                            ppin_set_q << "SELECT PPIN FROM SystemInfo";
                            mysqlpp::StoreQueryResult ppin_set_res = DB_STORE(ppin_set_q.str().c_str());
                            //std::cout<< "check in" << ppin_set_res[0][0] << std::endl;
                            //if(ppin_set_res[0][0] != "")
                            {
                                std::ostringstream update_q;
                                update_q << "UPDATE Room SET Parental='FALSE', PPIN='"<< ppin_set_res[0][0] <<"' WHERE RoomID=" << RoomNo;
                                DB_EXECUTE(update_q.str().c_str());
                            }
                        }
                    }

                    {
                        std::ostringstream update_q;
                        update_q << "UPDATE RoomTZone SET TargetT=DefaultGuestT WHERE RoomID=" << RoomNo;
                        DB_EXECUTE(update_q.str().c_str());
                    }

                    {
                        std::ostringstream update_q;
                        update_q << "INSERT INTO RoomGuestLog (RoomID, CheckIn, GuestName) SELECT "<< RoomNo <<", '"<< get_date_time <<"', '"<< GuestName <<"'";
                        DB_EXECUTE(update_q.str().c_str());
                    }

                    for(uint16 i = 0; i < tzone_res.num_rows(); i++)
                    {
                        TZones.push_back(tzone_res[i][0]);
                        TargetT.push_back(tzone_res[i][1]);
                        GuestT_EmptyRoom.push_back(tzone_res[i][2]);
                        Mode.push_back(tzone_res[i][3]);
                    }

                    if(ctxt.conf->auto_inet)
                    {
                        //std::cout<<"gen inet\n";
                        gen_inet(1, RoomNo, 0, "", 0);
                    }
                }

            TRANSACTION_END()

            if(bOK)
            {
                if(ctxt.conf->using_rc)
                {
                int Season = get_season(qr);

                for(uint16 i = 0; i < TZones.size(); i++)
                {
                    std::ostringstream clmt;
                    clmt.fill('0');
                    clmt << "CLMT:" << std::setw(2) << TZones[i] - 1 << Season << std::setw(2) << TargetT[i] << std::setw(2) << GuestT_EmptyRoom[i] << Mode[i] << "*";

                    std::string ip = get_rc_ip(RoomNo);
                    send_to_rc(ip, clmt);
                }
                }
            }
            else
            {
                std::ostringstream msg;
                msg << "Неуспешна заявка за настаняване на гост в стая "<<RoomNo;

                write_event(4, msg);
            }
        }
    END_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
}

void bms_delete_keys(int RNo)
{
    std::ostringstream ReqText;
    ReqText << "KD|RN"<<RNo<<"|";

    int KCID = 1;
    std::string KeyCardID;
    int ReqID = 0;
    int EmpID = 0;
    int PReqNo = 0;

    std::vector<int> RCs;

    BEGIN_DB(ctxt.db_pool[0])
        {
            std::ostringstream update_q;
            update_q << "INSERT INTO KeycardReq (ReqType, KCID, ReqText, ReqDate) VALUES (2, "<<KCID<<", '"<<ReqText.str()<<"', '"<<get_date_time<<"')";
    //      PRQ(update_q);
            DB_EXECUTE(update_q.str().c_str());
        }

        mysqlpp::StoreQueryResult maxreq_res = DB_STORE("SELECT MAX(ReqID) FROM KeycardReq WHERE ReqType=2;");

        if(maxreq_res.num_rows() > 0)
        {
            ReqID = (int) maxreq_res[0][0];
        }
        else {
            END_DB_RETURN(ctxt.db_pool[0])
            return;
        }

        std::ostringstream keycard_q;
        keycard_q << "SELECT hex(KeyID) As KeyN FROM Keycard Where KHRoomID=" << RNo <<" AND KHTypeID IN(";
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

        logger(INFO, "DeleteKeys %s\n", keycard_q.str().c_str());
    //  PRQ(keycard_q);

        mysqlpp::StoreQueryResult keycard_res = DB_STORE(keycard_q.str().c_str());

        if(keycard_res.num_rows() > 0)
        {
            for(uint16 j = 0; j < keycard_res.num_rows(); j++)
            {
                KeyCardID = keycard_res[j][0].c_str();

                TRANSACTION_BEGIN(*conn)
                    std::ostringstream roomid_q;
                    roomid_q << "SELECT RoomID FROM KeycardRoom WHERE KeyID=0x"<<KeyCardID;
    //              PRQ(roomid_q);
                    mysqlpp::StoreQueryResult roomid_res = DB_STORE(roomid_q.str().c_str());

                    for(uint16 i = 0; i < roomid_res.num_rows(); i++)
                    {
                        RCs.push_back(roomid_res[i][0]);
                    }

                    {
                        std::ostringstream insert_q;
                        insert_q << "INSERT INTO KeycardReq (ReqType, KCID, ReqText, ReqDate, EmpID, ParentReqID) VALUES (1, "<<KCID<<", '"<<ReqText.str()<<"', '"<<get_date_time<<"', "<<EmpID<<", "<<PReqNo<<")";
                        DB_EXECUTE(insert_q.str().c_str());
                    }

                    mysqlpp::StoreQueryResult maxid_1_res = DB_STORE("SELECT MAX(ReqID) FROM KeycardReq WHERE ReqType=1");

                    if(maxid_1_res.num_rows() > 0)
                    {
                        int ReqNo = (int)maxid_1_res[0][0];

                        {
                            std::ostringstream insert_q;
                            insert_q << "DELETE FROM Keycard WHERE KeyID=0x"<<KeyCardID;
                            DB_EXECUTE(insert_q.str().c_str());
                        }

                        {
                            std::ostringstream insert_q;
                            insert_q << "DELETE FROM KeycardRoom WHERE KeyID=0x" << KeyCardID;
                            DB_EXECUTE(insert_q.str().c_str());
                        }

                        {
                            std::ostringstream insert_q;
                            insert_q << "UPDATE KeycardLog SET EndReqID="<<ReqNo<<", End='"<<get_date_time<<"' WHERE KeyID=0x"<<KeyCardID<<" AND (EndReqID=0)";
                            logger(INFO, "Update KeycarLog pms guest out %s\n", insert_q.str().c_str());
                            DB_EXECUTE(insert_q.str().c_str());
                        }

                        {
                            std::ostringstream insert_q;
                            insert_q << "UPDATE KeycardRoomLog SET EndReqID="<<ReqNo<<", End='"<<get_date_time<<"' WHERE KeyID=0x"<<KeyCardID<<" AND (EndReqID=0)";
                            DB_EXECUTE(insert_q.str().c_str());
                        }
                    }

                TRANSACTION_END()

                if(bOK)
                {
                    if(ctxt.conf->using_rc)
                    {
                        std::ostringstream blnk;
                        blnk.fill('0');
                        blnk<<"BLNK:"<<std::hex<<std::uppercase<<std::setw(16)<<KeyCardID<<"*";

                        for(uint16 i = 0; i < RCs.size(); i ++)
                        {
                            std::string ip = get_rc_ip(RCs[i]);

                            pthread_mutex_lock(&ctxt.rooms[ip]->lock);
                            ctxt.rooms[ip]->key_map_send[KeyCardID] = true;
                                                                //send_to_rc(affected_rcs[i], message);
                            pthread_mutex_unlock(&ctxt.rooms[ip]->lock);

                            //send_to_rc(ip, blnk);
                        }
                    }
                }
            }
        }
    END_DB(ctxt.db_pool[0])
}

void check_out(std::string& req)
{
    std::string RNo = "0";
    int GuestNo = 0;

    std::vector<int> TZones;
    std::vector<int> TargetT;
    std::vector<int> GuestT_EmptyRoom;
    std::vector<int> Mode;

    DECODE_BEGIN(req)
        //EXTRACT("RN(\\w+)",                 RNo)
        EXTRACT("RN([\\x20-\\x7B\\x7D-\\xFF]+)",                 RNo)
        EXTRACT("G\\#(\\d+)",               GuestNo)
    DECODE_END()

    bool bUpdateR = false;
    bool bBusyRoom = false;

    int GuestNo_New = 0;
    std::string GuestName_New;

        int RoomNo = get_room_id(RNo);
        //if(RoomNo == 0 || GuestNo == 0)
        if(RoomNo == 0)
        {
            logger( ERROR, "Room Number is 0 Check-Out FAILED!!! ");
            std::ostringstream msg;
            msg << "Неуспешна заявка за напускане на гост в стая "<< RNo;
            write_event(4, msg);
            return;
        }

    BEGIN_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
        TRANSACTION_BEGIN(*conn)


	    {
                std::ostringstream delete_q;
                delete_q << "DELETE FROM RoomGuest WHERE RoomID=" << RoomNo <<" AND GuestNo=" << GuestNo;
//              PRQ(delete_q);
                DB_EXECUTE(delete_q.str().c_str());
            }
            if(get_guest_in_room(qr, RoomNo) == GuestNo) bUpdateR = true;
            if(bUpdateR)
            {
                std::ostringstream guestno_q;
                guestno_q << "SELECT GuestNo, GuestName FROM RoomGuest WHERE RoomID=" << RoomNo << " ORDER BY GuestNo LIMIT 1";
    //          PRQ(guestno_q);
                mysqlpp::StoreQueryResult guestno_res = DB_STORE(guestno_q.str().c_str());
                if(guestno_res.num_rows() > 0)
                {
                    GuestNo_New = (int) guestno_res[0][0];
                    GuestName_New = guestno_res[0][1].c_str();

                    bBusyRoom = true;
                }

                if(bBusyRoom)
                {
                    std::ostringstream update_q;
                    update_q << "UPDATE Room SET GuestNo=" << GuestNo_New <<", GuestName='"<<GuestName_New<<"',  LastChange='"<<get_date_time<<"' WHERE RoomID="<<RoomNo<<" AND GuestNo="<<GuestNo;
                    DB_EXECUTE(update_q.str().c_str());
                }
                else
                {
                    std::ostringstream roomtzone_q;
                    roomtzone_q << "SELECT TZoneID, DefaultFreeT, Mode FROM RoomTZone WHERE RoomID=" << RoomNo;
                    mysqlpp::StoreQueryResult roomtzone_res = DB_STORE(roomtzone_q.str().c_str());

                    if(ctxt.conf->using_parental)
                    {
                        std::ostringstream ppin_set_q;
                        ppin_set_q << "SELECT PPIN FROM SystemInfo";
                        mysqlpp::StoreQueryResult ppin_set_res = DB_STORE(ppin_set_q.str().c_str());
                        //std::cout<< " check out"<< ppin_set_res[0][0] << std::endl;
                        //if(ppin_set_res[0][0] != "")
                        {
                            std::ostringstream update_q;
                            update_q << "UPDATE Room SET Parental='FALSE', PPIN='"<< ppin_set_res[0][0] <<"', GuestNo=0, GuestName='', LangID=0, GroupCode = '0', LastChange='"<<get_date_time<<"', PIN='"<< RoomNo <<"', WakeUp='0000-00-00 00:00:00', GuestRights = 'TU', WHERE RoomID="<<RoomNo<<" AND GuestNo="<<GuestNo;
                            DB_EXECUTE(update_q.str().c_str());
                        }
                    }
                    else
                    {
                        std::ostringstream update_q;
                        update_q << "UPDATE Room SET GuestNo=0, GuestName='', LangID=0, GroupCode = '0', LastChange='"<<get_date_time<<"', PIN='"<< RoomNo <<"', WakeUp='0000-00-00 00:00:00', GuestRights = 'TU' WHERE RoomID="<<RoomNo<<" AND GuestNo="<<GuestNo;
                        DB_EXECUTE(update_q.str().c_str());
                    }

                    {

                        std::ostringstream update_q;
                        update_q << "UPDATE RoomTZone SET TargetT=DefaultFreeT WHERE RoomID="<<RoomNo;
                        DB_EXECUTE(update_q.str().c_str());
                    }

                    {

                        std::ostringstream update_q;
                        update_q << "UPDATE RoomStatus SET GreenIndic=0, RedIndic=0 WHERE RoomID=" << RoomNo;
                        DB_EXECUTE(update_q.str().c_str());
                    }

                    {

                        std::ostringstream update_q;
                        //update_q << "DELETE FROM Message WHERE RoomID=" << RoomNo;
                        update_q << "UPDATE Message SET IsActive = 'FALSE' WHERE RoomID=" << RoomNo;
                        DB_EXECUTE(update_q.str().c_str());
                    }

                    {

                        std::ostringstream update_q;
                        update_q << "UPDATE MoviePay SET IsActive='FALSE' WHERE RoomID="<<RoomNo;
                        DB_EXECUTE(update_q.str().c_str());
                    }

                    /*Todor Remove internetPay
                    {

                        std::ostringstream update_q;
                        update_q << "UPDATE InternetPay SET IsActive='FALSE' WHERE RoomID=" << RoomNo;
                        DB_EXECUTE(update_q.str().c_str());
                    }
                    */
                    {

                        std::ostringstream update_q;
                        update_q << "UPDATE RoomGuestLog SET CheckOut = '"<<get_date_time<<"' WHERE RoomID = "<< RoomNo <<" AND CheckOut = '0000-00-00'";
                        DB_EXECUTE(update_q.str().c_str());
                    }

                    for(uint16 i = 0; i < roomtzone_res.num_rows() ; i ++)
                    {
                        TZones.push_back((int) roomtzone_res[i][0]);
                        TargetT.push_back((int) roomtzone_res[i][1]);
                        Mode.push_back((int) roomtzone_res[i][2]);
                    }
                }
            }
	    if (ctxt.conf->using_eltur){
                logger( INFO, "Using Eltour check-out way!!! ");
                std::ostringstream guestOutQ;
		guestOutQ << "DELETE FROM RoomGuest WHERE RoomID=" << RoomNo;
                DB_EXECUTE(guestOutQ.str().c_str());
		guestOutQ.str("");
                guestOutQ << "UPDATE Room SET GuestNo='', GuestName='', LangID = 0, Password = 0, PIN='" << RoomNo << "', LastChange='"<<get_date_time<<"' WHERE RoomID="<<RoomNo;
                DB_EXECUTE(guestOutQ.str().c_str());
		guestOutQ.str("");
                guestOutQ << "UPDATE RoomWS SET PlayList='"<< ctxt.conf->default_playlist << "' WHERE RoomID="<<RoomNo;
                DB_EXECUTE(guestOutQ.str().c_str());
            }


        TRANSACTION_END()

        if(bOK)
        {
            if(ctxt.conf->using_eraseonlast)
            {
                bms_delete_keys(RoomNo);
            }
        }
        else
        {
            std::ostringstream msg;
            msg << "Неуспешна заявка за напускане на гост в стая "<<RoomNo;

            write_event(4, msg);
        }

        if(!bBusyRoom && ctxt.conf->using_rc)
        {
            int Season = get_season(qr);

            for(uint16 i = 0; i < TZones.size(); i++)
            {
                std::ostringstream clmt;
                clmt.fill('0');
                clmt << "CLMT:" << std::setw(2) << TZones[i] - 1 << Season << std::setw(2) << TargetT[i] << std::setw(2) << TargetT[i] << Mode[i] << "*";
                std::string ip = get_rc_ip(RoomNo);
                send_to_rc(ip, clmt);
            }
        }

    END_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
}

void guest_change(std::string& req)
{
    std::string RNo;
    int GuestNo = 0;
        int alhTVRights = 0;
        int groupCode = 0;
    std::string GuestRights;
    std::string GuestName;
    std::string GLang = "-1";
    int OldRoomNo = 0;
    int GuestShare = 0;
    std::vector<int> TZones;
    std::vector<int> TargetT;

    DECODE_BEGIN(req)
        EXTRACT("RN([\\x20-\\x7B\\x7D-\\xFF]+)",                 RNo)
        EXTRACT("G\\#(\\d+)",               GuestNo)
        EXTRACT("GN([\\x20-\\x7B\\x7D-\\xFF]+)",    GuestName)
        EXTRACT("GL(\\w+)",                 GLang)
        EXTRACT("RO(\\d+)",                 OldRoomNo)
        EXTRACT("GS(\\d+)",                 GuestShare)
        EXTRACT("TV(\\w+)",                 GuestRights)
        EXTRACT("A0(\\d+)",                 alhTVRights)
        EXTRACT("GG(\\d+)",                 groupCode)
    DECODE_END()

        int RoomNo = get_room_id(RNo);
        std::cout<<"RoomNo="<<RoomNo<<std::endl;
        if(RoomNo == 0)
        {
            return;
        }

    BEGIN_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
        int GuestLang = guest_to_int(qr, GLang);

        if(GuestNo != 0)
        {
            if(alhTVRights){
                    GuestRights = "TX";
            } else {
                    GuestRights = "TU";
            }
            {
                std::ostringstream update_q;
                if(ctxt.conf->using_tvrights){
                    update_q << "UPDATE Room SET GuestNo=" << GuestNo << ", GuestRights = '"<< GuestRights  <<"', GroupCode='"<< groupCode <<"' WHERE RoomID=" << RoomNo;
                } else {
                    update_q << "UPDATE Room SET GuestNo="<<GuestNo<<", GroupCode='"<< groupCode <<"' WHERE RoomID=" << RoomNo;
                }
                DB_EXECUTE(update_q.str().c_str());
            }
            {
                std::ostringstream update_q;
                update_q << "UPDATE RoomTZone SET TargetT=DefaultGuestT WHERE RoomID=" << RoomNo;
                DB_EXECUTE(update_q.str().c_str());
            }
        }

        normalize_string(GuestName);

        if(!GuestName.empty())
        {
            std::ostringstream update_q;
            if(ctxt.conf->using_tvrights){
                update_q << "UPDATE Room SET GuestName='"<<GuestName<<"', GuestRights = '"<< GuestRights  <<"', GroupCode='"<< groupCode <<"' WHERE RoomID=" << RoomNo;
            } else {
                update_q << "UPDATE Room SET GuestName='"<<GuestName<<"', GroupCode='"<< groupCode <<"' WHERE RoomID=" << RoomNo;
            }
            DB_EXECUTE(update_q.str().c_str());
        }

        if(GuestLang != -1)
        {
            std::ostringstream update_q;
            if(ctxt.conf->using_tvrights){
                update_q << "UPDATE Room SET LangID="<<GuestLang<<", GuestRights = '"<< GuestRights  <<"', GroupCode='"<< groupCode <<"' WHERE RoomID=" << RoomNo;
            } else {
                update_q << "UPDATE Room SET LangID="<<GuestLang<<", GroupCode='"<< groupCode <<"'  WHERE RoomID=" << RoomNo;
            }
            DB_EXECUTE(update_q.str().c_str());
        }

    END_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
}

void bill_info(std::string& req)
{
    std::string of = req;
    std::string RNo;
//  std::cout << "MSG:" << of << std::endl;

    int GuestNo = 0;
    int screen = 0;
    int DepCode = 0;
    //int Amount = 0;
    std::string Amount;
    std::string Date;
    std::string Time;
    std::string Description;
    std::string Pass = "Y";

    DECODE_BEGIN(req)
        EXTRACT("RN([\\x20-\\x7B\\x7D-\\xFF]+)",                 RNo)
        EXTRACT("G\\#(\\d+)",               GuestNo)
        EXTRACT("DC(\\d+)",                 DepCode)
        //EXTRACT("BI(\\w+)",               Amount)
        EXTRACT("F\\#(\\d+)",               screen)
        EXTRACT("DA(\\w+)",                 Date)
        EXTRACT("TI(\\w+)",                 Time)
        EXTRACT("FD(\\w+)",                 Pass)
        EXTRACT("(BD[\\x20-\\x7B\\x7D-\\xFF]+)",    Description)
        EXTRACT("BI([\\x20-\\x7B\\x7D-\\xFF]+)",    Amount)
    DECODE_END()

    // BEGIN_DB(ctxt.db_pool[0])
        int RoomNo = get_room_id(RNo);
        if(RoomNo == 0)
            return;

        if(Pass == "Y")
        {
            pthread_mutex_lock(&ctxt.php_lock);
            std::map<int, linked_ptr<php_status_t> >::iterator i = ctxt.php.find(RoomNo);
            if(i != ctxt.php.end())
            {
                std::cout << "Description: " << Description << std::endl;
                std::ostringstream msg;
                //std::cout << "BI:[" << Amount << "]" << std::endl;
                msg << "XI|RN"<<RoomNo<<"|DC"<<DepCode<<"|BI"<<Amount<<"|"<<Description<<"|F#"<< screen <<"|DA"<<Date<<"|TI"<<Time<<"|\r\n";
                //std::cout << "MSG send to PHP:[" << msg.str().c_str() << "]" << std::endl;
                send(i->second->handle, msg.str().c_str(), msg.str().length(), 0);
            }
            pthread_mutex_unlock(&ctxt.php_lock);
        }
    // END_DB(ctxt.db_pool[0])
}

void fastChoutAns(std::string& req)
{
    std::string RNo;
    std::string fastCOAns;
    int GuestNo = 0;
    int Amount = 0;
    std::string Date;
    std::string Time;

    DECODE_BEGIN(req)
        EXTRACT("RN([\\x20-\\x7B\\x7D-\\xFF]+)",                 RNo)
        EXTRACT("G\\#(\\d+)",               GuestNo)
        EXTRACT("BA(\\d+)",                 Amount)
        EXTRACT("AS([\\w]+)",               fastCOAns)
        EXTRACT("DA([\\w]+)",   Date)
        EXTRACT("TI([\\w]+)",   Time)
    DECODE_END()

    // BEGIN_DB(ctxt.db_pool[0])
        int RoomNo = get_room_id(RNo);
        if(RoomNo == 0)
            return;

        pthread_mutex_lock(&ctxt.php_lock);
        std::map<int, linked_ptr<php_status_t> >::iterator i = ctxt.php.find(RoomNo);
        if(i != ctxt.php.end())
        {
            std::ostringstream msg;
            msg << "XC|RN"<<RoomNo<<"|BA"<<Amount<<"|AS"<<fastCOAns<<"|DA"<<Date<<"|TI"<<Time<<"|\r\n";
            send(i->second->handle, msg.str().c_str(), msg.str().length(), 0);
        }

        pthread_mutex_unlock(&ctxt.php_lock);

        ctxt.delete_php(RoomNo);

    // END_DB(ctxt.db_pool[0])
}
void bill_total(std::string& req)
{
    std::string RNo;
    int GuestNo = 0;
    int Amount = 0;
    std::string Date;
    std::string Time;

    DECODE_BEGIN(req)
        EXTRACT("RN([\\x20-\\x7B\\x7D-\\xFF]+)",                 RNo)
        EXTRACT("G\\#(\\d+)",               GuestNo)
        EXTRACT("BA(\\d+)",                 Amount)
        EXTRACT("DA([\\w]+)",   Date)
        EXTRACT("TI([\\w]+)",   Time)
    DECODE_END()

    // BEGIN_DB(ctxt.db_pool[0])
        int RoomNo = get_room_id(RNo);
        if(RoomNo == 0)
            return;

        pthread_mutex_lock(&ctxt.php_lock);
        std::map<int, linked_ptr<php_status_t> >::iterator i = ctxt.php.find(RoomNo);
        if(i != ctxt.php.end())
        {
            std::ostringstream msg;
            msg << "XB|RN"<<RoomNo<<"|BA"<<Amount<<"|DA"<<Date<<"|TI"<<Time<<"|\r\n";
            send(i->second->handle, msg.str().c_str(), msg.str().length(), 0);
        }

        pthread_mutex_unlock(&ctxt.php_lock);

        ctxt.delete_php(RoomNo);

    // END_DB(ctxt.db_pool[0])
}

void posting_answer(std::string& req)
{
    std::string RNo;
    std::string Answer = "0";
    std::string Date;
    std::string Time;

    DECODE_BEGIN(req)
        EXTRACT("RN([\\x20-\\x7B\\x7D-\\xFF]+)",                 RNo)
        EXTRACT("AS([\\x20-\\x7B\\x7D-\\xFF]+)",    Answer)
        EXTRACT("DA([\\w]+)",   Date)
        EXTRACT("TI([\\w]+)",   Time)
    DECODE_END()

    // BEGIN_DB(ctxt.db_pool[0])
        int RoomNo = get_room_id(RNo);

        if(RoomNo == 0)
            return;

        pthread_mutex_lock(&ctxt.php_lock);
        std::map<int, linked_ptr<php_status_t> >::iterator i = ctxt.php.find(RoomNo);
        if(i != ctxt.php.end())
        {
            if(i->second->type == 1)
            {
                if(Answer == "OK")
                {
                    std::string admno = gen_inet(ctxt.php[RoomNo]->inettype, RoomNo, 0, "", 0);

                    if(admno != "")
                    {
                        std::ostringstream msg;
                        msg << "PA|RN"<<RoomNo<<"|NMroom"<<RoomNo<<"|PD"<<admno<<"|ASOK|\r\n";
                        send(ctxt.php[RoomNo]->handle, msg.str().c_str(), msg.str().length(), 0);
                    }
                    else {
                        std::ostringstream msg;
                        msg << "PA|RN"<<RoomNo<<"|NM|PD|ASNO|\r\n";
                        send(ctxt.php[RoomNo]->handle, msg.str().c_str(), msg.str().length(), 0);
                    }
                }
            }
            else if(i->second->type == 0)
            {
                std::ostringstream msg;
                msg << "PA|RN"<<RoomNo<<"|AS"<<Answer<<"|\r\n";
                send(i->second->handle, msg.str().c_str(), msg.str().length(), 0);
            }
        }

        pthread_mutex_unlock(&ctxt.php_lock);

        ctxt.delete_php(RoomNo);

    // END_DB(ctxt.db_pool[0])
}

void empty_room(std::string& req)
{
    std::string RNo = "0";

    std::vector<int> TZones;
    std::vector<int> TargetT;
    std::vector<int> GuestT_EmptyRoom;
    std::vector<int> Mode;

    DECODE_BEGIN(req)
        EXTRACT("RN([\\x20-\\x7B\\x7D-\\xFF]+)",                 RNo)
    DECODE_END()

        int RoomNo = get_room_id(RNo);

        if(RoomNo == 0)
        {
            //std::cout<<"room 0\n";
        //  std::ostringstream msg;
        //  msg << "Неуспешна заявка за напускане на гост в стая " << RoomNo;
        //  write_event(4, msg);
            // END_DB_RETURN(ctxt.db_pool[0])
            return;
        }

    BEGIN_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
        TRANSACTION_BEGIN(*conn)
            {
                std::ostringstream update_q;
                update_q << "DELETE FROM RoomGuest WHERE RoomID=" << RoomNo;
                DB_EXECUTE(update_q.str().c_str());
                //std::cout<<update_q.str()<<std::endl;
            }

            std::ostringstream roomtemp_q;
            roomtemp_q << "SELECT TZoneID, DefaultFreeT, Mode FROM RoomTZone WHERE RoomID="<<RoomNo;
            mysqlpp::StoreQueryResult roomtemp_res = DB_STORE(roomtemp_q.str().c_str());

            {
                if(ctxt.conf->using_parental)
                {
                    std::ostringstream ppin_set_q;
                    ppin_set_q << "SELECT PPIN FROM SystemInfo";
                    mysqlpp::StoreQueryResult ppin_set_res = DB_STORE(ppin_set_q.str().c_str());
                    //std::cout<< "empty room " << ppin_set_res[0][0] << std::endl;
                    //if(ppin_set_res[0][0] != "")
                    {
                        std::ostringstream update_q;
                        update_q << "UPDATE Room SET Parental='FALSE', PPIN='"<< ppin_set_res[0][0] <<"', GuestNo=0, GuestName='', LangID=0, LastChange='"<<get_date_time<<"', PIN='', WakeUp='0000-00-00 00:00:00' WHERE RoomID="<<RoomNo;
                        DB_EXECUTE(update_q.str().c_str());
                    }
                }
                else
                {
                    std::ostringstream update_q;
                    //update_q << "UPDATE Room SET GuestNo=0, GuestName='', LangID=0, LastChange='"<<get_date_time<<"', PIN='', WakeUp='0000-00-00 00:00:00' WHERE RoomID="<<RoomNo;
                    update_q << "UPDATE Room SET GuestNo=0, GuestName='', LangID=0, LastChange='"<<get_date_time<<"', PIN='"<< RoomNo <<"', WakeUp='0000-00-00 00:00:00' WHERE RoomID="<<RoomNo;
                    DB_EXECUTE(update_q.str().c_str());
                }
//              std::ostringstream update_q;
//              update_q << "UPDATE Room SET GuestNo=0, GuestName='', LangID=0, LastChange='"<<get_date_time<<"', PIN='', WakeUp='0000-00-00 00:00:00' WHERE RoomID=" << RoomNo;
//              DB_EXECUTE(update_q.str().c_str());
                //std::cout<<update_q.str()<<std::endl;
            }

            {
                std::ostringstream update_q;
                update_q << "UPDATE RoomTZone SET TargetT=DefaultFreeT WHERE RoomID=" << RoomNo;
                DB_EXECUTE(update_q.str().c_str());
            //  std::cout<<update_q.str()<<std::endl;
            }

            {
                std::ostringstream update_q;
                update_q << "UPDATE RoomStatus SET GreenIndic=0, RedIndic=0 WHERE RoomID=" << RoomNo;
                DB_EXECUTE(update_q.str().c_str());
                //std::cout<<update_q.str()<<std::endl;
            }

            {
                std::ostringstream update_q;
                //update_q << "DELETE FROM Message WHERE RoomID=" << RoomNo;
                update_q << "UPDATE Message SET IsActive = 'FALSE' WHERE RoomID=" << RoomNo;
                DB_EXECUTE(update_q.str().c_str());
            //  std::cout<<update_q.str()<<std::endl;
            }

            {
                std::ostringstream update_q;
                update_q << "UPDATE MoviePay SET IsActive='FALSE' WHERE RoomID=" << RoomNo;
                DB_EXECUTE(update_q.str().c_str());
                //std::cout<<update_q.str()<<std::endl;
            }
            /* Todor 2
            {
                std::ostringstream update_q;
                update_q << "UPDATE InternetPay SET IsActive='FALSE' WHERE RoomID=" << RoomNo;
                DB_EXECUTE(update_q.str().c_str());
//              std::cout<<update_q.str()<<std::endl;
            }
            */
            for(uint16 i = 0; i < roomtemp_res.num_rows(); i ++)
            {
    //          std::cout<<i<<std::endl;
                TZones.push_back((int) roomtemp_res[i][0]);
                TargetT.push_back((int) roomtemp_res[i][1]);
                Mode.push_back((int) roomtemp_res[i][2]);
            }

        TRANSACTION_END()

        if(bOK)
        {
        //  std::cout<<"delete keys\n";
            bms_delete_keys(RoomNo);

        }
        else
        {
            std::ostringstream msg;
            msg << "Неуспешна заявка за напускане на гост в стая "<<RoomNo;

            write_event(4, msg);
        }

        if(ctxt.conf->using_rc)
        {
            int Season = get_season(qr);
            std::string ips = get_rc_ip(RoomNo);

            for(uint16 i = 0; i < TZones.size(); i++)
            {
                std::ostringstream clmt;
                clmt.fill('0');
                clmt << "CLMT:" << std::setw(2) << TZones[i] - 1 << Season << std::setw(2) << TargetT[i] << std::setw(2) << TargetT[i] << Mode[i] << "*";

                send_to_rc(ips, clmt);
            }

            std::ostringstream indc;
            indc << "INDC:00*";

            send_to_rc(ips, indc);
        }

    END_DB(const_cast<DBPool&> (ctxt.rooms[ctxt.rooms_ip[RoomNo]]->db_name))
}

void pms_msg_handler(void* msg)
{
    msg_t* _msg = static_cast<msg_t*> (msg);

    mysqlpp::Connection::thread_start();

    //pcrecpp::RE("(\\x0D)").GlobalReplace("", &_msg->command);
    //pcrecpp::RE("(\\x0A)").GlobalReplace("", &_msg->command);
    pcrecpp::StringPiece input_buffer(_msg->command);
//  for(int i = 0; i < _msg->command.length(); i ++)
//      std::cout << std::hex << (int)_msg->command[i] << " ";
//  std::cout<<"\n";
    std::vector<std::string> msg_commands;
    std::string commands;

    while(pcrecpp::RE("\\x02([\\x09-\\xFF]+)\\x03").Consume(&input_buffer, &commands))
    {
        msg_commands.push_back(commands);
        std::cout<<commands<<std::endl;
    }

//  pcrecpp::RE("(\3)").Replace("", &_msg->command);

//  pcrecpp::RE("(\2)").GlobalReplace("", &_msg->command);
//  pcrecpp::RE("(\3)").GlobalReplace("", &_msg->command);
//  pcrecpp::RE("(\\s{2})").GlobalReplace("", &_msg->command);

//  std::vector<std::string> msg_decomposed;
//  pcrecpp::StringPiece input(_msg->command);
//  std::string command;

    for(uint16 c = 0; c < msg_commands.size(); c++)
    {
        std::vector<std::string> msg_decomposed;
        pcrecpp::StringPiece input(msg_commands[c]);
        std::string command;
        while(pcrecpp::RE("([\\x09-\\x7B\\x7D-\\xFF]+)\\|").Consume(&input, &command))
        {
            msg_decomposed.push_back(command);

        }

        if(ctxt.pmsanswered == 0)
        {
            if(msg_decomposed[0] == "LS")
            {
                pthread_mutex_lock(&ctxt.pms_lock);
                link_start(msg_commands[c]);
                pthread_mutex_unlock(&ctxt.pms_lock);
            } else if(msg_decomposed[0] == "LA")
            {
                pthread_mutex_lock(&ctxt.pms_lock);
                link_alive(msg_commands[c]);
                pthread_mutex_unlock(&ctxt.pms_lock);
            }
            else
            {
                logger( ERROR, "Ignorring command %s. Incorrect state.", msg_decomposed[0].c_str() );
            }
        } else if(ctxt.pmsanswered == 1)
        {
            if(msg_decomposed[0] == "LA")
            {
                pthread_mutex_lock(&ctxt.pms_lock);
                link_alive(msg_commands[c]);
                pthread_mutex_unlock(&ctxt.pms_lock);
            }
            else
            {
                logger( ERROR, "Ignorring command %s. Incorrect state.", msg_decomposed[0].c_str() );
            }

        } else {

            if(msg_decomposed[0] == "KR")
            {
                key_request(msg_commands[c]);


            } else if(msg_decomposed[0] == "KD")
            {
                key_delete(msg_commands[c]);

            } else if(msg_decomposed[0] == "DS")
            {
                ctxt.SF_RoomNo = 0;
                //sync_db(msg_commands[c], _msg->sd);

            } else if(msg_decomposed[0] == "DE")
            {
                init_synch_room_data(0, 0, "", 0, "", 0, 0);
                //sync_db(msg_commands[c], _msg->sd);
            }else if(msg_decomposed[0] == "GI")
            {
                check_in(msg_commands[c]);

            } else if(msg_decomposed[0] == "GO")
            {
                bool empty = false;

                for(uint16 i = 0; i < msg_decomposed.size(); i ++)
                {
                    if(msg_decomposed[i] == "SF")
                        empty = true;
                }

                if(empty)
                    empty_room(msg_commands[c]);
                else
                    check_out(msg_commands[c]);

            } else if(msg_decomposed[0] == "GC")
            {
                guest_change(msg_commands[c]);

            } else if(msg_decomposed[0] == "XI")
            {
                bill_info(msg_commands[c]);

            } else if(msg_decomposed[0] == "XB")
            {
                bill_total(msg_commands[c]);

            } else if(msg_decomposed[0] == "XC")
            {
                fastChoutAns(msg_commands[c]);

            } else if(msg_decomposed[0] == "PA")
            {
                posting_answer(msg_commands[c]);

            } else if((msg_decomposed[0] == "XL") || (msg_decomposed[0] == "WC") || (msg_decomposed[0] == "WR") || (msg_decomposed[0] == "XD") )
            {
                process_command(msg_commands[c]);
            }

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
pms_server_func(void*, int& pmssock, int& pms_port, std::string& pms_host, int& pmsanswered, void (*msg_handler)(void*))
{
    time_t lastcheck = time(NULL);

    pmssock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(pmssock == -1){
        logger(ERROR, "Can't create socket for pms server: %s\n", strerror(errno));
        ctxt.shutdown = 1;

        return NULL;
    }

    struct sockaddr_in isa;

    isa.sin_family = AF_INET;
    isa.sin_port = htons(pms_port);
    inet_aton(pms_host.c_str(), &isa.sin_addr);

    logger(INFO, "Successfully started pms server\n");

    try{
        do{
            pollfd pfd;
            pfd.fd = pmssock;
            pfd.events = POLLOUT;

            //int ready = poll(&pfd, 1, 2000);
            int ready = 1;
            if(ready != -1)
            {
        //      printf("%.2x\n", pfd.revents);
                //if((pfd.revents & POLLHUP) == POLLHUP)
                {
                    int ret = connect(pmssock, (sockaddr*) &isa, sizeof(isa));
                    if(ret == -1)
                    {
                        logger(ERROR, "Can not connect to IP address %s: %s\n", pms_host.c_str(), strerror(errno));
                        BEGIN_DB(ctxt.db_pool[0])
                        update_log(qr, "BMS", "BMS connection error", pms_host, pms_port, false);
                        END_DB(ctxt.db_pool[0])
                        sleep(5);
                    } else {
                        BEGIN_DB(ctxt.db_pool[0])
                        update_log(qr, "BMS", "BMS interface connection lost", pms_host, pms_port, true);
                        END_DB(ctxt.db_pool[0])
                        logger(INFO, "Connect to pms %s: %s\n", pms_host.c_str(), strerror(errno));
                        std::ostringstream init_msg;
                        init_msg << "LS|DA" << get_date << "|TI"<<get_time<<"|";
//                      send_to_pms(init_msg, pmssock);

                        do{
                            if(ctxt.conf->link_aktive != 0)
                            {
                                time_t now = time(NULL);

                                if (difftime(now, lastcheck) > ctxt.conf->link_aktive)
                                {
                                    std::ostringstream la;
                                    la << "LA|" << "DA"<< get_date << "|TI"<< get_time << "|";
                                    send_to_pms(la);

                                    lastcheck = time(NULL);
                                }
                            }

                            pfd.events = POLLIN;

                            int ready = poll(&pfd, 1, 3000);
                            if(ready != -1)
                            {

                                if((pfd.revents & POLLHUP) == POLLHUP)
                                {
                                    logger(ERROR, "Remote site closed connection for pms\n");

                                    throw errno;
                                }

                                if((pfd.revents & POLLIN) == POLLIN)
                                {
                                    char buffer[TCP_MSG_LENGTH];

                                    ssize_t received = recv(pmssock, buffer, TCP_MSG_LENGTH, 0);

                                    if(received > 0)
                                    {
                                        buffer[received] = '\0';
                                        msg_t* new_msg = new msg_t(buffer);

                                        logger(RECV, "msg %s from pms length: %d\n", buffer, received);
                                        ctxt.pool_tcp->tpool_add_work(msg_handler, new_msg);
                                    }
                                    else
                                    {
                                        throw errno;
                                    }
                                }
                                else if(ctxt.pmsanswered == 0)
                                {
                                    send_to_pms(init_msg, pmssock);
                                }
                            } else {

                                throw errno;
                            }
                        }while(ctxt.shutdown != 1);
                    }
                }
                continue;
            } else {

                throw errno;
            }

        } while (ctxt.shutdown != 1);
    } catch(int& n)
    {
        logger(FATAL, "Error in pms server on IP address %s: %s\n", pms_host.c_str(), strerror(n));
        //ctxt.shutdown = 1;
    }
    BEGIN_DB(ctxt.db_pool[0])
    update_log(qr, "BMS", "BMS interface connection lost", pms_host, pms_port, false);
    END_DB(ctxt.db_pool[0])
    pmsanswered = 0;
    shutdown(pmssock, SHUT_RDWR);
    close(pmssock);
    logger(INFO, "Closing pms server\n");

    return NULL;
}

void*
pms_server(void* context)
{
    sigset_t mask;

    sigfillset(&mask); /* Mask all allowed signals */
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
    mysqlpp::Connection::thread_start();

    std::cout<<"starting PMS 1\n";
    while(ctxt.shutdown != 1)
    {
        pms_server_func(context, ctxt.pmssock, ctxt.conf->pms_port, ctxt.conf->pms_host, ctxt.pmsanswered, pms_msg_handler);
        sleep(10);
    }

    return NULL;
}

void pms_msg_handler_2(void* msg)
{
    msg_t* _msg = static_cast<msg_t*> (msg);

    mysqlpp::Connection::thread_start();

    pcrecpp::StringPiece input_buffer(_msg->command);
        std::vector<std::string> msg_commands;
        std::string commands;

        while(pcrecpp::RE("\\x02([\\x09-\\xFF]+)\\x03").Consume(&input_buffer, &commands))
        {
            msg_commands.push_back(commands);
        }

    //  pcrecpp::RE("(\3)").Replace("", &_msg->command);

    //  pcrecpp::RE("(\2)").GlobalReplace("", &_msg->command);
    //  pcrecpp::RE("(\3)").GlobalReplace("", &_msg->command);
    //  pcrecpp::RE("(\\s{2})").GlobalReplace("", &_msg->command);

    //  std::vector<std::string> msg_decomposed;
    //  pcrecpp::StringPiece input(_msg->command);
    //  std::string command;

        for(uint16 c = 0; c < msg_commands.size(); c++)
        {
            std::cout<<msg_commands[c]<<std::endl;
            std::vector<std::string> msg_decomposed;
            pcrecpp::StringPiece input(msg_commands[c]);
            std::string command;
    while(pcrecpp::RE("([\\x09-\\x7B\\x7D-\\xFF]+)\\|").Consume(&input, &command))
    {
        msg_decomposed.push_back(command);

    }

    if(ctxt.pmsanswered2 == 0)
    {
        if(msg_decomposed[0] == "LS")
        {
            std::cout<<"state "<<ctxt.pmsanswered2<<std::endl;
            pthread_mutex_lock(&ctxt.pms_lock2);
            link_start_2(msg_commands[c]);
            pthread_mutex_unlock(&ctxt.pms_lock2);
            std::cout<<"enter\n";
        } else if(msg_decomposed[0] == "LA")
        {
            pthread_mutex_lock(&ctxt.pms_lock2);
            link_alive_2(msg_commands[c]);
            pthread_mutex_unlock(&ctxt.pms_lock2);
        }
        else
        {
            logger( ERROR, "Ignorring command %s. Incorrect state.", msg_decomposed[0].c_str() );
        }
    } else if(ctxt.pmsanswered2 == 1)
    {
        if(msg_decomposed[0] == "LA")
        {
            pthread_mutex_lock(&ctxt.pms_lock2);
            link_alive_2(msg_commands[c]);
            pthread_mutex_unlock(&ctxt.pms_lock2);
        }
        else
        {
            logger( ERROR, "Ignorring command %s. Incorrect state.", msg_decomposed[0].c_str() );
        }

    } else {

        if(msg_decomposed[0] == "XI")
        {
            pthread_mutex_lock(&ctxt.pms_lock2);
            bill_info(msg_commands[c]);
            pthread_mutex_unlock(&ctxt.pms_lock2);

        } else if(msg_decomposed[0] == "XB")
        {
            pthread_mutex_lock(&ctxt.pms_lock2);
            bill_total(msg_commands[c]);
            pthread_mutex_unlock(&ctxt.pms_lock2);

        } else if(msg_decomposed[0] == "PA")
        {
            posting_answer(msg_commands[c]);

        } else if((msg_decomposed[0] == "XL") || (msg_decomposed[0] == "WC") || (msg_decomposed[0] == "WR") || (msg_decomposed[0] == "XD") )
        {
            process_command(msg_commands[c]);
        }

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
pms_server_2(void* context)
{
    sigset_t mask;

    sigfillset(&mask); /* Mask all allowed signals */
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
    mysqlpp::Connection::thread_start();

    while(ctxt.shutdown != 1)
    {
        pms_server_func(context, ctxt.pmssock2, ctxt.conf->pms_port_2, ctxt.conf->pms_host_2, ctxt.pmsanswered2, pms_msg_handler_2);
        sleep(10);
    }

    return NULL;
}
