#ifndef MYSQLDBC_H
#define MYSQLDBC_H

#include <mysql/mysql.h>
#include <iostream>
#include <cstring>

class DBC{
public:

    int row_num;
    int field_num;

    DBC();
    DBC(const char *host, const  char *user, const  char *pwd, const char* dbname, unsigned int port = 3306);
    ~DBC();

    bool Connect(const char *host, const char *user, const char *pwd,const char* dbname,unsigned int port = 3306);

    bool query(std::string sqlstr);

    char* getfieldname(int index);

    bool nextline();
//private:

    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    MYSQL_FIELD *field;

};

#endif
