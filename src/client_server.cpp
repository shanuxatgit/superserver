/* -------------------------------------------------------------------------
 * client_server.cpp - client server functions
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

void sync_db(std::string& req, int cfd)
{
    std::string Date;
    std::string Time;

    DECODE_BEGIN(req)
        EXTRACT("DA([\\w]+)", Date)
        EXTRACT("TI([\\w]+)", Time)
    DECODE_END()

    std::ostringstream msg_client;
    msg_client << "DS|DA" << get_date << "|TI" << get_time << "|";

    send_to_client(msg_client, cfd);

    BEGIN_DB(ctxt.db_pool[0])

        std::ostringstream keycard_q;
        keycard_q << "SELECT hex(KeyID) As KeyNo, KHName,KHRoomID  FROM Keycard WHERE KHTypeID IN(";

        if(ctxt.conf->client_TypeId.size() > 0)
        {
            for(uint16 i = 0; i < ctxt.conf->client_TypeId.size() - 1; i++)
            {
                keycard_q << ctxt.conf->client_TypeId[i] << ",";
            }

            keycard_q << ctxt.conf->client_TypeId[ctxt.conf->client_TypeId.size()-1] << ")";
        }
        else
            keycard_q << "FALSE)";

        mysqlpp::StoreQueryResult keycard_res = DB_STORE(keycard_q.str().c_str());

        if(keycard_res.num_rows() > 0)
        {
            for(uint16 i = 0 ; i < keycard_res.num_rows(); i++)
            {
                std::ostringstream message_back;
                message_back << "KN|KI" << keycard_res[i][0].c_str() << "|HN"  << keycard_res[i][1].c_str()  << "|RN" << keycard_res[i][2].c_str() << "|SF|";
                int res = send_to_client(message_back, cfd);
                if(!res)
                    break;
                sleep (1);
            }
        }
        {
            std::ostringstream message_back;
            message_back << "DE|DA" << get_date << "|TI" << get_time << "|";
            send_to_client(message_back, cfd);
        }
    END_DB(ctxt.db_pool[0])
}

void client_msg_handler(void* msg)
{
    msg_t* _msg = static_cast<msg_t*> (msg);

    mysqlpp::Connection::thread_start();

    pcrecpp::RE("(\2)").Replace("", &_msg->command);
    pcrecpp::RE("(\3)").Replace("", &_msg->command);
    pcrecpp::RE("(\\s{2})").Replace("", &_msg->command);

    std::vector<std::string> msg_decomposed;
    pcrecpp::StringPiece input(_msg->command);
    std::string command;

    while(pcrecpp::RE("([\\x20-\\x7B\\x7D-\\xFF]+)\\|").Consume(&input, &command))
    {
        msg_decomposed.push_back(command);

    }

    if(msg_decomposed.size() != 0)
    {

    if(msg_decomposed[0] == "DR")
    {
        sync_db(_msg->command, _msg->sd);
        std::cout << "DR command" << std::endl;
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
client_server(void* )
{
    return server(ctxt.clientsock, ctxt.conf->client_port, ctxt.conf->client_host, client_msg_handler, server_accept, &context_t::add_client, &context_t::del_client);
}
