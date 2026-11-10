#include "io/Rle.h"
#include <cctype>
#include <fstream>
#include <limits>
#include <regex>
#include <stdexcept>
namespace {
size_t appendDigit(size_t value,unsigned digit) {
    if(value > (std::numeric_limits<size_t>::max()-digit)/10) throw std::runtime_error("RLE number is too large");
    return value*10+digit;
}
size_t dimension(const std::string& text) { size_t result=0; for(char c:text) result=appendDigit(result,static_cast<unsigned>(c-'0')); return result; }
}
LocalBoard readRle(std::istream& input) {
    const std::regex header(R"(^\s*x\s*=\s*([0-9]+)\s*,\s*y\s*=\s*([0-9]+)\s*(,.*)?$)");
    std::string line; std::smatch match; bool found=false;
    while(std::getline(input,line)) {
        if(!line.empty() && line.back()=='\r') line.pop_back();
        const size_t first=line.find_first_not_of(" \t\r");
        if(first==std::string::npos || line[first]=='#') continue;
        if(!std::regex_match(line,match,header)) throw std::runtime_error("Invalid RLE header");
        found=true; break;
    }
    if(!found) throw std::runtime_error("Missing RLE header");
    LocalBoard board(dimension(match[1].str()),dimension(match[2].str()));
    size_t x=0,y=0,count=0; bool haveCount=false;
    while(std::getline(input,line)) {
        const size_t first=line.find_first_not_of(" \t\r");
        if(first==std::string::npos || line[first]=='#') continue;
        for(unsigned char c:line) {
            if(std::isspace(c)) continue;
            if(std::isdigit(c)) {count=appendDigit(count,c-'0');haveCount=true;continue;}
            if(c=='!') {if(haveCount)throw std::runtime_error("Run count before terminator");return board;}
            const size_t run=haveCount?count:1;
            if(run==0)throw std::runtime_error("Run length zero");
            if(c=='b'||c=='o') {
                if(y>=board.getHeight() || x>board.getWidth() || run>board.getWidth()-x)throw std::runtime_error("RLE exceeds board");
                if(c=='o')for(size_t i=0;i<run;++i) board.setPos(x+i,y,alive);
                x+=run;
            } else if(c=='$') {
                if(y>board.getHeight() || run>board.getHeight()-y)throw std::runtime_error("RLE row exceeds board");
                y+=run;x=0;
            } else throw std::runtime_error("Invalid RLE character");
            count=0;haveCount=false;
        }
    }
    if(input.bad()||haveCount)throw std::runtime_error("Incomplete RLE stream");
    return board;
}
LocalBoard loadRle(const std::string& path) { std::ifstream in(path);if(!in)throw std::runtime_error("Cannot open input file: "+path);return readRle(in); }
void writeRle(const LocalBoard& board,std::ostream& out) {
    out << "#C VPS-I Game of Life (toroidal board)\n" << "x = " <<board.getWidth()<<", y = "<<board.getHeight()<<", rule = B3/S23\n";
    size_t column=0;
    const auto emit=[&](size_t count,char symbol) {
        const std::string token=(count>1?std::to_string(count):"")+symbol;
        if(column+token.size()>70){out<<'\n';column=0;}out<<token;column+=token.size();
    };
    for(size_t y=0;y<board.getHeight();++y){
        size_t end=board.getWidth();while(end>0&&board.readPos(end-1,y)==dead)--end;
        for(size_t x=0;x<end;){const life_status_t state=board.readPos(x,y);size_t next=x+1;
            while(next<end&&board.readPos(next,y)==state)++next;
            emit(next-x,state==alive?'o':'b');x=next;}
        if(y+1<board.getHeight())emit(1,'$');
    }
    emit(1,'!');out<<'\n';if(!out)throw std::runtime_error("Could not write RLE");
}
void saveRle(const LocalBoard& b,const std::string& path){std::ofstream out(path);if(!out)throw std::runtime_error("Cannot open output: "+path);writeRle(b,out);out.close();if(!out)throw std::runtime_error("Could not save: "+path);}
