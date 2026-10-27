#include "io/Rle.h"
#include "simulation/Life.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <limits>

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void equal(const LocalBoard& a, const LocalBoard& b) {
    check(a.getWidth() == b.getWidth() && a.getHeight() == b.getHeight(), "dimensions differ");
    for (size_t y = 0; y < a.getHeight(); ++y)
        for (size_t x = 0; x < a.getWidth(); ++x)
            check(a.readPos(x,y) == b.readPos(x,y), "cells differ");
}
template<class F> void rejects(F action) {
    bool caught = false;
    try { action(); } catch (const std::exception&) { caught = true; }
    check(caught, "invalid input accepted");
}
LocalBoard parse(const std::string& text) {
    std::istringstream stream(text);
    return readRle(stream);
}
void roundtrip(const LocalBoard& board) {
    std::ostringstream encoded;
    writeRle(board, encoded);
    equal(board, parse(encoded.str()));
    std::istringstream lines(encoded.str());
    std::string line;
    while (std::getline(lines, line))
        if (!line.empty() && line[0] != '#' && line[0] != 'x')
            check(line.size() <= 70, "RLE body line too long");
}

int main() {
    try {
        LocalBoard b(5,5), next(5,5);
        Board& interface = b;
        check(interface.readPos(0,0) == dead, "initial cells not dead");
        check(interface.readPos(5,0) == invalid, "out-of-range read");
        interface.setPos(5,0,alive);
        interface.setPos(0,0,invalid);
        check(b.readPos(0,0) == dead, "invalid write modified board");
        rejects([] { LocalBoard zero(0,1); });
        rejects([] { LocalBoard overflow(std::numeric_limits<size_t>::max(),2); });
        rejects([&] { simulateStep(b,b); });
        rejects([&] { LocalBoard small(1,1); simulateStep(b,small); });

        // Vollständige B3/S23-Wahrheitstabelle für die Mitte: 512 Konfigurationen.
        for (unsigned mask = 0; mask < 512; ++mask) {
            LocalBoard from(5,5), to(5,5);
            unsigned neighbors = 0;
            for (unsigned k = 0; k < 9; ++k) {
                const bool live = (mask & (1u << k)) != 0;
                from.setPos(1+k%3,1+k/3, live ? alive : dead);
                if (k != 4 && live) ++neighbors;
            }
            simulateStep(from,to);
            check((to.readPos(2,2) == alive) ==
                  (neighbors == 3 || (neighbors == 2 && (mask & 16))), "B3/S23 truth table");
        }
        b.setPos(1,1,alive); b.setPos(2,1,alive);
        b.setPos(1,2,alive); b.setPos(2,2,alive);
        simulateStep(b,next); equal(b,next); // Block: unveränderliches Muster.
        LocalBoard blinker(5,5), phase(5,5), restored(5,5);
        blinker.setPos(4,0,alive); blinker.setPos(0,0,alive); blinker.setPos(1,0,alive);
        simulateStep(blinker,phase);
        for (size_t y=0;y<5;++y) for (size_t x=0;x<5;++x)
            check((phase.readPos(x,y)==alive) == (x==0 && (y==4 || y==0 || y==1)), "wrapped blinker");
        simulateStep(phase,restored); equal(blinker,restored);

        // Unabhängige Referenz mit vorzeichenbehaftetem Modulo, auch für kleine Rechtecke.
        std::mt19937 rng(42);
        for (size_t h=1;h<=9;++h) for (size_t w=1;w<=9;++w) {
            LocalBoard from(w,h), to(w,h);
            randomize(from,rng);
            for (int step=0;step<8;++step) {
                simulateStep(from,to);
                for (size_t y=0;y<h;++y) for (size_t x=0;x<w;++x) {
                    unsigned n=0;
                    for (int dy=-1;dy<=1;++dy) for(int dx=-1;dx<=1;++dx)
                        if(dx || dy) n += from.readPos((int(x)+dx+int(w))%int(w),
                                                       (int(y)+dy+int(h))%int(h))==alive;
                    check((to.readPos(x,y)==alive)==(n==3 || (n==2 && from.readPos(x,y)==alive)), "modulo oracle");
                }
                std::swap(from,to);
            }
            roundtrip(from);
        }
        auto example = parse("# comment\nx = 5, y = 5, ignore rest of line\nbbbbb$\n2bo2b$\n3bo$\nb3o$$\n");
        check(example.readPos(2,1)==alive && example.readPos(3,2)==alive, "PDF example");
        roundtrip(example);
        auto multiline = parse("\r\n #comment\r\nx=15,y=5, rule=B3/S23\r\n1\n2o2$\n# body comment\n3bo!\n");
        check(multiline.readPos(11,0)==alive && multiline.readPos(3,2)==alive, "multiline/count/blank rows");
        roundtrip(multiline);
        roundtrip(LocalBoard(3,4));
        for (const char* malformed : {"", "x=0,y=3\n!", "x=3,y=3\n4o!", "x=3,y=3\n4$!",
             "x=3,y=3\n0o!", "x=3,y=3\n2", "x=3,y=3\n2!", "x=3,y=3\nz!",
             "x=3,y=3\n3$o!", "x=3,y=3\n99999999999999999999999999o!"})
            rejects([&] { parse(malformed); });
        for (const char* name : {"glider", "bigun", "c5greyship", "hwss", "tumbler"})
            roundtrip(loadRle(std::string("boards/")+name+".rle"));
        LocalBoard glider = loadRle("boards/glider.rle"), original = glider, temp(6,6);
        for (int i=0;i<4;++i) { simulateStep(glider,temp); std::swap(glider,temp); }
        for (size_t y=0;y<6;++y) for(size_t x=0;x<6;++x)
            check(glider.readPos((x+1)%6,(y+1)%6)==original.readPos(x,y), "glider translation");
        std::cout << "PASS: board interface, 512 neighborhoods, toroidal oracle, patterns, RLE roundtrips and errors\n";
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
