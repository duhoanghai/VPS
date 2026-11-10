#ifndef LIFE_H
#define LIFE_H
#include "board/LocalBoard.h"
#include <random>
void randomize(LocalBoard& board,std::mt19937& generator);
void simulateStep(const LocalBoard& from,LocalBoard& to);
#endif
