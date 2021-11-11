#ifndef RDATA_H
#define RDATA_H

#include <iostream>
#include <cstring>

#define BEATS           0
#define LOGIN_CHECK     1
#define ACT_CONTENT     2
#define LOGIN_SEND      3
#define ULOG_SEND       4
#define RLOG_SEND       5
#define MESSAGE_SEND    6

class Rdata
{
public:
    Rdata();
    Rdata(std::string action, std::string content = "");

public:
    int len;
    std::string action;
    std::string content;
    std::string uid;
    std::string account;
    std::string password;
    std::string uname;
    std::string friends;
    std::string chatroom;

public:
    bool stoxml(int op, char *buffer);
    bool xmlparse(std::string data);
};

#endif // RDATA_H
