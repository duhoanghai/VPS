#ifndef IPNETWORK_H
#define IPNETWORK_H
#include "net/IPAddress.h"
class IPNetwork {
public:
    typedef IPAddress Server;
    typedef IPAddress Client;
    virtual ~IPNetwork() {}
    virtual ssize_t request(const Server& server,void* req,size_t reqlen,void* res,size_t reslen,int timeout=5)=0;
    virtual ssize_t receive(Client& client,void* req,size_t reqlen)=0;
    virtual ssize_t reply(const Client& client,void* res,size_t reslen)=0;
    virtual bool isConnectionOriented() const=0;
protected:
    int sockfd=-1;
};
#endif
