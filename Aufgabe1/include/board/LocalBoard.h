#ifndef LOCAL_BOARD_H
#define LOCAL_BOARD_H

#include "board/Board.h"
#include <vector>

/** Konkretes Spielfeld: speichert Zellen, enthält aber keine Simulations- oder GUI-Logik. */
class LocalBoard : public Board {
public:
    // Erstellt ein nicht leeres Spielfeld; anfangs sind alle Zellen tot.
    LocalBoard(size_t width, size_t height);
    life_status_t readPos(size_t x, size_t y) override;
    // Zusätzlicher const-Zugriff für Algorithmen; die Vorgabe selbst ist nicht const.
    life_status_t readPos(size_t x, size_t y) const;
    void setPos(size_t x, size_t y, life_status_t status) override;
    size_t getWidth() const { return width; }
    size_t getHeight() const { return height; }

private:
    // Zeilenweise Speicherung: Index = y * width + x, 0 = tot, 1 = lebendig.
    std::vector<unsigned char> cells;
};
#endif
