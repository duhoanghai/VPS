#include "io/Rle.h"
#include "simulation/Life.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <random>
#include <string>
int main(int argc,char** argv) {
    try {
        std::string input="",output="local.rle";int steps=10,width=20,height=20;unsigned seed=42;
        for(int i=1;i<argc;++i){const std::string a=argv[i];
            if(a=="--help"||a=="-h"){std::cout<<"Usage: bin/local [--input PATH] [--output PATH] [--steps N] [--width W] [--height H] [--seed N]\n";return 0;}
            if(i+1>=argc)throw std::invalid_argument("Wert fehlt fuer "+a);
            const std::string v=argv[++i];
            if(a=="--input")input=v;else if(a=="--output")output=v;
            else if(a=="--steps")steps=std::stoi(v);else if(a=="--width")width=std::stoi(v);
            else if(a=="--height")height=std::stoi(v);else if(a=="--seed")seed=static_cast<unsigned>(std::stoul(v));
            else throw std::invalid_argument("Unbekanntes Argument: "+a);
        }
        if(steps<0||width<1||height<1)throw std::invalid_argument("Ungueltige Parameter");
        LocalBoard board=input.empty()?LocalBoard(width,height):loadRle(input);
        if(input.empty()){std::mt19937 gen(seed);randomize(board,gen);}
        LocalBoard next(board.getWidth(),board.getHeight());
        LocalBoard* from=&board;LocalBoard* to=&next;
        const auto t0=std::chrono::steady_clock::now();
        for(int i=0;i<steps;++i){simulateStep(*from,*to);std::swap(from,to);}
        const auto t1=std::chrono::steady_clock::now();
        saveRle(*from,output);
        std::cout<<"Lokal: "<<steps<<" Schritte, "<<std::chrono::duration<double>(t1-t0).count()<<" Sekunden, Ergebnis: "<<output<<'\n';
        return 0;
    }catch(const std::exception& e){std::cerr<<"Fehler: "<<e.what()<<'\n';return 1;}
}
