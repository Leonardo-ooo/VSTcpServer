#include "DBC.h"


DBC::DBC()
{
    res = NULL;
    field = NULL;
    row = NULL;
    row_num = field_num = 0;
}

DBC::DBC(const char* host, const char* user, const char* pwd, const char* dbname, unsigned int port)
{
    res = NULL;
    field = NULL;
    row = NULL;
    mysql_init(conn);
    if (!conn) { std::cerr << "init error! \n"; }
    conn = mysql_real_connect(conn, host, user, pwd, dbname, port, NULL, 0);
    if (!conn) { std::cerr << "connect error! \n"; }

}

DBC::~DBC()
{
    if (conn) mysql_close(conn);
}

bool DBC::Connect(const char* host, const char* user, const char* pwd, const char* dbname, unsigned int port)
{
    conn = new MYSQL;
    mysql_init(conn);
    conn = mysql_real_connect(conn, host, user, pwd, dbname, port, NULL, 0);
    if (!conn) return false;
    mysql_query(conn, "set names utf8");
    return true;
}

bool DBC::query(std::string sqlstr)
{
    const char* s = sqlstr.data();
    if (mysql_query(conn, s)) return false;
    res = mysql_store_result(conn);
    if (!res) return false;

    row_num = (int)mysql_num_rows(res);
    field_num = mysql_num_fields(res);
    return true;
}

char* DBC::getfieldname(int index)
{
    field = mysql_fetch_field_direct(res, index);
    return field->name;
}

bool DBC::nextline()
{
    row = mysql_fetch_row(res);
    if (!row) return false;
    return true;
}

