#include "board/LocalBoard.h"
#include <limits>
#include <stdexcept>
LocalBoard::LocalBoard(size_t width, size_t height) : Board(width, height) {
    if (width == 0 || height == 0) throw std::invalid_argument("Board dimensions must be positive");
    if (width > std::numeric_limits<size_t>::max() / height) throw std::length_error("Board dimensions overflow");
    cells.assign(width * height, 0);
}
life_status_t LocalBoard::readPos(size_t x, size_t y) { return static_cast<const LocalBoard&>(*this).readPos(x,y); }
life_status_t LocalBoard::readPos(size_t x, size_t y) const {
    if (x >= width || y >= height) return invalid;
    return cells[y*width+x] ? alive : dead;
}
void LocalBoard::setPos(size_t x,size_t y,life_status_t status) {
    if (x<width && y<height && (status==dead || status==alive)) cells[y*width+x]=(status==alive);
}
