#include "board/BoardServer.h"
#include "board/LocalBoard.h"
#include "io/Rle.h"
#include "net/TCPNetwork.h"
#include "net/UDPNetwork.h"
#include "simulation/Life.h"
#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>

int main(int argc,char** argv) {
    try {
        std::string protocol="udp",input="",output="final.rle";
        int port=7654,clients=2,steps=10,width=20,height=20;unsigned seed=42;
        for(int i=1;i<argc;++i) {
            const std::string a=argv[i];
            if(a=="--help"||a=="-h") {
                std::cout<<"Usage: bin/server [--protocol udp|tcp] [--port N] [--clients N] [--steps N] [--input PATH] [--output PATH] [--width W] [--height H] [--seed N]\n";
                return 0;
            }
            if(i+1>=argc)throw std::invalid_argument("Wert fehlt fuer "+a);
            const std::string v=argv[++i];
            if(a=="--protocol")protocol=v;
            else if(a=="--port")port=std::stoi(v);
            else if(a=="--clients")clients=std::stoi(v);
            else if(a=="--steps")steps=std::stoi(v);
            else if(a=="--input")input=v;
            else if(a=="--output")output=v;
            else if(a=="--width")width=std::stoi(v);
            else if(a=="--height")height=std::stoi(v);
            else if(a=="--seed")seed=static_cast<unsigned>(std::stoul(v));
            else throw std::invalid_argument("Unbekanntes Argument: "+a);
        }
        if(port<1||port>65535||clients<1||steps<0||width<1||height<1||(protocol!="udp"&&protocol!="tcp"))
            throw std::invalid_argument("Ungueltige Parameter");
        LocalBoard board=input.empty()?LocalBoard(width,height):loadRle(input);
        if(input.empty()){std::mt19937 generator(seed);randomize(board,generator);}
        LocalBoard next(board.getWidth(),board.getHeight());
        std::unique_ptr<IPNetwork> net(protocol=="udp"?static_cast<IPNetwork*>(new UDPNetwork(static_cast<short>(port))):static_cast<IPNetwork*>(new TCPNetwork(static_cast<short>(port))));
        BoardServer server(net.get(),clients,&board,&next,steps);
        std::cerr<<"Server "<<protocol<<" auf Port "<<port<<", "<<clients<<" Clients, "<<steps<<" Schritte (0 = unbegrenzt)\n";
        const auto t0=std::chrono::steady_clock::now();
        server.start();
        const auto t1=std::chrono::steady_clock::now();
        saveRle(*dynamic_cast<LocalBoard*>(server.finalBoard()),output);
        std::cout<<"Server: "<<server.completedSteps()<<" Schritte, "
                 <<std::chrono::duration<double>(t1-t0).count()<<" Sekunden, Ergebnis: "<<output<<'\n';
        return 0;
    }catch(const std::exception& e){std::cerr<<"Fehler: "<<e.what()<<'\n';return 1;}
}
