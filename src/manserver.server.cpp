/////////////////////////////////////////
#include "manserver.server.h"

#include "c_var.h"

/////////////////////////////////////////
CServer::CServer(bool bIsQuiet) {
    pLog         = 0;
    bQuiet       = bIsQuiet;
    bQuit        = false;
    bRestart     = false;
    pFirstPlayer = 0;
    pConsole     = 0;
    pCVars       = 0;

    randomize();

    pLog = new CLog("server.log", bQuiet);  // LOG
    if (!pLog) {
        printf("\nNo memory for log!\n");
        exit(0);
    }
    pLog->On();
    pLog->Restart();

    StartUp();  // STARTUP/SHUTDOWN
}

/////////////////////////////////////////
CServer::~CServer() {
    kick_all("Server shutting down");
    C_GSC *c = pFirstPlayer;
    while (c) {
        pFirstPlayer = c;
        c            = c->pNext;
        DEL(pFirstPlayer);
    }
    shut_down();  // STARTUP/SHUTDOWN
    NET_Shutdown();
    DEL(pLog);
}
/////////////////////////////////////////
bool CServer::check_restart(void) {
    if (bRestart) {
        LogEntry("[Initiating server restart]=====================================");
        shut_down();
        StartUp();
        LogEntry("==============================================[Server restarted]");
        bRestart = 0;
        return true;
    }
    return false;
}

/////////////////////////////////////////

int CServer::StartUp(void) {
    ////////////////////////////////////////////////////////////////////////////
    // Initialize some variables
    dwStartTime = dlcs_get_tickcount();
    int iRetVal = false;

    ////////////////////////////////////////////////////////////////////////////
    // Log title stuff
    /*
    LogEntry("                                 ░▒");
    LogEntry("     ░▓▓█▓▓▓▓▒▒░          ░▓█");
    LogEntry("   ░▓████▒          █▒  █▒ ▒█▓                            ░░");
    LogEntry("   ▒███▓░        ░██▓ ▓██ ▒██▓▓▒░░ ░▓████▓▓▒░ ▓█████▒");
    LogEntry("   ████████▓▒ ░▓███████░████████▒███▒░░██▒▓██▓▒██▓");
    LogEntry("  ░██▓         ░██░███ ████▓   ░██▒▓██████░ ▒██▓▒");
    LogEntry("  ░███▓░      ▓██ ░█▒  ████▒░▒▓███▒▒██░      ▓██░");
    LogEntry("   ░▒▓█████████▓   ░   ██▒██████▒░░▒▓██▓▒░░▓█");
    */
    LogEntry("////////////////////////////////////////////////////////////////////////////");
    LogEntry("Mantra Server %s(%s - %s) Net(%s) Build(%d) %s\n", MANTRA_VERSION, DLCS_CPU_STRING, DLCS_OS_STRING, MANTRA_NET_REVISION, MANTRA_S_BUILD, MANTRA_COPYRIGHT);
    LogEntry("BUILD: %s %s", __DATE__, __TIME__);
    LogEntry("////////////////////////////////////////////////////////////////////////////");
    /*
    LogEntry("°±²ÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛßßßßÛßßßßßßßßÛÛÛÛßßßßÛÛÛÛÛÛÛÛÛÛÛßßßßÛÛÛÛÛÛÛÛÛ²±°");
    LogEntry("                  ÜÛÛÜ ÜÛÛÛÛÛß     ÜÛÛ             ÛÛ ");
    LogEntry("                ÜÛß      ÞÛ  ÜÛÛ  Ûß ßÛ  ÜÛÛÛÛÛß  ÞÛ  ");
    LogEntry("                ßÛÜÜÜ    ÛÝ Ûß Û ÛÝ ÜÛ     ÞÛ     Û   ");
    LogEntry("                  ßßÛÛ  ÞÛ ÞÛ ÜÛ ÛÛßÛÛ     ÛÝ    ÞÝ   ");
    LogEntry("                ÜÜ  ÞÛ  ÛÝ ÛÛßÛÛ ÛÛ  ÞÛ   ÞÛ          ");
    LogEntry("                 ßßßß      ÛÝ ÛÝ Û    Û   ÛÝ    Û     ");
    LogEntry("°±²ÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÜÜÜÜÜÜÜÜÜÛÛÜÜÜÛÜÜÜÛÛÛÜÜÜÛÛÛÛÛÛÛÛÛÛÛÛÛ²±°");
    */

    ////////////////////////////////////////////////////////////////////////////
    // Log start time... Todo: Store proper timestamp for server start time
    // LogEntry("STARTTIME:%s", dlcs_timestamp(dwStartTime));

    ////////////////////////////////////////////////////////////////////////////
    // Setup CVars
    LogEntry("Initializing CVars...");
    pCVars = new CVarSet(pLog);

    // Define default variables
    LogEntry("Setting default CVar values...");
    pCVars->set_cvar("i_world_save_timer", "300000");  // 5 minutes tick time
    // Overwrite defaults from server.ini
    LogEntry("Loading server.ini CVar values...");
    pCVars->bLoad("server.ini");
    LogEntry("CVars initialized...");
    /*
        LogEntry("s_name         = [%s]", (char *)pCVars->get_cvar("s_name"));
        LogEntry("s_admin_email  = [%s]", (char *)pCVars->get_cvar("s_admin_email"));
        LogEntry("s_website_link = [%s]", (char *)pCVars->get_cvar("s_website_link"));
        LogEntry("s_motd         = [%s]", (char *)pCVars->get_cvar("s_motd"));
        */
    LogEntry("////////////////////////////////////////////////////////////////////////////");

    ////////////////////////////////////////////////////////////////////////////
    // Initialize Console
    LogEntry("Initializing Console...");
    pConsole = new C_CONS(pLog, pCVars);  // Initialize Console for it has cvars built in
                                          // Todo: SetupConsoleHistory
                                          // SetupConsoleHistory();
                                          // Todo: Register Server Functions
                                          // pCVars->bRegisterFunction("kick_all", kick_all);

    ////////////////////////////////////////////////////////////////////////////
    // Attempt to disable power management
    // Todo: make this a configurable option under CVars

    LogEntry("Disabling power management...");
    dlcs_suspend_power_management();

    ////////////////////////////////////////////////////////////////////////////
    // Attempt to Initialize the Network

    LogEntry("Initializing Network...");
    iRetVal = bNetStartUp();
    if (!iRetVal) {
        return iRetVal;
        LogEntry("Network initialization FAILURE!");
    } else {
        LogEntry("Network initialized...");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Initialize Chat Channels
    LogEntry("Initializing Default Chat Channels...");
    dfGUI_CHAT();
    LogEntry("Default chat channels initialized...");

    ////////////////////////////////////////////////////////////////////////////
    // Initialize Database Todo: finish this section
    LogEntry("Initializing Database...");
    /* pSQLite      = 0; // add dlcs_db
    // memset(&r_data, 0, sizeof(r_data));
    // add dlcs_db initialization
    pSQLite = new C_SQLite();  // Open User Database
    if (pSQLite) {
        LogEntry("SQLite started");
        pSQLite->pLog = pLog;
        pSQLite->OpenDB("data_new.sqlite");
        LogEntry("SQLite opened DB data_new.sqlite");
    }
    // pSQLite->db_query("update users set chat_channels = 'SYSTEM'",0);
    // pSQLite->db_query("alter table users add column NAME char(256) default 'SYSTEM'");
        if (!pSQLite->db_query("select * from users")) {
            LogEntry("=====================[WARNING! ERROR!]======================");
            LogEntry("Database [data.sqlite] empty or corrupt");
            LogEntry("=====================[WARNING! ERROR!]======================");
            bQuit = true;
            // where username='seth'
            // pSQLite->db_query("CREATE TABLE users (username varchar(32), password varchar(32), access smallint)",0);
            // pSQLite->db_query("CREATE TABLE users(id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT default 'user',password TEXT default 'none',access smallint default '0',chat_channels TEXT default 'SYSTEM')",0);
            // pSQLite->db_query((char *)va("INSERT INTO users VALUES (1, 'seth', '%s', 255)",md5_digest("123")),0);
            // pSQLite->db_query((char *)va("INSERT INTO users (username) VALUES ('seth_also')",0));
            // pSQLite->db_query((char *)va("INSERT INTO users VALUES ('seth','%s', 255)",md5_digest("123")),0);
            // pSQLite->db_query((char *)va("INSERT INTO users VALUES ('test','%s', 5)",  md5_digest("226fi3")),0);
            // pSQLite->db_query((char *)va("INSERT INTO users VALUES ('test2','%s', 6)", md5_digest("2326df3")),0);
            // pSQLite->db_query((char *)va("INSERT INTO users VALUES ('test4','%s', 7)", md5_digest("223k6gf3")),0);
            // pSQLite->db_query((char *)va("INSERT INTO users VALUES ('zany','%s', 8)",  md5_digest("22lg63f3")),0);

        } else {
            // LogEntry("seth.password=[%s]",       (char *)  db_getvalue("username","seth","password").c_str());
            // LogEntry("seth.access=[%d]",     atoi((char *) db_getvalue("username","seth","access").c_str()));
        }
    */
    LogEntry("Database initialized... (TODO PLACEHOLDER TEXT)");

    ////////////////////////////////////////////////////////////////////////////
    // Todo: finish load_world function
    LogEntry("Initializing World...");
    load_world();
    LogEntry("World loaded...");

    ////////////////////////////////////////////////////////////////////////////
    // Todo: Add in character loading for NPC's and such here
    LogEntry("Initializing NPCs...");
    /*  C_GSC *c_x = 0;
        c_x        = new C_GSC(pLog, this);
        strcpy(c_x->username, "seth");
        c_x->load();
        CMPCharacter *pc = new CMPCharacter(c_x);
        // pc->load(pSQLite);  // add dlcs_db features
        pc->gender                = 1;
        pc->health->current_value = 2;
        DEL(c_x);
        DEL(pc);
        */
    LogEntry("NPC's initialized... (TODO PLACEHOLDER TEXT)");

    LogEntry("////////////////////////////////////////////////////////////////////////////");
    LogEntry("CServer::StartUp... FINISHED!            (Type '/help' for help on commands)");
    LogEntry("////////////////////////////////////////////////////////////////////////////");
    return iRetVal;
}

bool CServer::bNetStartUp() {
    int iPort;
    iPort = atoi((const char *)pConsole->pCVars->get_cvar("i_port"));
    NET_Init();
    initSocket();
    if ((Listen(iPort, true)) == -1) {
        LogEntry("ERROR LISTENING ON PORT %d\n", iPort);
        return false;
    } else {
        LogEntry("Listening on port [%d]", iGetLocalPort());
        return true;
    }
}

/////////////////////////////////////////
void CServer::shut_down(void) {
    LogEntry("////////////////////////////////////////////////////////////////////////////");
    LogEntry("Shutting down the server...");
    LogEntry("////////////////////////////////////////////////////////////////////////////");
    LogEntry("CServer::shut_down: Saving World");
    save_world();
    LogEntry("CServer::shut_down: Saving CVars");
    save_data();
    LogEntry("CServer::shut_down: Cleaning up Console");
    DEL(pConsole);  // RemoveConsoleHistory();
    LogEntry("CServer::shut_down: Cleaning up CVars");
    DEL(pCVars);
    LogEntry("CServer::shut_down: Cleaning up Players");
    DEL(pFirstPlayer);
    LogEntry("CServer::shut_down: Cleaning up Database");
    // DEL(pSQLite); // add dlcs_db shutdown (DEL)

    ///////////////////////////////////////////
    /*
        LogEntry("==================================================================================================");
        LogEntry("                                                   ░░░   ░░░                        ░▒▒░ ");
        LogEntry("         ▒▓█████████▓▒     ░░    ▒▒    ░░░  ▓▓▓  ░██▓                      ▒██▒");
        LogEntry("         ▒▓█████████▓▒     ▓█░   ▓█    ▓██░ ███  ▒███                     ▓██▓");
        LogEntry("       ░▓█████████████▓▒  ▓██ ░███   ▒██░ ███  ▒███                     ▓██▓");
        LogEntry("       ▒███░  ░██▓  ░██▓▒  ▓██████▒    ░░  ███  ░███░                   ░███▓");
        LogEntry("       ▒█████████▓  ▒██▒   ▓████▓░   ░▒▒░ ███   ███▒  ░▓▓▓███▒ ░▒██████▓");
        LogEntry("       ▓███████████████░  ▓█████▒   ▒██▒ ███   ███▓  ▒▓██░ █▓░███░▒███▓");
        LogEntry("       ░▓▓███████████▓▒   ▓██▓▒██    ██   ███   ███▓  ▓█▓  ▓██▒██░  ▒██▓");
        LogEntry("          ▓█▒▒ ░░▒▒▒█▒       ▓██ ░▒██▓ ██▓  ███▒  ████░▓████▓▒  ██   ░██▓");
        LogEntry("          ▓█▒▒ ░░▒▒▒▒        ▒██   ░██░▒██▒  ████░ ████▓▓██▓    ▒███▒░▒███░");
        LogEntry("             ▒░░   ░░ ░        ░▓▒    ▒█░░██▒   ▒███░ ▒███▓▒▒███▒ ░▓████████▓░");
        LogEntry("          ░▒▓▓▓▓▓▓▓▒░       ░▒▒▒░░░▓█▓▒░  ░░░▒▓▓░░  ░▒▒░ ░▓████▓░ ░▒██▓▓▒░");
        LogEntry(" ");
        LogEntry("==================================================================================================");
        */
    LogEntry("////////////////////////////////////////////////////////////////////////////");
    LogEntry("Server Killed!");
}

/////////////////////////////////////////
void CServer::do_keyboard() {
    u_char ch = 0;
    if (_kbhit()) {
        ch = _getch();
        switch (ch) {
            case 0:
                ch = _getch();
                switch (ch) {
                    default: break;
                }
                break;

            case 13:
            case 10:
                /* TODO: Re Add History
                AddToConsoleHistory(szConsoleInput);
                iConsoleHistoryPosition = 0;                    */

                LogEntry(">%s", pConsole->szConsoleInput);
                if (pConsole->szConsoleInput[0] == 0)
                    break;
                else {
                    console_command(pConsole->szConsoleInput);
                    memset(pConsole->szConsoleInput, 0, _DLCS_CONSOLE_ENTRY_SIZE);
                }
                break;

            case 27: break;

            case 8:  // backspace
                pConsole->szConsoleInput[strlen(pConsole->szConsoleInput) - 1] = 0;
                printf(">                                                  \r>%s \r", pConsole->szConsoleInput);
                break;

            case 21:  // ctrl-U
                printf(">                                                  \r>");
                memset(pConsole->szConsoleInput, 0, _DLCS_CONSOLE_ENTRY_SIZE);
                break;

            case 0xE0:  // Arrow keys...
                ch = _getch();
                switch (ch) {
                    case 72:  // UP
                              /*
                                  strcpy(szConsoleInput, GetConsoleHistory(iConsoleHistoryPosition));
                                  iConsoleHistoryPosition++;
                                  if (iConsoleHistoryPosition > MAX_CONSOLE_INPUT_BUFFER) iConsoleHistoryPosition = 0;
                                  printf(">                                                  \r>%s \r", szConsoleInput);
                                  */
                        break;

                    case 80:  // DOWN
                              /*
                                  strcpy(szConsoleInput, GetConsoleHistory(iConsoleHistoryPosition));
                                  iConsoleHistoryPosition--;
                                  if (iConsoleHistoryPosition < 0) iConsoleHistoryPosition = MAX_CONSOLE_INPUT_BUFFER;
                                  printf(">                                                  \r>%s \r", szConsoleInput);
                                  */

                        break;

                    case 75:  // LEFT (TODO: build cursor into console command input)
                        break;

                    case 77:  // RIGHT (TODO: build cursor into console command input)
                        break;

                    default: break;
                }
                break;

            default:
                int i                           = strlen(pConsole->szConsoleInput);
                pConsole->szConsoleInput[i]     = ch;
                pConsole->szConsoleInput[i + 1] = 0;
                printf(">                                                  \r>%s \r", pConsole->szConsoleInput);
                break;
        }
    }
}

/////////////////////////////////////////
void CServer::cycle(void) {
    static C_GSC *pClient = 0;
    if (pClient == 0) pClient = pFirstPlayer;
    if (pClient) {
        pClient->do_net();
        pClient = pClient->pNext;
    }
    check_restart();
    do_keyboard();
    accept_connection();
    do_world();
    save_world();
    purge_clients();
}

/////////////////////////////////////////
void CServer::accept_connection(void) {
    C_GSC *            client;
    int                iLength;
    int                iFlags, iHelp;
    int                iAcceptSocket;
    char               cType;
    struct sockaddr_in ServerAddr;
    memset(&ServerAddr, 0, sizeof(ServerAddr));
    CPacket Send(NET_DATAGRAMSIZE);
    CPacket Recv(NET_DATAGRAMSIZE);
    iAcceptSocket = CheckNewConnections();
    if (iAcceptSocket == -1) return;
    if ((iLength = nRecv(iAcceptSocket, Recv.pGetPacketBuffer(), Recv.iGetMaxSize(), (struct sockaddr *)&ServerAddr)) <= 0) return;
    Recv.SetCurSize(iLength);
    iHelp  = ntohl(Recv.iRead());
    iFlags = iHelp & (~NET_FLAG_LENGTH_MASK);
    iHelp &= NET_FLAG_LENGTH_MASK;
    if (!(iFlags & NET_FLAG_CTL)) return;
    if (iLength != iHelp) return;
    if (iLength < (int)sizeof(int) * 2) return;
    Recv.iRead();
    cType = Recv.cRead();
    switch (cType) {
        case CTL_CONNECT:
            if (strcmp(Recv.pRead(), "MANTRA")) {
                LogEntry("Fail attempt, not MANTRA packet");
                reject_connection(iAcceptSocket, (struct sockaddr *)&ServerAddr, "This is a Ember Game Server");
                return;
            }
            if (Recv.iRead() != atoi(MANTRA_NET_REVISION)) {
                reject_connection(iAcceptSocket, (struct sockaddr *)&ServerAddr, "Server uses newer protocol! Update available.");
                return;
            }
            // TODO: Add reject (PRIVATE SERVER)
            // create new client object
            client = new C_GSC(pLog, this);
            if (!client) {
                LogEntry("manserver.server.cpp -> Can't create client");
                return;
            }
            if (!client) {
                reject_connection(iAcceptSocket, (struct sockaddr *)&ServerAddr, "Server can't assign a new socket!");
                return;
            }
            memcpy(&client->ToAddr, &ServerAddr, sizeof(ServerAddr));
            client->iSocket = zOpenSocket(0);
            if (client->iSocket == -1) {
                DEL(client);
                reject_connection(iAcceptSocket, (struct sockaddr *)&ServerAddr, "Server can't assign new socket!");
                return;
            }
            Send.Reset();
            Send.Write(0);
            Send.Write(0);
            Send.Write((char)CTL_ACCEPT);
            Send.Write(client->iGetLocalPort());
            FinishCtlPacket(&Send);
            if (nSend(iAcceptSocket, Send.pGetPacketBuffer(), Send.iGetCurSize(), (struct sockaddr *)&ServerAddr) == -1) {
                DEL(client);
                LogEntry("s_server.cpp -> CSocket::pAccept() -> error making a new connection");
                return;
            }
            client->bCanSend   = true;
            client->bConnected = true;
            break;
        case CTL_PING:
            Send.Reset();
            Send.Write(0);
            Send.Write(0);
            Send.Write((char)CTL_PING);
            Send.Write((long)Recv.dwRead());
            FinishCtlPacket(&Send);
            if (nSend(iAcceptSocket, Send.pGetPacketBuffer(), Send.iGetCurSize(), (struct sockaddr *)&ServerAddr) == -1) {
                LogEntry("s_server.cpp -> respond to pingrequest failed");
            }
            return;
        case CTL_REJECT:
            LogEntry("CTL_REJECT ?");  //
            return;
        default:
            reject_connection(iAcceptSocket, (struct sockaddr *)&ServerAddr, "This is a Mantra gameserver");
            LogEntry("UNKNOWN CTL_PACKET (%d) received", cType);
            return;
    }
    // Move client to the front of the list (can get last user logged in information from pFirstPlayer)
    if (pFirstPlayer) {
        client->pNext = pFirstPlayer;  //
    }
    pFirstPlayer = client;
}

/////////////////////////////////////////
void CServer::reject_connection(int iReSocket, sockaddr *pReAddr, const char *reason) {
    CPacket Send(NET_DATAGRAMSIZE);
    if (!reason) return;
    Send.Reset();
    Send.Write(0);
    Send.Write(0);
    Send.Write((char)CTL_REJECT);
    Send.Write((char *)reason);
    FinishCtlPacket(&Send);
    if (nSend(iReSocket, Send.pGetPacketBuffer(), Send.iGetCurSize(), pReAddr) == -1) {
        LogEntry("Reject message failed (%s)", reason);
    }
}

/////////////////////////////////////////
void CServer::send_all(CPacket *pPacket, float fBlockTime) {
    C_GSC *pClient;
    pClient = pFirstPlayer;
    while (pClient) {
        if (!pClient->bDelete) pClient->SendUnreliableMessage(pPacket);
        pClient = pClient->pNext;
    }
}

/////////////////////////////////////////
void CServer::disconnect(C_GSC *client, const char *reason) {
    CPacket Send(NET_DATAGRAMSIZE);
    if (!client) return;
    if (client) {
        Send.Reset();
        Send.Write((char)NM_CLIENT_SHUTDOWN);
        Send.Write((char *)client->session_id);
        Send.Write((char *)reason);
        client->SendUnreliableMessage(&Send);
    }
    Send.Reset();
    Send.Write((char)NM_OTHER_CLIENT_SHUTDOWN);
    Send.Write((char *)client->session_id);
    send_all(&Send, 0);
    client->bDelete = true;
}

/////////////////////////////////////////
void CServer::save_world(void) {
    int         iWorldSaveTimer = atoi((const char *)pCVars->get_cvar("i_world_save_timer"));
    static long dwSaveTimer     = dlcs_get_tickcount();
    if ((dlcs_get_tickcount() - dwSaveTimer) > iWorldSaveTimer) {
        dwSaveTimer = dlcs_get_tickcount();
        LogEntry("World data saved i_world_save_timer=[%d]...", iWorldSaveTimer);
    }
}

/////////////////////////////////////////
void CServer::load_world(void) {
    LogEntry("World data loaded...");  //
}

/////////////////////////////////////////
C_GSC *CServer::get_client(const char *user_name) {
    C_GSC *find = pFirstPlayer;
    while (find) {
        if (dlcs_strcasecmp(find->username, user_name)) return find;
        find = find->pNext;
    }
    return 0;
}

/////////////////////////////////////////
void CServer::kick_all(const char *reason) {
    C_GSC *pClient;
    pClient = pFirstPlayer;
    if (!pClient) return;
    LogEntry("Kicking all online players");
    while (pClient) {
        LogEntry("       ....%s", pClient->username);
        disconnect(pClient, reason);
        pClient = pClient->pNext;
    }
    LogEntry("All players kicked!");
}

/////////////////////////////////////////
void CServer::kick_user(const char *format, ...) {
    dlcsm_make_sizestr(buffer, _DLCS_CONSOLE_ENTRY_SIZE);
    dlcsm_make_textstr(szUserName);
    dlcsm_make_textstr(szReason);
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    // perror(buffer);
    va_end(args);

    // LogEntry("CServer::kick_user(const char *format, ...) %s", buffer);

    dlcsm_explode_list(explode_list);
    explode_list = dlcs_explode(" ", buffer);

    vector<string>::iterator ex_data = explode_list.begin();

    if (explode_list.size() == 1) {
        strcpy(szUserName, ex_data->c_str());
        strcpy(szReason, "Server admin kicked you.");
    } else {
        strcpy(szUserName, ex_data->c_str());
        for (vector<string>::iterator ex_data_2 = explode_list.begin() + 1; ex_data_2 != explode_list.end(); ++ex_data_2) {
            strcat(szReason, va("%s ", ex_data_2->c_str()));
        }
    }

    dlcsm_rtrim(' ', szReason);

    LogEntry("CServer::kick_user: szUserName[%s] szReason[%s]", szUserName, szReason);

    C_GSC *pUser = pFirstPlayer;
    while (pUser) {
        if (dlcs_strcasecmp(pUser->username, szUserName)) {
            disconnect(pUser, szReason);
            LogEntry("CServer::kick_user: Admin kicked user [%s]", szReason);
            return;
        }
        pUser = pUser->pNext;
    }

    LogEntry("CServer::kick_user: %s is not online!", szUserName);
    return;
}

/////////////////////////////////////////
void CServer::add_user(const char *in, u_char ia) {
    // add dlcs_db add player query
    // pSQLite->db_query(va("insert into users (username,access) values ('%s',%d)", in, ia));
}

/////////////////////////////////////////
void CServer::delete_user(const char *szName) {
    // add dlcs_db delete user query
    // pSQLite->db_query("delete * from users where usersname = '%s'", szName);
}

/////////////////////////////////////////
void CServer::set_user_access(const char *in_user_name, int in_access) {
    if (in_access > 255) return;
    if (in_access < 0) return;
    // pSQLite->db_query("update users set access=%d where username='%s'", in_access, in_user_name);
    C_GSC *c = get_client(in_user_name);
    if (c) c->access = in_access;
}

/////////////////////////////////////////
void CServer::chat(C_GSC *pClient, const char *from, const char *msg, int channel) {
    CPacket Send(NET_DATAGRAMSIZE);
    Send.Reset();
    Send.Write((char)NM_MESSAGE);
    Send.Write((char *)msg);
    Send.Write((char *)from);
    Send.Write((int)channel);  // CHANNEL
    if (pClient) Send.Write((char *)pClient->session_id);
    switch (channel) {
        case CHANNEL_CLAN: break;
        case CHANNEL_GENERAL: break;
        case CHANNEL_TRADE: break;
        case CHANNEL_SAY:
        case CHANNEL_YELL:
        case CHANNEL_LOCAL: break;
        case CHANNEL_PARTY: break;
        case CHANNEL_RAID: break;
        case CHANNEL_SYSTEM: send_all((CPacket *)&Send, 0); break;
        case CHANNEL_WHISPER: pClient->SendUnreliableMessage((CPacket *)&Send); break;
        case CHANNEL_CUSTOM: break;
        default: break;
    }
}

/////////////////////////////////////////
void CServer::purge_clients(void) {
    C_GSC *pClient = 0;
    C_GSC *pDelete = 0;
    pClient        = pFirstPlayer;
    if (!pClient) return;
    while (pClient->pNext) {
        if (pClient->pNext->bDelete) {
            pDelete = pClient->pNext;
            if (pClient->pNext->pNext) pClient->pNext = pClient->pNext->pNext;
            DEL(pDelete);
        }
        if (pClient->pNext) pClient = pClient->pNext;
    }
    if (pClient == pFirstPlayer) {
        if (pClient) {
            if (pClient->bDelete) {
                DEL(pFirstPlayer);
            }
        }
    }
}

/////////////////////////////////////////
void CServer::do_world(void) {
    //
}

/////////////////////////////////////////
void CServer::console_command(char *command) {
    vector<string> v = dlcs_explode(" ", command);

    dlcsm_make_sizestr(szTempConsoleString1, _DLCS_CONSOLE_ENTRY_SIZE);
    dlcsm_make_sizestr(szTempConsoleString2, _DLCS_CONSOLE_ENTRY_SIZE);
    dlcsm_make_sizestr(szTargs, _DLCS_CONSOLE_ENTRY_SIZE);

    if (v.size() > 0) {
        strcpy(szTargs, " ");
        if (v.size() > 1) {
            for (unsigned int i = 1; i < v.size(); i++) {
                strcat(szTargs, v[i].c_str());
                strcat(szTargs, " ");
            }
        }

        if (dlcs_strcasecmp(v[0].c_str(), "/help")) {
            LogEntry("[List of available commands]====================================");
            LogEntry("/db..............................Database management");
            LogEntry("/nuy.............................Allow new user accounts");
            LogEntry("/nun.............................Do not allow new user accounts");
            LogEntry("/ann <message>...................Announce a message to the entire server");
            LogEntry("/msg <name> <message>............Send message to specific player");
            LogEntry("/motd [new motd].................View, or change the message of the day");
            LogEntry("/who.............................List online players");
            LogEntry("/users...........................List users from database");
            LogEntry("/add_user <name> [access]........Add new user to database");
            LogEntry("/remove_user <name>..............Remove a user account");
            LogEntry("/kick_user <name> [message]......Kick player with optional message");
            LogEntry("/access <name> <access>..........Change player's access level");
            LogEntry("/kill [minutes]..................Kill the server [minutes] (CTRL-C immediate)");
            LogEntry("/unkill..........................Stop kill countdown");
            LogEntry("/restart [minutes]...............Restart the server [minutes]");
            LogEntry("/unrestart.......................Stop restart countdown");
            LogEntry("/time............................See time / timestamp");
            LogEntry("/ban <user>......................Ban player (will ban user)");
            LogEntry("/banip <user>....................Ban player (will ban user's ip address)");
            LogEntry("/unban <user>....................Unban player (will unban user)");
            LogEntry("/unbanip <ipaddress>.............Unban player (will unban user's ip address)");
            LogEntry("/bandomain <domain>..............Ban entire domain");
            LogEntry("/unbandomain <domain>............Unban entire domain");
            LogEntry("/banlist.........................List all banned ip's");
            LogEntry("======================================================[End List]");
            return;
        }

        ///////////////////////////////////////////////////////////////////////////

        if ((dlcs_strcasecmp(v[0].c_str(), "/db"))) {
            // call dlcs_db functions to manipulate database configuration
            // pSQLite->db_queryl(targs);
        }

        //////////////////////////////////////////

        if ((dlcs_strcasecmp(v[0].c_str(), "/users"))) {
            //  call new CServer class function to return user list
            // pSQLite->db_queryl("select username,access from users");
        }

        //////////////////////////////////////////

        if ((dlcs_strcasecmp(v[0].c_str(), "/add_user"))) {
            if (v.size() > 1) {
                if (v.size() > 2) {
                    add_user((char *)v[1].c_str(), atoi((char *)v[2].c_str()));
                } else {
                    add_user((char *)v[1].c_str(), 0);
                }
            }
        }

        //////////////////////////////////////////

        if (dlcs_strcasecmp(v[0].c_str(), "/delete_user")) {
            if (v.size() > 1) delete_user((char *)v[1].c_str());
        }

        //////////////////////////////////////////

        if (dlcs_strcasecmp(v[0].c_str(), "/set_user_access")) {
            if (v.size() > 2) {
                int dx = atoi(v[2].c_str());  // dx is access
                if ((dx < 0) || (dx > 255)) dx = 0;
                set_user_access((char *)v[1].c_str(), dx);
            }
        }

        //////////////////////////////////////////

        if ((dlcs_strcasecmp(v[0].c_str(), "/time"))) {
            strcpy(szTempConsoleString1, dlcs_get_time());
            LogEntry(szTempConsoleString1);
            dlcs_timestamp(szTempConsoleString1);
            LogEntry("timestamp: %s", szTempConsoleString1);
        }

        //////////////////////////////////////////

        if (dlcs_strcasecmp(v[0].c_str(), "/ann")) {
            if (v.size() > 1) {
                chat(0, "Admin", szTargs, CHANNEL_SYSTEM);
                LogEntry(szTargs);
            }
        }

        //////////////////////////////////////////

        if (dlcs_strcasecmp(v[0].c_str(), "/msg")) {
            if (v.size() > 2) {
                strcpy(szTargs, " ");
                if (v.size() > 2) {
                    for (unsigned int i = 2; i < v.size(); i++) {
                        strcat(szTargs, v[i].c_str());
                        strcat(szTargs, " ");
                    }
                }
                chat(get_client((char *)v[1].c_str()), "Admin", szTargs, CHANNEL_WHISPER);
            }
        }

        //////////////////////////////////////////

        if (dlcs_strcasecmp(v[0].c_str(), "/kick_user")) {
            if (v.size() > 2) {
                strcpy(szTempConsoleString1, "");
                for (int i = 2; i < v.size(); i++) {
                    strcat(szTempConsoleString1, va("%s ", (char *)v[i].c_str()));
                }
                kick_user("%s %s", (char *)v[1].c_str(), szTempConsoleString1);
            }  //
            else if (v.size() > 1) {
                kick_user("%s %s", (char *)v[1].c_str(), "Kicked by admin");
            }
        }

        //////////////////////////////////////////

        if (dlcs_strcasecmp(v[0].c_str(), "/kill")) bQuit = true;

        //////////////////////////////////////////

        if (dlcs_strcasecmp(v[0].c_str(), "/restart")) bRestart = 1;

        //////////////////////////////////////////

        if (dlcs_strcasecmp(v[0].c_str(), "/nuy")) {
            pConsole->pCVars->set_cvar("b_new_accounts", true);
            LogEntry("Now accepting new users");
        }

        //////////////////////////////////////////

        if (dlcs_strcasecmp(v[0].c_str(), "/nun")) {
            pConsole->pCVars->set_cvar("b_new_accounts", false);
            LogEntry("New users will not be accepted");
        }

        //////////////////////////////////////////

        if (dlcs_strcasecmp(v[0].c_str(), "/who")) {
            C_GSC *c = pFirstPlayer;
            while (c) {
                LogEntry("%s", c->username);
                c = c->pNext;
            }
        }

        //////////////////////////////////////////

        if (dlcs_strcasecmp(v[0].c_str(), "/motd")) {
        }

        //////////////////////////////////////////
    }
}
/////////////////////////////////////////
// char *CServer::GetConsoleHistory(int x) {
// if (SConsoleHistory) return (SConsoleHistory[x].text);
// return (char *)(va("con history error![%d]", x));
//}
/////////////////////////////////////////
// void CServer::SetupConsoleHistory(void) {
// SConsoleHistory = 0;
// SConsoleHistory = new stConHistory[MAX_CONSOLE_INPUT_BUFFER + 1];
// for (int i = 0; i < MAX_CONSOLE_INPUT_BUFFER; i++) memset(SConsoleHistory[i].text, 0, _DLCS_CONSOLE_ENTRY_SIZE);
// iConsoleHistoryPosition = 0;
//}
/////////////////////////////////////////
// void CServer::AddToConsoleHistory(char *s) {
// TODO: Access pConsole (add history)
// for (int i = MAX_CONSOLE_INPUT_BUFFER; i > 0; i--) strcpy(SConsoleHistory[i].text, SConsoleHistory[i - 1].text);
// strcpy(SConsoleHistory[0].text, s);
//}
/////////////////////////////////////////
// void CServer::RemoveConsoleHistory(void) {
// delete[] SConsoleHistory;
// SConsoleHistory = 0;
//}

/////////////////////////////////////////
bool CServer::save_data(void) {
    pCVars->bSave("server.ini");
    return true;
}

/////////////////////////////////////////
int CServer::num_clients() {
    int    x = 0;
    C_GSC *c = pFirstPlayer;
    while (c) {
        x++;
        c = c->pNext;
    }
    return x;
}

/////////////////////////////////////////
void CServer::create_guid(const char *cin, char *cout) {
    char t[1024];
    memset(t, 0, 1024);
    char t2[1024];
    memset(t2, 0, 1024);
    strcpy(t, va("%s%d", cin, rand() % 99999));
    strcpy(t2, dlcs_md5_digest(t).c_str());
    dlcs_timestamp(t);
    strcpy(cout, va("%s-%s-%05d", t2, t, rand() % 99999));
}
/////////////////////////////////////////
/*
void CServer::ReportToMaster(void){
static bool starter=false; static long dwReportTimer=GetTickCount();
 //if(!dlcs_strcasecmp(gmvar("sys.master_report"),"yes")) return;
     if(!pFMMS_Connection) return;
if((GetTickCount()-dwReportTimer)<15000) {
    if(!starter) starter=true; else return;
}
dwReportTimer=GetTickCount(); //LogEntry("Reported to master");
pFMMS_Connection->Connect("127.0.0.1","40404");
}
*/
