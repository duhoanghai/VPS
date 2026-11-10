#include "client/LifeClient.h"
#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>
#include <thread>
#include <unistd.h>

LifeClient::LifeClient(IPNetwork* network,const char* host,short port)
    : server_(host,port),net_(network) {
    if(!net_)throw std::invalid_argument("Network ist null");
    std::random_device device;
    token_=device() ^ (static_cast<uint32_t>(::getpid())*2654435761u);
    if(token_==0)token_=1;
}
Message LifeClient::exchange(Message req) {
    req.sequence=++sequence_;req.token=token_;req.clientId=id_;req.timestep=timestep_;
    Message response{};
    if(net_->request(server_,&req,sizeof(req),&response,sizeof(response),1)!=static_cast<ssize_t>(sizeof(response)) ||
       response.type!=RESPONSE||response.sequence!=req.sequence||response.status==FAILURE)
        throw std::runtime_error("Netzwerkanfrage fehlgeschlagen (Typ "+std::to_string(req.type)+")");
    return response;
}
int LifeClient::startUp() {
    Message req;req.type=LOGON;
    const Message reply=exchange(req);
    id_=reply.clientId;x1_=reply.x1;x2_=reply.x2;y1_=reply.y1;y2_=reply.y2;
    width_=reply.width;height_=reply.height;
    if(width_<=0||height_<=0||x1_<0||x2_>width_||y2_>height_)return -1;
    std::cout<<"Client "<<id_<<": Bereich x=["<<x1_<<","<<x2_<<"), y=["<<y1_<<","<<y2_<<")\n";
    return 0;
}
bool LifeClient::aliveAt(int x,int y) {
    // Toroidales Spielfeld wie in Aufgabe 1 (Wrap-around an den Raendern).
    x=(x+width_)%width_;y=(y+height_)%height_;
    Message req;req.type=READ;req.x=x;req.y=y;
    return exchange(req).value==1;
}
void LifeClient::makeStep() {
    for(int y=y1_;y<y2_;++y)for(int x=x1_;x<x2_;++x) {
        int neighbors=0;
        for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)
            if(dx!=0||dy!=0)neighbors+=aliveAt(x+dx,y+dy);
        const bool current=aliveAt(x,y);
        const bool next=neighbors==3||(neighbors==2&&current);
        Message write;write.type=WRITE;write.x=x;write.y=y;write.value=next?1:0;
        exchange(write);
    }
}
void LifeClient::loop() {
    for(;;) {
        makeStep();
        Message barrier;barrier.type=BARRIER;
        for(;;) {
            const Message reply=exchange(barrier);
            if(reply.status==WAIT) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            ++timestep_;
            if(reply.status==FINISHED) {
                Message quit;quit.type=QUIT;
                exchange(quit);
                std::cout<<"Client "<<id_<<" fertig: "<<timestep_<<" Schritte\n";
                return;
            }
            break;
        }
    }
}
void LifeClient::run() {if(startUp()<0)throw std::runtime_error("Logon fehlgeschlagen");loop();}
