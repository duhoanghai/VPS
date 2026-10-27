#ifndef LIFE_H
#define LIFE_H
#include "board/LocalBoard.h"
#include <random>

// Belegt jede Zelle unabhängig mit 25 Prozent Wahrscheinlichkeit als lebendig.
void randomize(LocalBoard& board, std::mt19937& generator);
// Liest ausschließlich from und überschreibt to vollständig.
// Beide Boards müssen verschieden sein und dieselben Abmessungen besitzen.
void simulateStep(const LocalBoard& from, LocalBoard& to);
#endif
