#include "client/LifeClient.h"
#include "net/TCPNetwork.h"
#include "net/UDPNetwork.h"
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
int main(int argc,char** argv) {
    try {
        std::string protocol="udp",host="localhost";int port=7654;
        for(int i=1;i<argc;++i){
            const std::string a=argv[i];
            if(a=="--help"||a=="-h") {std::cout<<"Usage: bin/client [--protocol udp|tcp] [--host HOST] [--port N]\n";return 0;}
            if(i+1>=argc)throw std::invalid_argument("Wert fehlt fuer "+a);
            const std::string v=argv[++i];
            if(a=="--protocol")protocol=v;
            else if(a=="--host")host=v;
            else if(a=="--port")port=std::stoi(v);
            else throw std::invalid_argument("Unbekanntes Argument: "+a);
        }
        if(port<1||port>65535||(protocol!="udp"&&protocol!="tcp"))throw std::invalid_argument("Ungueltige Parameter");
        std::unique_ptr<IPNetwork> net(protocol=="udp"?static_cast<IPNetwork*>(new UDPNetwork()):static_cast<IPNetwork*>(new TCPNetwork()));
        LifeClient client(net.get(),host.c_str(),static_cast<short>(port));
        client.run();return 0;
    }catch(const std::exception& e){std::cerr<<"Fehler: "<<e.what()<<'\n';return 1;}
}
