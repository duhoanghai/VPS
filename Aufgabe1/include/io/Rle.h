#ifndef RLE_H
#define RLE_H
#include "board/LocalBoard.h"
#include <iosfwd>
#include <string>

// Parser und Writer arbeiten mit Streams und sind ohne echte Dateien testbar.
LocalBoard readRle(std::istream& input);
// Dateivariante: öffnet die Datei und ruft den Parser auf.
LocalBoard loadRle(const std::string& path);
void writeRle(const LocalBoard& board, std::ostream& output);
// Dateivariante: schreibt das Board und prüft auch Fehler beim Schließen.
void saveRle(const LocalBoard& board, const std::string& path);
#endif
