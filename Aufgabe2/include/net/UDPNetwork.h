#ifndef UDPNETWORK_H
#define UDPNETWORK_H
#include "net/IPNetwork.h"
// Oeffentliche Vererbung ist notwendig, damit UDPNetwork als IPNetwork nutzbar ist.
class UDPNetwork : public IPNetwork {
public:
    explicit UDPNetwork(short port);
    UDPNetwork();
    ~UDPNetwork() override;
    ssize_t request(const Server& server,void* req,size_t reqlen,void* res,size_t reslen,int timeout=5) override;
    ssize_t receive(Client& client,void* req,size_t reqlen) override;
    ssize_t reply(const Client& client,void* res,size_t reslen) override;
    bool isConnectionOriented() const override { return false; }
};
#endif
