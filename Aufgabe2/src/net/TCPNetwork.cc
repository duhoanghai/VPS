#include "net/TCPNetwork.h"
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace {
bool allBytes(int fd,void* data,size_t length,bool sending) {
    char* bytes=static_cast<char*>(data);size_t done=0;
    while(done<length) {
        ssize_t n=sending?::send(fd,bytes+done,length-done,0) : ::recv(fd,bytes+done,length-done,0);
        if(n<0&&errno==EINTR)continue;
        if(n<=0)return false;
        done+=static_cast<size_t>(n);
    }
    return true;
}
int tcpSocket() {
    const int fd=::socket(AF_INET,SOCK_STREAM,0);
    if(fd<0)throw std::runtime_error(std::string("TCP socket: ")+std::strerror(errno));
    return fd;
}
}
TCPNetwork::TCPNetwork(short port) {
    sockfd=tcpSocket();int yes=1;::setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
    IPAddress addr(port);
    if(::bind(sockfd,reinterpret_cast<const sockaddr*>(&addr),sizeof(addr))<0 || ::listen(sockfd,16)<0)
        throw std::runtime_error(std::string("TCP bind/listen: ")+std::strerror(errno));
}
TCPNetwork::TCPNetwork() {} // Client baut fuer jede Anfrage einen frischen Socket auf.
TCPNetwork::~TCPNetwork() {if(connection_>=0)::close(connection_);if(sockfd>=0)::close(sockfd);}
ssize_t TCPNetwork::request(const Server& server,void* req,size_t reqlen,void* res,size_t reslen,int) {
    int fd=tcpSocket();
    if(::connect(fd,reinterpret_cast<const sockaddr*>(&server),sizeof(server))<0){::close(fd);return -1;}
    const bool ok=allBytes(fd,req,reqlen,true)&&allBytes(fd,res,reslen,false);
    ::close(fd); // Verbindung nach jeder Antwort wieder abbauen.
    return ok?static_cast<ssize_t>(reslen):-1;
}
ssize_t TCPNetwork::receive(Client& client,void* req,size_t reqlen) {
    socklen_t addrlen=sizeof(client);
    connection_=::accept(sockfd,reinterpret_cast<sockaddr*>(&client),&addrlen);
    if(connection_<0)return -1;
    if(!allBytes(connection_,req,reqlen,false)){::close(connection_);connection_=-1;return -1;}
    return static_cast<ssize_t>(reqlen);
}
ssize_t TCPNetwork::reply(const Client&,void* res,size_t reslen) {
    if(connection_<0)return -1;
    const bool ok=allBytes(connection_,res,reslen,true);
    ::close(connection_);connection_=-1;
    return ok?static_cast<ssize_t>(reslen):-1;
}
