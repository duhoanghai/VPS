#include "net/UDPNetwork.h"
#include "net/Protocol.h"
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {
int udpSocket() {
    int fd=::socket(AF_INET,SOCK_DGRAM,0);
    if(fd<0)throw std::runtime_error(std::string("UDP socket: ")+std::strerror(errno));
    return fd;
}
}
UDPNetwork::UDPNetwork(short port) {
    sockfd=udpSocket(); IPAddress addr(port);
    if(::bind(sockfd,reinterpret_cast<const sockaddr*>(&addr),sizeof(addr))<0)
        throw std::runtime_error(std::string("UDP bind: ")+std::strerror(errno));
}
UDPNetwork::UDPNetwork() { sockfd=udpSocket(); }
UDPNetwork::~UDPNetwork() { if(sockfd>=0)::close(sockfd); }
ssize_t UDPNetwork::request(const Server& server,void* req,size_t reqlen,void* res,size_t reslen,int timeout) {
    if(reqlen!=sizeof(Message)||reslen<sizeof(Message))return -1;
    const Message* sent=static_cast<const Message*>(req);
    // Die Antwort derselben Adresse und Sequenz zuordnen; alte UDP-Pakete verwerfen.
    for(int attempt=0;attempt<60;++attempt) {
        if(::sendto(sockfd,req,reqlen,0,reinterpret_cast<const sockaddr*>(&server),sizeof(server))!=(ssize_t)reqlen)return -1;
        for(;;) {
            fd_set fds; FD_ZERO(&fds);FD_SET(sockfd,&fds);
            timeval tv{timeout,0};int ready=::select(sockfd+1,&fds,nullptr,nullptr,&tv);
            if(ready==0)break; // Timeout -> gleiche Nachricht erneut senden.
            if(ready<0){if(errno==EINTR)continue;return -1;}
            IPAddress from; socklen_t fromlen=sizeof(from);
            const ssize_t n=::recvfrom(sockfd,res,reslen,0,reinterpret_cast<sockaddr*>(&from),&fromlen);
            if(n!=(ssize_t)sizeof(Message))continue;
            const Message* received=static_cast<const Message*>(res);
            if(from==server && received->type==RESPONSE && received->sequence==sent->sequence)return n;
        }
    }
    return -1;
}
ssize_t UDPNetwork::receive(Client& client,void* req,size_t reqlen) {
    socklen_t len=sizeof(client);
    return ::recvfrom(sockfd,req,reqlen,0,reinterpret_cast<sockaddr*>(&client),&len);
}
ssize_t UDPNetwork::reply(const Client& client,void* res,size_t reslen) {
    return ::sendto(sockfd,res,reslen,0,reinterpret_cast<const sockaddr*>(&client),sizeof(client));
}
