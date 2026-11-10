#include "board/BoardServer.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

BoardServer::BoardServer(IPNetwork* network,int clientcount,Board* board_a,Board* board_b,int timesteps)
    : net_(network),current_(board_a),next_(board_b),expected_(clientcount),maxsteps_(timesteps) {
    if(!net_||!current_||!next_||current_==next_||clientcount<=0||timesteps<0)
        throw std::invalid_argument("Ungueltige Server-Parameter");
    size_t w2=0,h2=0;
    current_->getDimension(width_,height_);next_->getDimension(w2,h2);
    if(!width_||!height_||width_!=w2||height_!=h2)
        throw std::invalid_argument("Beide Boards muessen gleich gross sein");
}
int BoardServer::findClient(uint32_t token) const {
    for(size_t i=0;i<clients_.size();++i)if(clients_[i].token==token)return static_cast<int>(i);
    return -1;
}
bool BoardServer::allArrived() const {
    if(static_cast<int>(clients_.size())!=expected_)return false;
    for(const auto& client:clients_)if(!client.arrived)return false;
    return true;
}
bool BoardServer::allQuit() const {
    if(static_cast<int>(clients_.size())!=expected_)return false;
    for(const auto& client:clients_)if(!client.quit)return false;
    return true;
}
void BoardServer::notifyAll(ResultCode result) {
    // UDP: Antworten erst nach Eintreffen ALLER Clients senden.
    for(auto& client:clients_) {
        Message reply=client.pending;
        reply.type=RESPONSE;reply.status=result;
        net_->reply(client.address,&reply,sizeof(reply));
    }
}
bool BoardServer::process(const IPAddress& sender,const Message& req,Message& res,bool& shouldReply) {
    res=req;res.type=RESPONSE;res.status=OK;shouldReply=true;
    if(req.type==LOGON) {
        if(!req.token){res.status=FAILURE;return false;}
        int id=findClient(req.token);
        if(id<0){
            if(static_cast<int>(clients_.size())>=expected_){res.status=FAILURE;return false;}
            ClientInfo info;info.token=req.token;info.address=sender;
            clients_.push_back(info);id=static_cast<int>(clients_.size())-1;
            std::cerr<<"Client "<<id<<" angemeldet ("<<clients_.size()<<"/"<<expected_<<")\n";
        }
        // Aufteilung nach Spalten; x2 und y2 sind EXKLUSIV.
        res.clientId=static_cast<uint32_t>(id);
        res.x1=static_cast<int32_t>((static_cast<size_t>(id)*width_)/expected_);
        res.x2=static_cast<int32_t>((static_cast<size_t>(id+1)*width_)/expected_);
        res.y1=0;res.y2=static_cast<int32_t>(height_);
        res.width=static_cast<int32_t>(width_);res.height=static_cast<int32_t>(height_);
        return false;
    }
    const int id=findClient(req.token);
    if(id<0 || req.clientId!=static_cast<uint32_t>(id)) {res.status=FAILURE;return false;}
    ClientInfo& client=clients_[id];
    if(req.type==QUIT) {
        if(maxsteps_==0||timestep_<maxsteps_){res.status=FAILURE;return false;}
        client.quit=true;
        return allQuit();
    }
    if(req.type==BARRIER) {
        if(req.timestep<static_cast<uint32_t>(timestep_)) {
            // Wiederholte/lost UDP-Antwort oder TCP-Polling einer alten Barriere.
            res.status=(maxsteps_>0&&timestep_>=maxsteps_)?FINISHED:OK;
            return false;
        }
        if(req.timestep!=static_cast<uint32_t>(timestep_) ||
           (maxsteps_>0&&timestep_>=maxsteps_)) {res.status=FAILURE;return false;}
        client.arrived=true;
        client.address=sender;
        client.pending=req;
        if(allArrived()) {
            std::swap(current_,next_); ++timestep_;
            const ResultCode result=(maxsteps_>0&&timestep_>=maxsteps_)?FINISHED:OK;
            std::cerr<<"Zeitschritt "<<timestep_<<" abgeschlossen\n";
            if(!net_->isConnectionOriented()) {
                notifyAll(result);
                shouldReply=false; // Alle UDP-Antworten wurden bereits verteilt.
            } else {
                res.status=result; // TCP: die letzte Verbindung direkt beantworten.
            }
            for(auto& peer:clients_)peer.arrived=false;
        } else if(net_->isConnectionOriented()) {
            res.status=WAIT; // TCP darf die Verbindung nicht offen halten.
        } else {
            shouldReply=false; // UDP: Anfrage bis zur globalen Barriere blockieren.
        }
        return false;
    }
    if(req.timestep!=static_cast<uint32_t>(timestep_) || client.arrived) {
        res.status=FAILURE;return false;
    }
    if(req.x<0||req.y<0||static_cast<size_t>(req.x)>=width_||static_cast<size_t>(req.y)>=height_){
        res.status=FAILURE;return false;
    }
    if(req.type==READ) {
        res.value=(current_->readPos(req.x,req.y)==alive)?1:0;
    } else if(req.type==WRITE) {
        const size_t x1=(static_cast<size_t>(id)*width_)/expected_;
        const size_t x2=(static_cast<size_t>(id+1)*width_)/expected_;
        if(static_cast<size_t>(req.x)<x1||static_cast<size_t>(req.x)>=x2 ||
           (req.value!=0&&req.value!=1))res.status=FAILURE;
        else next_->setPos(req.x,req.y,req.value?alive:dead);
    } else res.status=FAILURE;
    return false;
}
void BoardServer::start() {
    for(;;) {
        IPAddress sender;Message request;
        const ssize_t size=net_->receive(sender,&request,sizeof(request));
        if(size!=static_cast<ssize_t>(sizeof(request)))continue;
        Message response;bool reply=true;
        const bool shouldStop=process(sender,request,response,reply);
        if(reply)net_->reply(sender,&response,sizeof(response));
        if(shouldStop)break;
    }
}
