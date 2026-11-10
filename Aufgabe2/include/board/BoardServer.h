#ifndef BOARDSERVER_H
#define BOARDSERVER_H
#include "board/Board.h"
#include "net/IPNetwork.h"
#include "net/Protocol.h"
#include <vector>

// Der Server verwaltet beide Generationen und koordiniert alle angemeldeten Clients.
class BoardServer {
public:
    BoardServer(IPNetwork* network,int clientcount,Board* board_a,Board* board_b,int timesteps=0);
    void start();
    Board* finalBoard() const { return current_; }
    int completedSteps() const { return timestep_; }
private:
    struct ClientInfo {
        uint32_t token=0;
        bool arrived=false;
        bool quit=false;
        IPAddress address;
        Message pending; // Offene UDP-Barrierenantwort samt Sequenznummer.
    };
    IPNetwork* net_;
    Board* current_;
    Board* next_;
    int expected_;
    int timestep_=0;
    int maxsteps_;
    size_t width_=0,height_=0;
    std::vector<ClientInfo> clients_;
    int findClient(uint32_t token) const;
    bool allArrived() const;
    bool allQuit() const;
    void notifyAll(ResultCode status);
    bool process(const IPAddress& sender,const Message& request,Message& response,bool& shouldReply);
};
#endif
