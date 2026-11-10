#ifndef LOCAL_BOARD_H
#define LOCAL_BOARD_H
#include "board/Board.h"
#include <vector>
class LocalBoard : public Board {
public:
    LocalBoard(size_t width, size_t height);
    life_status_t readPos(size_t x, size_t y) override;
    life_status_t readPos(size_t x, size_t y) const;
    void setPos(size_t x, size_t y, life_status_t status) override;
    size_t getWidth() const { return width; }
    size_t getHeight() const { return height; }
private:
    std::vector<unsigned char> cells;
};
#endif
