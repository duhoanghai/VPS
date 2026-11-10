#ifndef LIFECLIENT_H
#define LIFECLIENT_H
#include "net/IPAddress.h"
#include "net/IPNetwork.h"
#include "net/Protocol.h"
#include <cstdint>

class LifeClient {
public:
    LifeClient(IPNetwork* network,const char* servername,short port);
    void run();
private:
    int x1_=0,x2_=0,y1_=0,y2_=0,width_=0,height_=0;
    IPAddress server_;
    IPNetwork* net_;
    uint32_t token_=0,id_=0,sequence_=0,timestep_=0;
    Message exchange(Message request);
    int startUp();
    void loop();
    void makeStep();
    bool aliveAt(int x,int y);
};
#endif
