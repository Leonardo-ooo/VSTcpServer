#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <malloc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <fcntl.h>

constexpr auto LISTEN_LOG = 5;
constexpr auto BUFFER_SIZE = 5242880;
class TcpServer
{
public:
    
    int listenfd;       //      listen socket
    int clientfd;       //      client socket
    char *buffer; 
    
    TcpServer();                
    ~TcpServer();

        //  init server, set port
    bool InitServer(const unsigned short port);

        //  wait a connection
    bool Accept();

        //  get user`s ip address
    char* GetIP();
    
    static int Write(int fd, char *buffer, int size = 0);
    static int Read(int fd, char *buffer, int len);

//private:
    int cliaddr_len;
    struct sockaddr_in cliaddr;
    struct sockaddr_in seraddr;

};

#endif
