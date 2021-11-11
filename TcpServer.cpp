#include "TcpServer.h"

TcpServer::TcpServer()
{
    listenfd = clientfd = -1;
    buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE);
}

TcpServer::~TcpServer()
{
    close(clientfd);
    close(listenfd);
}

bool TcpServer::InitServer(const unsigned short port)
{
    if(listenfd > 0) {close(listenfd); listenfd = -1;}


        //      create a server listen socket

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0) return false;
        
        //      set SO_REUSEADDR (solve addr already in use)

        //      Linux
    int opt = 1; unsigned int len = sizeof opt;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, len);

        //      Windows
    // char b_opt = "1";
    // setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &b_opt, sizeof b_opt);


    
        //      set protocl address to server

    memset(&seraddr, 0, sizeof seraddr);
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(port);
    seraddr.sin_addr.s_addr = htonl(INADDR_ANY);


        //      bind the address to server socket

    if(bind(listenfd, (struct sockaddr*) &seraddr, sizeof seraddr))
    {
        close(listenfd);
        return false;
    }
        

        //      set listen mod

    listen(listenfd, LISTEN_LOG);
    return true;
}

bool TcpServer::Accept()
{
    if(listenfd == -1) return false;


        //      create a client socket

    clientfd = accept(listenfd, (struct sockaddr*) &cliaddr,(socklen_t*) &cliaddr_len);
    if(clientfd < 0) return false;
    return true;

}

char* TcpServer::GetIP()
{
    if(clientfd > 0)
        return inet_ntoa(cliaddr.sin_addr);
    else
        return (char*) "not connect! \n";
}

int TcpServer::Write(int fd, char *buffer, int size)
{
    if(!size) size = strlen(buffer);
    return send(fd, buffer, size, 0);
}

int TcpServer::Read(int fd,char *buffer, int len)
{   
    int res =  recv(fd, buffer, len, 0);
    buffer[res] = 0;
    return res;
}
