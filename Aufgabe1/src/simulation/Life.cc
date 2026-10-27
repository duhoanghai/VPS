#include "simulation/Life.h"
#include <stdexcept>

void randomize(LocalBoard& board, std::mt19937& generator) {
    std::bernoulli_distribution liveCell(0.25);
    for (size_t y = 0; y < board.getHeight(); ++y)
        for (size_t x = 0; x < board.getWidth(); ++x)
            board.setPos(x, y, liveCell(generator) ? alive : dead);
}

void simulateStep(const LocalBoard& from, LocalBoard& to) {
    // Gleichzeitige Generationen erfordern getrennte Lese- und Schreibpuffer.
    if (&from == &to || from.getWidth() != to.getWidth() ||
        from.getHeight() != to.getHeight())
        throw std::invalid_argument("simulateStep needs two different, equally sized boards");

    const size_t w = from.getWidth(), h = from.getHeight();
    for (size_t y = 0; y < h; ++y) {
        // Oberer/unterer Rand wird auf die gegenüberliegende Seite abgebildet.
        // Fallunterscheidungen vermeiden einen size_t-Unterlauf bei y == 0.
        const size_t ys[] = {y == 0 ? h - 1 : y - 1, y, y + 1 == h ? 0 : y + 1};
        for (size_t x = 0; x < w; ++x) {
            // Dieselbe Randbehandlung für links/rechts, einschließlich der Ecken.
            const size_t xs[] = {x == 0 ? w - 1 : x - 1, x, x + 1 == w ? 0 : x + 1};
            // Acht Richtungen zählen; die Mitte ist kein Nachbar. Bei sehr kleinen
            // Boards können mehrere Richtungen auf dieselbe Zelle zeigen.
            unsigned neighbors = 0;
            for (unsigned j = 0; j < 3; ++j)
                for (unsigned i = 0; i < 3; ++i)
                    if (i != 1 || j != 1)
                        neighbors += (from.readPos(xs[i], ys[j]) == alive);
            // Conway B3/S23: Geburt bei 3, Überleben bei 2 oder 3 Nachbarn.
            const bool lives = neighbors == 3 ||
                               (neighbors == 2 && from.readPos(x, y) == alive);
            to.setPos(x, y, lives ? alive : dead);
        }
    }
}
