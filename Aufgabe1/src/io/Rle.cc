#include "io/Rle.h"
#include <cctype>
#include <fstream>
#include <limits>
#include <regex>
#include <stdexcept>

namespace {
// Dezimalzahlen schrittweise lesen und Überläufe vor der Berechnung erkennen.
size_t appendDigit(size_t value, unsigned digit) {
    if (value > (std::numeric_limits<size_t>::max() - digit) / 10)
        throw std::runtime_error("RLE number is too large");
    return value * 10 + digit;
}

size_t dimension(const std::string& text) {
    size_t result = 0;
    for (char c : text) result = appendDigit(result, static_cast<unsigned>(c - '0'));
    return result;
}
}

LocalBoard readRle(std::istream& input) {
    // Nach x und y wird der restliche Header wie in der Aufgabe ignoriert.
    // Die Simulation verwendet unabhängig vom angegebenen rule-Feld immer B3/S23.
    const std::regex header(R"(^\s*x\s*=\s*([0-9]+)\s*,\s*y\s*=\s*([0-9]+)\s*(,.*)?$)");
    std::string line;
    std::smatch match;
    bool found = false;
    while (std::getline(input, line)) {
        // Windows-Zeilenenden (CRLF) ebenfalls akzeptieren.
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line[first] == '#') continue;
        if (!std::regex_match(line, match, header))
            throw std::runtime_error("Invalid RLE header: expected x = width, y = height");
        found = true;
        break;
    }
    if (!found) throw std::runtime_error("Missing RLE header");
    LocalBoard board(dimension(match[1].str()), dimension(match[2].str()));
    // Nicht beschriebene Zellen bleiben durch den Konstruktor automatisch tot.
    size_t x = 0, y = 0, count = 0;
    bool haveCount = false;
    while (std::getline(input, line)) {
        const size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line[first] == '#') continue;
        for (unsigned char c : line) {
            // Physische Textzeilen sind keine Board-Zeilen; nur $ wechselt die Zeile.
            if (std::isspace(c)) continue;
            if (std::isdigit(c)) {
                count = appendDigit(count, c - '0');
                haveCount = true;
                continue;
            }
            if (c == '!') {
                if (haveCount) throw std::runtime_error("Run count before RLE terminator");
                return board;
            }
            // Ohne vorangestellte Zahl bezeichnet ein Symbol genau eine Wiederholung.
            const size_t run = haveCount ? count : 1;
            if (run == 0) throw std::runtime_error("RLE run length must be positive");
            if (c == 'b' || c == 'o') {
                if (y >= board.getHeight() || run > board.getWidth() - x)
                    throw std::runtime_error("RLE cell run exceeds board dimensions");
                if (c == 'o')
                    for (size_t i = 0; i < run; ++i) board.setPos(x + i, y, alive);
                x += run;
            } else if (c == '$') {
                if (run > board.getHeight() - y)
                    throw std::runtime_error("RLE row run exceeds board dimensions");
                y += run;
                x = 0;
            } else {
                throw std::runtime_error("Invalid RLE body character");
            }
            count = 0;
            haveCount = false;
        }
    }
    if (input.bad()) throw std::runtime_error("Could not read RLE stream");
    if (haveCount) throw std::runtime_error("RLE ends with an incomplete run count");
    // Das Beispiel im Aufgabenblatt enthält kein !; deshalb ist auch EOF zulässig.
    return board;
}

LocalBoard loadRle(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot open input file: " + path);
    return readRle(input);
}

void writeRle(const LocalBoard& board, std::ostream& output) {
    output << "#C VPS-I Game of Life (toroidal board)\n"
           << "x = " << board.getWidth() << ", y = " << board.getHeight()
           << ", rule = B3/S23\n";
    size_t column = 0;
    // Zwischen vollständigen Tokens umbrechen, damit die Datenzeilen kurz bleiben.
    const auto emit = [&](size_t count, char symbol) {
        const std::string token = (count > 1 ? std::to_string(count) : "") + symbol;
        if (column + token.size() > 70) { output << '\n'; column = 0; }
        output << token;
        column += token.size();
    };
    for (size_t y = 0; y < board.getHeight(); ++y) {
        // Tote Zellen am Zeilenende dürfen entfallen. Die Header-Abmessungen bleiben
        // erhalten, da eine Verkleinerung die toroidale Nachbarschaft ändern würde.
        size_t end = board.getWidth();
        while (end > 0 && board.readPos(end - 1, y) == dead) --end;
        for (size_t x = 0; x < end;) {
            const life_status_t state = board.readPos(x, y);
            size_t next = x + 1;
            while (next < end && board.readPos(next, y) == state) ++next;
            emit(next - x, state == alive ? 'o' : 'b');
            x = next;
        }
        if (y + 1 < board.getHeight()) emit(1, '$');
    }
    emit(1, '!');
    output << '\n';
    if (!output) throw std::runtime_error("Could not write RLE stream");
}

void saveRle(const LocalBoard& board, const std::string& path) {
    std::ofstream output(path);
    if (!output) throw std::runtime_error("Cannot open output file: " + path);
    writeRle(board, output);
    output.close();
    if (!output) throw std::runtime_error("Could not finish writing: " + path);
}
