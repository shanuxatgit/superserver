/* -------------------------------------------------------------------------
 * his_server.cpp - his server functions
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

void internet(std::string &req, int cfd)
{
    int RoomNo = 0;
    int InternetType = 0;
    int Official = 0;
    std::string Reason;
    int EmpID = 0;

    DECODE_BEGIN(req)
        EXTRACT("RN(\\d+)", RoomNo)
        EXTRACT("TA(\\d+)", InternetType)
        EXTRACT("OF(\\d+)", Official)
        EXTRACT("RS(\\d+)", Reason)
        EXTRACT("EI(\\d+)", EmpID)
    DECODE_END()

    make_gen(RoomNo, InternetType, 1, cfd, "HIS", Official, Reason, EmpID);
}

void his_msg_handler(void* msg)
{
    msg_t* _msg = static_cast<msg_t*> (msg);

    mysqlpp::Connection::thread_start();

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

    if(msg_decomposed[0] == "IN")
    {
        internet(_msg->command, _msg->sd);
        std::cout << "IN command" << std::endl;

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
his_server(void* )
{
    return server(ctxt.hissock, ctxt.conf->his_port, ctxt.conf->his_host, his_msg_handler, server_accept, &context_t::add_his, &context_t::del_his);
}
