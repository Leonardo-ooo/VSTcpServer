#ifndef RDATA_H
#define RDATA_H

#include <iostream>
#include <cstring>

constexpr auto BEATS = 0;
constexpr auto LOGIN_CHECK = 1;
constexpr auto ACT_CONTENT = 2;
constexpr auto LOGIN_SEND = 3;
constexpr auto ULOG_SEND = 4;
constexpr auto RLOG_SEND = 5;
constexpr auto MESSAGE_SEND = 6;

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
