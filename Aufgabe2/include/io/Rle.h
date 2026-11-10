#ifndef RLE_H
#define RLE_H
#include "board/LocalBoard.h"
#include <iosfwd>
#include <string>
LocalBoard readRle(std::istream& input);
LocalBoard loadRle(const std::string& path);
void writeRle(const LocalBoard& board,std::ostream& output);
void saveRle(const LocalBoard& board,const std::string& path);
#endif
