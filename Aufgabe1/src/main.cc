#include "gui/DrawingWindow.h"
#include "io/Rle.h"
#include "simulation/Life.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>

namespace {
// Vorgabewerte: zufälliges Board, 100 Schritte, keine grafische Ausgabe.
struct Options {
    std::string input, output;
    size_t steps = 100;
    bool gui = false, help = false;
};

// Liest Flags in beliebiger Reihenfolge; unbekannte Flags und ungültige Zahlen
// führen zu einer Fehlermeldung statt zu einem stillschweigend falschen Lauf.
Options parseOptions(int argc, char* argv[]) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-g") options.gui = true;
        else if (arg == "-h" || arg == "--help") options.help = true;
        else if (arg == "-i" || arg == "-o" || arg == "-r") {
            if (++i == argc || argv[i][0] == '\0')
                throw std::invalid_argument("Missing value for " + arg);
            const std::string value = argv[i];
            if (arg == "-i") options.input = value;
            else if (arg == "-o") options.output = value;
            else {
                // Nur nicht negative ganze Zahlen innerhalb des size_t-Bereichs zulassen.
                size_t steps = 0;
                for (char c : value) {
                    if (c < '0' || c > '9' ||
                        steps > (std::numeric_limits<size_t>::max() - (c - '0')) / 10)
                        throw std::invalid_argument("-r needs a non-negative integer in range");
                    steps = steps * 10 + (c - '0');
                }
                options.steps = steps;
            }
        } else throw std::invalid_argument("Unknown option: " + arg);
    }
    return options;
}

void drawBoard(const LocalBoard& board, GUI::DrawingWindow& window, unsigned scale) {
    // Auch tote Zellen zeichnen, damit zuvor lebendige Zellen wieder verschwinden.
    for (size_t y = 0; y < board.getHeight(); ++y)
        for (size_t x = 0; x < board.getWidth(); ++x) {
            window.setForegroundColor(board.readPos(x, y) == alive ? GUI::LIME : GUI::BLACK);
            window.drawFilledRectangle(static_cast<unsigned>(x) * scale,
                                       static_cast<unsigned>(y) * scale, scale, scale);
        }
}
}

int main(int argc, char* argv[]) {
    try {
        const Options options = parseOptions(argc, argv);
        if (options.help) {
            std::cout << "Usage: " << argv[0] << " [-i input.rle] [-o output.rle] [-r steps] [-g]\n"
                      << "Default: random board, 100 steps, no GUI. -r 0 saves generation 0.\n";
            return 0;
        }
        // Eine Eingabedatei liefert eine reproduzierbare Generation 0. Ohne Datei
        // werden Größe (50 bis 150 je Richtung) und Zellzustände zufällig gewählt.
        std::mt19937 generator(std::random_device{}());
        std::uniform_int_distribution<int> dimension(50, 150);
        LocalBoard boardA = options.input.empty()
            ? LocalBoard(dimension(generator), dimension(generator)) : loadRle(options.input);
        if (options.input.empty()) randomize(boardA, generator);
        // Beide Speicherpuffer genau einmal anlegen; später nur die Zeiger tauschen.
        LocalBoard boardB(boardA.getWidth(), boardA.getHeight());
        LocalBoard* current = &boardA;
        LocalBoard* next = &boardB;

        std::unique_ptr<GUI::DrawingWindow> window;
        unsigned scale = 1;
        if (options.gui) {
            // X11 verwendet vorzeichenbehaftete 16-Bit-Zeichenkoordinaten.
            if (current->getWidth() > 32767 || current->getHeight() > 32767)
                throw std::invalid_argument("Board is too large for the X11 window");
            // Höchstens 12 Pixel pro Zelle; große Boards mindestens mit einem Pixel.
            scale = static_cast<unsigned>(std::max<size_t>(1, std::min<size_t>(12,
                std::min(1000 / current->getWidth(), 750 / current->getHeight()))));
            window.reset(new GUI::DrawingWindow(current->getWidth() * scale,
                current->getHeight() * scale, "VPS-I Game of Life", true, true));
            // Generation 0 vor dem ersten Simulationsschritt anzeigen.
            drawBoard(*current, *window, scale);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        for (size_t step = 0; step < options.steps; ++step) {
            simulateStep(*current, *next);
            std::swap(current, next); // Neuer Zustand wird zur nächsten Lesequelle.
            // Ein geschlossenes Fenster beendet nur die Darstellung. Alle angeforderten
            // Schritte werden weiter berechnet, damit die Ausgabedatei vollständig ist.
            if (window && !window->isDestroyed()) {
                drawBoard(*current, *window, scale);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        // Bei -r 0 wird hier unverändert die Anfangsgeneration gespeichert.
        if (!options.output.empty()) saveRle(*current, options.output);
        std::cout << "Simulated " << options.steps << " steps on "
                  << current->getWidth() << " x " << current->getHeight() << " cells.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
