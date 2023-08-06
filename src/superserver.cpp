#include <stdio.h>       /* standard I/O routines                     */
#include <pthread.h>     /* pthread functions and data structures     */
#include <stdlib.h>      /* rand() and srand() functions              */
#include <unistd.h>
#include <string>
#include <signal.h>
#include <iostream>
#include <vector>
using std::cout;
using std::endl;
#include "log.h"
#include "common.h"
#include "tpool.h"
#include "servers.h"

context_t ctxt;

void exit_server()
{
    if(ctxt.conf->using_rc)
        pthread_join(ctxt.rcthread, NULL);

    printf("exiting server\n");
     int ret = pthread_join(ctxt.mngthread, NULL);

    ret = pthread_join(ctxt.phpthread, NULL);

    if(ctxt.conf->using_pms)
    {
        pthread_join(ctxt.pmsthread, NULL);
        if(ctxt.conf->second_interface)
            pthread_join(ctxt.pmsthread2, NULL);
    }

    ret = pthread_join(ctxt.histhread, NULL);

    ret = pthread_join(ctxt.clientthread, NULL);

    if(ctxt.pool != NULL){
        delete ctxt.pool;
        ctxt.pool = NULL;
    }

    if(ctxt.pool_tcp != NULL){
        delete ctxt.pool_tcp;
        ctxt.pool_tcp = NULL;
    }
    logger(2, "====================|| Server Close ||====================\n");

    if(ctxt.log != NULL)
    {
        delete ctxt.log;
        ctxt.log = NULL;
    }

    if(ctxt.conf != NULL)
    {
        delete ctxt.conf;
        ctxt.conf = NULL;
    }
}

static void sigtermhandler(int n)
{
    logger(FATAL, "Signal %d", n);
    logger(FATAL, "Signal %d", n);
    logger(FATAL, "Signal %d", n);
    ctxt.shutdown = 1;
    exit_server();

//  exit (0);
}

static void sigtermhandlerlog(int)
{
    std::cout<<"sig HUP handeled\n";
    ctxt.log->rotate(LOG_STDERR, ctxt.conf->log_file.c_str());
    ctxt.db_pool[0].shrink(); //add all db-s here
}

void poll_rcs()
{
    int rcs = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(rcs == -1){
        logger(ERROR, "Can't create socket for broadcast\n");

        return;
    }

    int brodacast = 1;

    setsockopt(rcs, SOL_SOCKET, SO_BROADCAST, &brodacast, sizeof(int));

    struct sockaddr_in isa;

    isa.sin_family = AF_INET;
    isa.sin_port = htons(ctxt.conf->rc_port);
    inet_aton(ctxt.conf->rc_broadcast.c_str(), &isa.sin_addr);

    std::ostringstream message;

    message.fill('0');
    message << "POLL:"<<std::setw(8)<< std::hex << std::uppercase << time(NULL)<<"*";

    CRC(message);

    sendto(rcs, message.str().c_str(), message.str().length(), 0, (sockaddr *) &isa, sizeof(isa));

    shutdown(rcs, SHUT_RDWR);
    close(rcs);

    std::ostringstream event;

    event << "Стартиране на BMS сървър...";
    write_event(2, event);
}

void work(void*)
{
    sleep(1);
    logger(INFO, "%d", rand());
}

/* like any C program, program's execution begins in main */
extern void check_in(std::string& req);
extern void new_card(std::string&, int);
extern void check_out(std::string& req);
extern void empty_room(std::string& req);

int
main()
{
    ctxt.conf = new conf_t();
    if(ctxt.conf == NULL)
        return EXIT_FAILURE;

    ctxt.conf->read_conf();
    ctxt.db_pool[0].set_db(ctxt.conf->database);
    ctxt.db_rel.insert(make_pair(string(""), ctxt.db_pool[0]));

    ctxt.log = new log_t(LOG_STDERR, ctxt.conf->log_file.c_str());
    if(ctxt.log == NULL)
        return EXIT_FAILURE;

    ctxt.pool = new tpool_t(300, 3000);
    if(ctxt.pool == NULL)
        return EXIT_FAILURE;

    ctxt.pool_tcp = new tpool_t(10,20);
    if(ctxt.pool_tcp == NULL)
        return EXIT_FAILURE;

    logger(INFO, "====================|| Server Start ||====================\n");

 //   {
        logger(INFO, "Starting DB repair\n");
        BEGIN_DB(ctxt.db_pool[0])
 //         mysqlpp::StoreQueryResult res = DB_STORE("REPAIR TABLE AccessControlLog , CurrencyRate , DoorLockEvent , DoorLockLog , Emp , Genre , GenreLang , InfoText , Internet , InternetOnLine , InternetPay , Lang , LangSub , Message , Movie , MovieLang , MovieLog , MovieOfficalRentLog , MoviePay , MovieSubLang , Music , MusicGenre , MusicGenreLang , OfficialOperationLog , Page , PageMenu , PlayLists , PlayListsDet , RadioChannel , RoomService , RoomServiceCell , RoomServiceMenu , RoomServiceRow , RoomStatus , RoomTZoneLog , RoomVolumeScheme , RoomWS , ServerEventLog , ServerEventType , SystemInfo , Weather , WeatherLang , WeatherPointData , WeatherPointLang , WeatherType , WeatherTypeLang");

  //        for (size_t i = 0; i < res.num_rows(); ++i) {
    //          if(strcmp(res[i][3], "OK"))
        //          logger(ERROR, "Error in table %s: %s\n", res[i][0].c_str(), res[i][3].c_str());

            //}
        //Todor 20
            std::ostringstream columnExist;
            columnExist << "SHOW COLUMNS FROM `Room` LIKE 'GroupCode'";
            mysqlpp::StoreQueryResult checkColumn = DB_STORE(columnExist.str().c_str());
            if(checkColumn.num_rows() > 0)
            {
                logger(INFO, "===||Column GroupCode exist  continue||===\n");
            } else {
                logger(INFO, "===||WARNING Column GroupCode DON'T  exist Createing column!||===\n");
                mysqlpp::StoreQueryResult temoRes = DB_STORE( "ALTER TABLE Room ADD COLUMN GroupCode VARCHAR(45) NULL DEFAULT '0' AFTER GuestRights");
            //DB_STORE( "ALTER TABLE Room ADD COLUMN GroupCode VARCHAR(45) NULL DEFAULT '0' AFTER GuestRights"
            }

            mysqlpp::StoreQueryResult db_room = DB_STORE("SELECT DISTINCT DBName from Room");

            for(size_t i = 0; i < db_room.num_rows(); ++i) {
                std::string db_name = db_room[i][0].c_str();
                if(!db_name.empty())
                {
                    // std::cout << db_name << endl;
                    ctxt.db_pool[ctxt.db_num++].set_db(db_name);
                    // ctxt.db_rel.insert(make_pair(db_name, ctxt.db_pool[ctxt.db_num]));
                    // ctxt.db_num++;
                    // std::cout << std::hex << &ctxt.db_rel[db_name] << std::endl;
                }
            }

            mysqlpp::StoreQueryResult resrooms = DB_STORE("SELECT RoomID, IP_RC, DBName, RoomName FROM Room ORDER BY RoomID asc");

            for (size_t i = 0; i < resrooms.num_rows(); ++i) {
                std::string db_name = resrooms[i][2].c_str();
                if(db_name.empty())
                {
                    std::cout << "empty db" << " roomno " << atoi(resrooms[i][0]) << std::endl;
                    ctxt.rooms.insert(make_pair(string(resrooms[i][1].c_str()), new room_status_t(atoi(resrooms[i][0]), ctxt.db_pool[0])));
                }
                else
                {
                    std::cout << db_name << std::endl;
                    for(size_t j = 0; j < ctxt.db_num; j++)
                    {
                        if(ctxt.db_pool[j].get_db() == db_name)
                        {
                            std::cout << "db_number " << j << " roomno " << atoi(resrooms[i][0]) << std::endl;
                            ctxt.rooms.insert(make_pair(string(resrooms[i][1].c_str()), new room_status_t(atoi(resrooms[i][0]), ctxt.db_pool[j])));
                            break;
                        }
                    }
                }

                ctxt.rooms_ip.insert(make_pair(atoi(resrooms[i][0]), string(resrooms[i][1].c_str())));
                ctxt.rooms_id.insert(make_pair(string(resrooms[i][3].c_str()), atoi(resrooms[i][0])));
                ctxt.rooms_name.insert(make_pair(atoi(resrooms[i][0]), string(resrooms[i][3])));
            }

            if(ctxt.rooms_ip[601].empty())
                std::cout << "empty room" << std::endl;
            else
                std::cout << "room ip: " << ctxt.rooms_ip[601] << std::endl;

            mysqlpp::StoreQueryResult rks = DB_STORE("SELECT BMS_WSID, BMS_WSIP FROM BMS_WS ORDER BY BMS_WSID asc");

            for (size_t i = 0; i < rks.num_rows(); ++i) {
                ctxt.ks.insert(make_pair(string(rks[i][1].c_str()), (int)rks[i][0]));
            }

        END_DB(ctxt.db_pool[0])
        logger(INFO, "Finished DB repair\n");
 //   }

//      {
//      //  std::string out = "GO|RN203|G#63845|GSN|";
//      //  check_out(out);
//      //  ctxt.SF_RoomNo = 201;
//          std::string in = "GI|RN203|G#63845|GNMr Kim|GLEN|GSN|SF|";
//          check_in(in);
//          std::string in2 = "GI|RN201|G#63846|GNMr Kim|GLEN|GSN|SF|";
//          check_in(in2);
////            std::string out_3 = "GO|RN203|GSN|SF|";
////            empty_room(out_3);
//          std::string out_1 = "GO|RN202|GSN|SF|";
//          empty_room(out_1);
////            std::string out_2 = "GO|RN201|GSN|SF|";
////            empty_room(out_2);
//  //          std::string out = "GO|RN203|G#63845|GSN|";
//   //         check_out(out);
//      }

    if(ctxt.conf->using_rc){
        if(pthread_create(&ctxt.rcthread, NULL, rc_server, NULL) != 0){
            logger(ERROR, "Can not create rc server thread\n");

            return EXIT_FAILURE;
        }
    }

    if(pthread_create(&ctxt.mngthread, NULL, mng_server, NULL) != 0){
        logger(ERROR, "Can not create mng server thread\n");

        return EXIT_FAILURE;
     }

    if(pthread_create(&ctxt.phpthread, NULL, php_server, NULL) != 0){
        logger(ERROR, "Can not create php server thread\n");

        return EXIT_FAILURE;
     }

    if(ctxt.conf->using_pms){
        if(pthread_create(&ctxt.pmsthread, NULL, pms_server, NULL) != 0){
            logger(ERROR, "Can not create pms server thread\n");

            return EXIT_FAILURE;
        }

        if(ctxt.conf->second_interface)
        {
            if(pthread_create(&ctxt.pmsthread2, NULL, pms_server_2, NULL) != 0){
                logger(ERROR, "Can not create pms server thread\n");

                return EXIT_FAILURE;
            }
        }
    }

    if(pthread_create(&ctxt.histhread, NULL, his_server, NULL) != 0){
        logger(ERROR, "Can not create his server thread\n");

        return EXIT_FAILURE;
     }

    if(pthread_create(&ctxt.clientthread, NULL, client_server, NULL) != 0){
        logger(ERROR, "Can not create his server thread\n");

        return EXIT_FAILURE;
    }

    //========= Catch terminate signals: ================
    // terminate requests:
    signal(SIGTERM, sigtermhandler); // kill
    signal(SIGHUP, sigtermhandlerlog);  // kill -HUP
    //signal(SIGTERM, sigtermhandler); //term closed

    signal(SIGINT, sigtermhandler);  // Interrupt from keyboard

    signal(SIGQUIT, sigtermhandler); // Quit from keyboard
    // fatal errors:
    signal(SIGBUS, sigtermhandler);  // bus error
    signal(SIGSEGV, sigtermhandler); // segfault
    signal(SIGILL, sigtermhandler);  // illegal instruction
    signal(SIGFPE, sigtermhandler);  // floating point exc.
    signal(SIGABRT, sigtermhandler); // abort()

 //   exit_server();
    poll_rcs();
    while(!ctxt.shutdown)
    {
        pause();
    }

    return 0;
}
