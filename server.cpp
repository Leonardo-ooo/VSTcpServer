#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <signal.h>
#include <vector>
#include <sys/stat.h>
#include <unordered_map>

#include "TcpServer.h"
#include "DBC.h"
#include "rdata.h"

constexpr auto SOCK_SIZE = 1024;
constexpr auto LOG_SIZE = 2;

constexpr auto LOGIN = 1;
constexpr auto MESSAGE = 2;
constexpr auto GETICON = 3;
constexpr auto RMESSAGE = 4;
constexpr auto GMESSAGE = 5;

using namespace std;
unordered_map<string, int>getact = {
{"login", LOGIN },
{"message", MESSAGE},
{"geticon", GETICON},
{"rmessage", RMESSAGE},
{"gmessage", GMESSAGE}
};

TcpServer server;
DBC db;
int epollfd;

unordered_map<int, int>sockmp;        //      key: uid,  value: sock
int sockuid[SOCK_SIZE + 1];         //      get the uid by sock
void signal_init();
void EXIT(int);
void login(Rdata trdata, struct epoll_event evts);
void posticon(int sock, string uid);
void getidlist(vector<string>& idlist, string chatroom);
void xmlWrite(int sock, Rdata& xmlf, int op);
void sendmessage(Rdata& trdata, int sender);
void sendrmessage(Rdata& trdata, int sender);
void sendgmessage(Rdata& trdata, int sender);

struct epoll_event& event(int fd, __uint32_t events)
{
    static epoll_event a;
    memset(&a, 0, sizeof a);
    a.data.fd = fd;
    a.events = events;

    return a;
};

void db_init(DBC& tdb)
{
    const char* host = "localhost";
    const char* user = "root";
    const char* pwd = "max.2021";
    const char* dbname = "server";
    unsigned int dbport = 3306;
    if (tdb.Connect(host, user, pwd, dbname, dbport) == false)
    {
        const char* pwd1 = "Zw0727@qq.cn";
        if (tdb.Connect(host, user, pwd1, dbname, dbport) == false)
        {
            cerr << "data base connect error! " << endl;
            exit(-1);
        }
    }
}

int ntohi(char* buffer)
{
    int res = 1;
    char c;
    memcpy(&c, &res, 1);
    if (!c) {
        swap(buffer[0], buffer[3]);
        swap(buffer[1], buffer[2]);
    }
    memcpy(&res, buffer, 4);
    return res;
}

void htoni(int size, char* buffer)
{
    int i = 1;
    memcpy(buffer, &i, 1);
    if (buffer[0]) memcpy(buffer, &size, 4);
    else {
        memcpy(buffer, &size, 4);
        swap(buffer[0], buffer[3]);
        swap(buffer[1], buffer[2]);
    }
}

string getname(string uid)
{
    DBC tdb;
    db_init(tdb);
    tdb.query("select uname from account where uid=" + uid);
    tdb.nextline();
    return tdb.row[0];
}

int main(int argc, char* argv[])
{
    if (argc > 2)
    {
        cout << "argment wrong ! expected one argment port !" << endl;
        return -1;
    }


    //      set port

    unsigned short port = 8011;
    if (argc == 2) port = (unsigned short)atoi(argv[1]);
    if (!port) { cout << "wrong port!" << endl; return -1; }


    //      TcpServer init

    server.InitServer(port);
    cout << "setver on ! listenfd = " << server.listenfd << endl;


    //      init signal and connect to mysql database
    signal_init();
    db_init(db);
    memset(sockuid, -1, sizeof sockuid);


    //      open a epoll sock and add the listen sock to epoll

    epollfd = epoll_create(1);
    struct epoll_event evts[SOCK_SIZE];
    epoll_ctl(epollfd, EPOLL_CTL_ADD, server.listenfd, &event(server.listenfd, EPOLLIN));


    //       epoll_wait 

    while (true)
    {
        int infds = epoll_wait(epollfd, evts, SOCK_SIZE, -1);
        if (infds < 0) { cerr << "epoll_wait() failed !\n"; return -1; }
        if (infds == 0)
        {
            //      timeout
        }


        //      get events

        for (int i = 0; i < infds; i++)
        {
            //      a new tcp connect

            if (evts[i].data.fd == server.listenfd)
            {
                if (!server.Accept()) { cout << "accept error! \n"; continue; }
                cout << server.GetIP() << "  connect, sock = " << server.clientfd << endl;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, server.clientfd, &event(server.clientfd, EPOLLIN));

            }
            else
            {
                //      a client disconnect

                if (server.Read(evts[i].data.fd, server.buffer, 4) <= 0) {
                    cout << evts[i].data.fd << " sock close" << endl;
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, evts[i].data.fd, &evts[i]);
                    close(evts[i].data.fd);
                    int uid = sockuid[evts[i].data.fd];
                    if (uid >= 0)
                    {
                        sockuid[evts[i].data.fd] = -1;
                        sockmp.erase(sockmp.find(uid));
                    }
                    continue;
                }

                //      deal the receive xml

                int rlen = ntohi(server.buffer), st = 0;
                cout << "recv size = " << rlen << endl;
                cout << "test char" << (unsigned int)server.buffer[0] << " " << (unsigned int)server.buffer[1] << " " << (unsigned int)server.buffer[2] << " " << (unsigned int)server.buffer[3] << endl;

                while (rlen > st)
                {
                    int tmp = server.Read(evts[i].data.fd, server.buffer + st, rlen - st);
                    if (tmp <= 0) {
                        cout << "read error!" << endl;
                        break;
                    }
                    cout << "read size=" << tmp << endl;
                    st += tmp;
                }

                cout << "recv xml : " << server.buffer << endl;
                Rdata trdata;
                trdata.xmlparse(string(server.buffer));
                switch (getact[trdata.action])
                {
                case LOGIN:
                    cout << "do login()\n";
                    login(trdata, evts[i]);
                    break;


                case GETICON:
                    cout << "do geticon()" << endl;
                    posticon(evts[i].data.fd, trdata.uid);
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, evts[i].data.fd, &evts[i]);
                    break;

                case MESSAGE:
                    cout << "do sendmessage()" << endl;
                    sendmessage(trdata, sockuid[evts[i].data.fd]);
                    break;

                case RMESSAGE:
                    cout << "do sendrmessage()" << endl;
                    sendmessage(trdata, sockuid[evts[i].data.fd]);
                    break;

                case GMESSAGE:
                    cout << "do sendgmessage()" << endl;
                    sendgmessage(trdata, sockuid[evts[i].data.fd]);
                    break;

                default:
                    break;
                }
            }
        }

    }

    return 0;
}

//      there are three steps for user's login
        //      logincheck: check the account and password return a status
        //      loginsend:  send the user's information such as uid, uname, friendslist, roomlist
        //      chat_log send: first send room chat_log, than send friend chat_log, over act mark the end

void login(Rdata trdata, struct epoll_event ev)
{
    Rdata logincheck("logincheck");
    db.query("select upassword, uid, uname, friends, chatroom from account where uaccount='" + trdata.account + "'");
    if (!db.row_num) logincheck.content = "0";
    else
    {
        db.nextline();
        if (db.row[0] == trdata.password)
        {
            logincheck.content = "1";
            int uid = atoi(db.row[1]);
            if (sockmp.find(uid) != sockmp.end())
            {
                //      logout the same account

                int& tfd = sockmp[uid];
                cout << "have the same account online , use sock = " << tfd << endl;
                Rdata tt("logout", "1");
                xmlWrite(ev.data.fd, tt, ACT_CONTENT);
                epoll_ctl(epollfd, EPOLL_CTL_DEL, tfd, &ev);
                close(tfd);
                sockuid[tfd] = -1;
                tfd = ev.data.fd;
            }
            else sockmp[uid] = ev.data.fd;
            cout << "password correct " << endl;
            sockuid[ev.data.fd] = uid;
        }
        else logincheck.content = "0";
        if (db.row[1]) logincheck.uid = db.row[1];
        cout << "logincheck = " << logincheck.content << endl;
    }
    xmlWrite(ev.data.fd, logincheck, LOGIN_CHECK);
    cout << "send size: " << logincheck.len << "  send : " << server.buffer + 4 << endl;
    cout << "size char:" << (unsigned int)server.buffer[0] << " " << (unsigned int)server.buffer[1] << " " << (unsigned int)server.buffer[2] << " " << (unsigned int)server.buffer[3] << endl;

    if (logincheck.content == "0") return;

    //      send the uname, friend list, chatroom list to the user

    Rdata loginsend("loginsend");
    if (db.row[2]) loginsend.uname = db.row[2];
    if (db.row[3]) loginsend.friends = db.row[3];
    if (db.row[4]) loginsend.chatroom = db.row[4];
    xmlWrite(ev.data.fd, loginsend, LOGIN_SEND);
    cout << "size: " << loginsend.len << " send :" << server.buffer + 4 << endl;


    //      send the chat_log of chatroom
    string qs = "select log, rid from room_log where rid=0";
    vector<string>idlist;
    getidlist(idlist, loginsend.chatroom);
    for (auto& i : idlist)    qs += " or rid=" + i;
    cout << "qs=  " << qs << endl;
    db.query(qs);
    cout << "query chat room num=" << db.row_num << endl;
    for (int i = 0; i < db.row_num; i++)
    {
        db.nextline();
        Rdata rlogsend("rlogsend", db.row[0]);
        rlogsend.uid = db.row[1];
        xmlWrite(ev.data.fd, rlogsend, RLOG_SEND);
        cout << "send room_log:  " << server.buffer + 4 << endl;
    }


    //      send the chat_log of friends

    db.query("select log, a, b from chat_log where a=" + logincheck.uid + " or b=" + logincheck.uid);

    for (int i = 0; i < db.row_num; i++) {
        db.nextline();
        Rdata ulogsend("ulogsend", db.row[0]);
        if (db.row[1] != logincheck.uid) ulogsend.uid = db.row[1];
        else ulogsend.uid = db.row[2];
        xmlWrite(ev.data.fd, ulogsend, ULOG_SEND);
        cout << "send chat_log : " << server.buffer + 4 << endl;
    }
    Rdata tt("over", "1");
    xmlWrite(ev.data.fd, tt, ACT_CONTENT);
}


//      send a chat message to a corresponding friend, and save the chat log in mysql database 

void sendmessage(Rdata& trdata, int sender)
{
    //      target user is online, send message to the user    

    int tuid = atoi(trdata.uid.c_str());
    if (sockmp.find(tuid) != sockmp.end())
    {
        Rdata tt("message", trdata.content);
        db.nextline();
        tt.uid = to_string(sender);
        xmlWrite(sockmp[tuid], tt, MESSAGE_SEND);
        cout << "send :" << server.buffer + 4 << endl;
    }

    //      update the chat_log
    DBC tdb;
    db_init(tdb);
    tdb.query("select cnt, log from chat_log where (a=" + trdata.uid + " and b=" + to_string(sender) + ") or (b=" + trdata.uid + " and a=" + to_string(sender) + ")");
    if (tdb.row_num == 0)
    {
        cout << "create a new chat log\n";
        tdb.query("insert into chat_log (a, b, log) values(" + trdata.uid + ", " + to_string(sender) + ", '" + trdata.content + ";')");
    }
    else
    {
        tdb.nextline();
        string log = tdb.row[1];
        cout << "old log: " << log << endl;
        if (atoi(tdb.row[0]) == LOG_SIZE)
        {
            int st = 0, tsize = (int)log.size();
            while (log[st] != ';') st++;
            st++;
            log = log.substr(st, tsize - st);
        }
        else
        {
            tdb.query("update chat_log set cnt=cnt+1 where (a=" + trdata.uid + " and b=" + to_string(sender) + ") or (b=" + trdata.uid + " and a=" + to_string(sender) + ")");
        }

        string ts = getname(to_string(sender)) + ":";
        ts.append(trdata.content);
        ts.append(";");
        log.append(ts);

        tdb.query("update chat_log set log='" + log + "' where (a=" + trdata.uid + " and b=" + to_string(sender) + ") or (b=" + trdata.uid + " and a=" + to_string(sender) + ")");
        cout << "message:" << trdata.content << endl << "updatelog: " << log << endl;
    }
}

//      send a chat message to a corresponding chat room, and save the chat_log in mysql database

void sendrmessage(Rdata& trdata, int sender)
{
    string uname = getname(to_string(sender));
    DBC tdb;
    db_init(tdb);
    tdb.query("select cnt, members, log from room_log where rid=" + trdata.uid);
    tdb.nextline();
    vector<string>uidlist;
    getidlist(uidlist, tdb.row[1]);

    //      send the chat message to online members

    Rdata tt("rmessage", trdata.content);
    tt.uname = uname;
    for (auto& s : uidlist)
    {
        int tuid = atoi(s.c_str());
        tt.uid = s;
        if (sockmp.find(tuid) != sockmp.end())
        {
            xmlWrite(tuid, tt, MESSAGE_SEND);
        }
    }

    //      update room chat_log

    string log = tdb.row[2];
    if (atoi(tdb.row[0]) == LOG_SIZE)
    {
        int st = 0, tsize = (int)log.size();
        while (log[st] != ';') st++;
        st++;
        log = log.substr(st, tsize - st);
    }
    else
    {
        tdb.query("update room_log set cnt=cnt+1 where rid=" + trdata.uid);
    }
    log.append(tt.uname);
    log.append(":");
    log.append(tt.content);
    log.append(";");
    tdb.query("update room_log set log=" + log + " where rid=" + trdata.uid);
    cout << "cur:" << trdata.content << endl << "updatelog: " << log << endl;

}

//      send the global message

void sendgmessage(Rdata& trdata, int sender)
{
    DBC tdb;
    db_init(tdb);
    tdb.query("select uname from account where id=" + to_string(sender));
    Rdata tt("gmessage", trdata.content);
    tt.uname = tdb.row[0];
    for (auto& i : sockmp) {
        xmlWrite(i.second, tt, MESSAGE_SEND);
    }

}
//      send message with the own protocol

void xmlWrite(int sock, Rdata& xmlf, int op)
{
    xmlf.stoxml(op, server.buffer + 4);
    htoni(xmlf.len, server.buffer);
    server.Write(sock, server.buffer, xmlf.len + 4);
}


//      get the uid or rid list return vector<string>idlist

void getidlist(vector<string>& idlist, string memberlist)
{
    string t = "";
    for (auto& i : memberlist)
    {
        if (i == ':') {
            idlist.push_back(t);
            t = "";
        }
        else if (i == ';') t = "";
        else t += i;
    }

}

//      function to init the signal

void posticon(int sock, string uid)
{
    db.query("select type, path from user_icon where uid =" + uid);
    if (!db.row_num) {
        cout << "no icon!\n";
        close(sock);
        return;
    }
    db.nextline();
    string path = db.row[1] + uid + db.row[0];
    FILE* fd = fopen(path.c_str(), "r");
    char* tbuffer = (char*)malloc(65535);

    struct stat statbuf;
    stat(path.c_str(), &statbuf);

    memcpy(tbuffer, &statbuf.st_size, 4);
    server.Write(sock, tbuffer, 4);
    int t;
    while ((t = (int)fread(tbuffer, 1, 65535, fd)) > 0)
    {
        server.Write(sock, tbuffer, t);
    }
    fclose(fd);
    close(sock);
    delete tbuffer;

}

void signal_init()
{
    for (int i = 1; i <= 64; i++)signal(i, SIG_IGN);

    struct sigaction sact;
    memset(&sact, 0, sizeof sact);
    sact.sa_handler = EXIT;
    sigaction(SIGINT, &sact, NULL);
    sigaction(SIGTERM, &sact, NULL);
}


//      function to exit

void EXIT(int sig)
{
    cout << "recv the " << sig << " signal exit!" << endl;
    exit(0);
}
