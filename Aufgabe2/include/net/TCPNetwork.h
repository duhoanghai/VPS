#ifndef TCPNETWORK_H
#define TCPNETWORK_H
#include "net/IPNetwork.h"
class TCPNetwork : public IPNetwork {
public:
    explicit TCPNetwork(short port);
    TCPNetwork();
    ~TCPNetwork() override;
    ssize_t request(const Server& server,void* req,size_t reqlen,void* res,size_t reslen,int timeout=5) override;
    ssize_t receive(Client& client,void* req,size_t reqlen) override;
    ssize_t reply(const Client& client,void* res,size_t reslen) override;
    bool isConnectionOriented() const override { return true; }
private:
    int connection_=-1;  // Genau eine akzeptierte Verbindung pro Anfrage.
};
#endif
