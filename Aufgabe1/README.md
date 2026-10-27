# VPS-I – Aufgabe 1: Game of Life

C++11-Implementierung von Conways Game of Life auf Grundlage der bereitgestellten Vorgabe. Das Projekt umfasst die Simulation, das Lesen und Schreiben von RLE-Dateien sowie eine grafische Darstellung mit `GUI::DrawingWindow`.

Diese README beschreibt die fertige Lösung, ihre Dateistruktur und die Änderungen gegenüber der Vorgabe. Die Beispiele werden im Verzeichnis mit diesem Makefile ausgeführt. Wird nur der Inhalt von `solution/` als GitHub-Repository hochgeladen, ist dies das Wurzelverzeichnis des Repositorys.

## 1. Aufgabenstellung und umgesetzte Funktionen

**Teil a: Simulation und Dateiverarbeitung**

- `LocalBoard` implementiert die vorgegebene abstrakte Schnittstelle `Board`.
- Eine tote Zelle mit genau drei lebendigen Nachbarn wird lebendig.
- Eine lebendige Zelle mit zwei oder drei lebendigen Nachbarn bleibt lebendig.
- In allen anderen Fällen ist die Zelle in der nächsten Generation tot.
- Alle acht Nachbarrichtungen werden berücksichtigt. An den Rändern wird auf die gegenüberliegende Seite übergegangen: Das Spielfeld ist toroidal.
- Generation 0 kann zufällig erzeugt oder aus einer RLE-Datei geladen werden.
- Der Endzustand kann als RLE-Datei gespeichert werden.
- Die Parameter `-i`, `-o`, `-r` und `-g` werden unterstützt.

**Teil b: Grafische Darstellung**

- Darstellung mit der bereitgestellten Klasse `DrawingWindow` auf Basis von X11.
- Lebendige Zellen sind grün, tote Zellen schwarz.
- Generation 0 und die folgenden Generationen werden angezeigt.

Zusätzlich vorhanden sind Eingabeprüfungen, `--help`/`-h`, automatisierte Tests und Anpassungen für XQuartz unter macOS.

## 2. Projektstruktur

```text
.
├── README.md                     Projektdokumentation
├── Makefile                      Regeln zum Übersetzen und Testen
├── .gitignore                    Ausschluss generierter und lokaler Dateien
├── include/                      Header: Schnittstellen und Deklarationen
│   ├── board/
│   │   ├── Board.h               Abstrakte Schnittstelle aus der Vorgabe
│   │   └── LocalBoard.h          Deklaration des konkreten Spielfelds
│   ├── simulation/Life.h         Deklaration der Simulationsfunktionen
│   ├── io/Rle.h                  Deklaration der RLE-Funktionen
│   ├── gui/DrawingWindow.h       Grafische Schnittstelle aus der Vorgabe
│   ├── thread/Thread.h           Hilfsklasse für pthreads
│   ├── timer/                    Unveränderte Zeitmessungshelfer der Vorgabe
│   └── misc/debug.h              Unveränderte Debug-Hilfe der Vorgabe
├── src/                          C++-Implementierungen
│   ├── main.cc                   Programmstart, Parameter und Ablaufsteuerung
│   ├── board/LocalBoard.cc       Speicherung und Zugriff auf Zellen
│   ├── simulation/Life.cc        Zufallsbelegung und Generationswechsel
│   ├── io/Rle.cc                 RLE-Parser und RLE-Writer
│   └── gui/DrawingWindow.cc      X11-Fenster und Zeichenfunktionen
├── boards/                       Die fünf RLE-Eingabemuster aus der Vorgabe
├── tests/
│   ├── test_life.cc              Spielfeld-, Regel- und RLE-Tests
│   ├── test_thread.cc            Regressionstest für Thread-Beendigung
│   ├── test_cli.py               Tests der Kommandozeilenschnittstelle
│   ├── test_gui.py               GUI-Ablauf und Vergleich der Ausgabedateien
│   └── gui_probe.cc              Pixelprüfung und simuliertes Fensterschließen
├── bin/                          Beim Build erzeugte ausführbare Programme
└── tmp/                          Beim Build erzeugte Objekt- und Dependency-Dateien
```

### Header und Quelldateien

Eine **Headerdatei (`.h`)** beschreibt, welche Klassen und Funktionen verfügbar sind. Eine **Quelldatei (`.cc`)** enthält deren Implementierung. `.cc` und `.cpp` sind übliche Endungen für C++; `.c` bezeichnet normalerweise C-Quellcode. Die Endung `.cc` wurde aus der Vorgabe übernommen.

Beispiel: [`LocalBoard.h`](include/board/LocalBoard.h) deklariert die Klasse und ihre Methoden. [`LocalBoard.cc`](src/board/LocalBoard.cc) legt fest, wie diese Methoden auf die gespeicherten Zellen zugreifen. Mit `#include "board/LocalBoard.h"` wird die Deklaration in anderen Quelldateien verfügbar. Kurze Methoden können auch direkt im Header definiert sein.

Die Unterordner gruppieren Dateien nach ihrer Aufgabe: `board` verwaltet Daten, `simulation` berechnet Zustände, `io` liest und schreibt Daten und `gui` stellt sie dar. Diese zusätzliche Aufteilung dient der Übersicht; die Aufgabe schreibt nicht alle diese Ordnernamen vor.

### Eingabedaten und Build-Ergebnisse

`boards/` enthält Daten, keinen Programmcode. Dagegen enthalten `include/board/` und `src/board/` den C++-Code zur Verwaltung beliebiger Spielfelder.

`bin/main` ist das fertige Programm. Es wird nicht von Hand bearbeitet. `tmp/` enthält unter anderem `.o`-Objektdateien und `.d`-Dateien mit Header-Abhängigkeiten. Beide Ordner können durch `make clean` entfernt und durch `make` neu aufgebaut werden.

```text
src/*.cc + eingebundene Header
             ↓ Compiler
      tmp/*.o und tmp/*.d
             ↓ Linker + X11/pthread
           bin/main
```

## 3. Architektur und Programmablauf

[`main.cc`](src/main.cc) verbindet die einzelnen Komponenten:

```text
Parameter lesen
      ↓
RLE laden oder Generation 0 zufällig erzeugen
      ↓
Zweiten, gleich großen Board-Puffer anlegen
      ↓
Optional: Fenster öffnen und Generation 0 zeichnen
      ↓
N-mal: Generation berechnen → Zeiger tauschen → optional zeichnen
      ↓
Optional: Endzustand als RLE speichern
      ↓
Programm und Fenster beenden
```

### Board und LocalBoard

`Board` ist eine abstrakte Basisklasse mit den rein virtuellen Methoden `readPos` und `setPos`. Sie kann nicht unmittelbar instanziiert werden. `LocalBoard` erbt von `Board` und stellt die konkrete Speicherung bereit.

Die Zellen liegen zeilenweise in einem `std::vector<unsigned char>`. Der Index einer Zelle lautet `y * width + x`. Anfangs sind alle Zellen tot. Ein Lesezugriff außerhalb des Feldes liefert `invalid`; ungültige Schreibzugriffe werden ignoriert. Die zusätzlich angebotene `const`-Überladung von `readPos` ermöglicht lesende Algorithmen ohne Änderung der ursprünglichen Schnittstelle.

`LocalBoard` speichert nur das Feld. Dateiverarbeitung, Simulation und Darstellung bleiben getrennte Aufgaben.

### Simulationsschritt und Double Buffering

[`simulateStep`](src/simulation/Life.cc) liest ausschließlich das alte Board und beschreibt vollständig ein zweites Board. Danach werden nur die beiden Zeiger getauscht:

```cpp
simulateStep(*current, *next);
std::swap(current, next);
```

So verwenden alle Zellen dieselbe alte Generation. Eine Änderung direkt im gelesenen Board würde spätere Berechnungen beeinflussen und das Ergebnis verfälschen. Die beiden Puffer werden einmal angelegt; pro Schritt werden weder komplette Boards kopiert noch neue Zellpuffer angefordert.

Der Rechenaufwand pro Generation beträgt O(Breite × Höhe). Die beiden Zellpuffer benötigen zusammen ungefähr 2 × Breite × Höhe Bytes.

### Randbehandlung

Für `x == 0` liegt der linke Nachbar bei `width - 1`, für `x == width - 1` der rechte Nachbar bei `0`. Dasselbe gilt für oben, unten und die Diagonalen. Die Fallunterscheidungen verhindern einen Unterlauf des vorzeichenlosen Typs `size_t`.

Wrap-around gehört zum Simulationsalgorithmus, nicht zur Schnittstelle `readPos`. Bei einer Breite oder Höhe von 1 oder 2 können mehrere der acht Richtungen auf dieselbe Zelle zeigen; gezählt werden weiterhin alle acht Richtungen.

## 4. RLE-Dateiformat

[`Rle.cc`](src/io/Rle.cc) trennt Stream-Verarbeitung (`readRle`, `writeRle`) von Dateizugriffen (`loadRle`, `saveRle`). Dadurch kann der Parser auch mit Text im Arbeitsspeicher getestet werden.

```text
#C Beispiel: Glider
x = 3, y = 3, rule = B3/S23
bob$2bo$3o!
```

| Zeichen | Bedeutung |
|---|---|
| `#` am Zeilenanfang, auch nach Leerraum | Kommentarzeile |
| `x`, `y` im Header | Breite und Höhe des Spielfelds |
| `b` | Tote Zelle |
| `o` | Lebendige Zelle |
| Zahl vor einem Symbol | Wiederholungszahl, beispielsweise `3o` |
| `$` | Nächste Board-Zeile; `3$` rückt drei Zeilen weiter |
| `!` | Ende des Musters |

Physische Textumbrüche ändern die Board-Zeile nicht. Nicht beschriebene Zellen bleiben tot. Der Parser unterstützt LF und CRLF, mehrstellige Wiederholungszahlen und das Aufgabenbeispiel ohne abschließendes `!`. Ungültige Symbole, Überläufe und Überschreitungen der angegebenen Board-Größe werden zurückgewiesen.

Der Header-Zusatz nach den Abmessungen wird entsprechend dem Aufgabenbeispiel ignoriert. Die Simulation verwendet immer **B3/S23**, auch wenn ein anderes `rule`-Feld angegeben wird. Andere zelluläre Automaten werden nicht unterstützt.

Der Writer schreibt `!`, begrenzt Datenzeilen auf 70 Zeichen und erhält die Board-Abmessungen. Das ist wichtig: Ein Verkleinern auf die lebendigen Zellen würde die toroidalen Nachbarschaften verändern. Das kompakte 3×3-Beispiel oben erklärt die Syntax; die mitgelieferte Glider-Datei verwendet ein 6×6-Feld.

## 5. Voraussetzungen und Build

Erforderlich sind ein C++11-fähiger Compiler, Make, X11-Header und -Bibliotheken sowie pthread. Python 3 wird nur für die Python-Tests benötigt.

- **macOS:** Das Makefile berücksichtigt XQuartz unter `/opt/X11`.
- **Linux:** Das Makefile verwendet die üblichen Compiler-Suchpfade; die X11-Entwicklungsdateien müssen vorhanden sein.
- **GUI:** Ein laufender X-Server und ein gültiges `DISPLAY` sind erforderlich.

```sh
make
make test
```

Weitere Build-Befehle:

| Befehl | Wirkung |
|---|---|
| `make` oder `make bin/main` | Programm übersetzen und linken |
| `make -j4` | Bis zu vier Build-Schritte parallel ausführen |
| `make depend` | Header-Abhängigkeiten erzeugen |
| `make clean` | Generierte Verzeichnisse `bin/` und `tmp/` entfernen |
| `make test` | Kernlogik, Thread-Verhalten und CLI prüfen |
| `make test-gui` | Zusätzliche Tests mit laufendem X-Server |

Die Quelldateiliste heißt im tatsächlichen Makefile `SRC_FILES`; das Aufgabenblatt verwendet dafür den Namen `CC_SOURCES`. Normale Builds erzeugen Abhängigkeiten automatisch. Änderungen an `.h` oder `.cc` erfordern einen erneuten Build; Änderungen an einer Eingabedatei in `boards/` nicht.

## 6. Programm verwenden

```sh
# Zufälliges Feld, 100 Schritte, keine GUI
./bin/main

# Glider nach 20 Schritten speichern
./bin/main -i boards/glider.rle -r 20 -o out.rle

# Glider mit grafischer Darstellung
./bin/main -i boards/glider.rle -r 100 -g

# Darstellung und Speicherung kombinieren
./bin/main -i boards/glider.rle -r 20 -g -o out.rle
```

| Parameter | Wirkung |
|---|---|
| `-i DATEI` | Generation 0 aus RLE laden |
| `-o DATEI` | Endzustand als RLE speichern |
| `-r N` | Nicht negative Anzahl von Schritten; Standard: 100 |
| `-g` | Grafische Ausgabe einschalten |
| `-h`, `--help` | Kurze Verwendungshinweise anzeigen |

Ohne Eingabedatei werden Breite und Höhe jeweils zwischen 50 und 150 gewählt. Jede Zelle lebt anfangs mit Wahrscheinlichkeit 0,25. Zum Wiederholen eines Zufallsversuchs kann Generation 0 gespeichert werden:

```sh
./bin/main -r 0 -o initial.rle
./bin/main -i initial.rle -r 100 -o final.rle
```

Die GUI wartet nach jedem gezeichneten Zustand 100 ms. Das Fenster schließt sich am Programmende. Wird es vorher geschlossen, läuft die Berechnung ohne weitere Darstellung bis zur angeforderten Schrittzahl weiter; eine angeforderte Ausgabe wird anschließend gespeichert.

Unter macOS kann XQuartz mit `open -a XQuartz` geöffnet werden. Bei „Could not open display“ müssen X-Server und `DISPLAY` geprüft werden. Ohne `-g` wird keine Display-Verbindung geöffnet; die X11-Bibliothek bleibt jedoch eine Laufzeitabhängigkeit des Programms.

## 7. Änderungen gegenüber der Vorgabe

| Datei oder Gruppe | Änderung |
|---|---|
| `include/board/LocalBoard.h`, `src/board/LocalBoard.cc` | Neu: konkrete Implementierung der Board-Schnittstelle mit Bereichs- und Größenprüfung. |
| `include/simulation/Life.h`, `src/simulation/Life.cc` | Neu: Zufallsbelegung, B3/S23, Wrap-around und Double Buffering. |
| `include/io/Rle.h`, `src/io/Rle.cc` | Neu: RLE-Ein-/Ausgabe, Wiederholungszahlen, Fehlerprüfung und CRLF-Unterstützung. |
| `src/main.cc` | Zeichenbeispiel durch vollständige Ablaufsteuerung mit CLI und optionaler GUI ersetzt. |
| `Makefile` | Neue Quellen, C++11, XQuartz-Pfade, automatische Abhängigkeiten und Testziele ergänzt. |
| `include/board/Board.h` | Nur die Reihenfolge der Konstruktorinitialisierung korrigiert; ursprüngliche Methodensignaturen beibehalten. |
| `include/thread/Thread.h` | Thread-Zustand initialisiert, Fehler bei der Erstellung behandelt und Join ohne gestarteten Thread verhindert. `cancel()` lässt den Zustand bis `join()` gesetzt, damit tatsächlich auf das Thread-Ende gewartet wird. |
| `include/gui/DrawingWindow.h` | Schließzustand als `std::atomic<bool>` zwischen Ereignis- und Hauptthread abgesichert. |
| `src/gui/DrawingWindow.cc` | GC-Zeiger auf `NULL` geprüft; temporäre Xlib-Daten freigegeben; Zeichenressourcen und Display erst nach Thread-Ende freigegeben; Hintergrundfüllung, Aktualisierungsbereich und Fehler-Exitcodes korrigiert. |
| `tests/` | Neu: automatisierte Kern-, CLI-, Thread- und GUI-Tests. |
| `README.md`, `.gitignore` | Deutsche Dokumentation und Ausschluss generierter Dateien ergänzt. |

Die RLE-Muster sowie `timer/` und `misc/` stammen unverändert aus der Vorgabe. Ursprüngliche Autorenangaben bleiben erhalten. Die ursprünglichen englischen Kommentare der Vorgabe bleiben erhalten. Deutsche Kommentare erläutern ausschließlich neu implementierte Funktionen und ergänzte oder korrigierte Codeabschnitte. Programmbezeichner und bestehende Konsolentexte wurden nicht vollständig übersetzt.

Bei der Dokumentationsprüfung wurde zusätzlich ein Fehler im zuvor ergänzten Join-Schutz gefunden: `cancel()` setzte `running` vor `join()` zurück, sodass der Join übersprungen wurde. Die Korrektur setzt den Zustand erst in `join()` zurück; `test_thread.cc` prüft ausdrücklich, dass `cancel()` auf die Aufräumroutine wartet.

## 8. Tests und bisherige Validierung

`make test` prüft:

- Board-Schnittstelle, ungültige Zugriffe und Abmessungen.
- Alle 512 lokalen 3×3-Konfigurationen für B3/S23.
- Block, Blinker über die Feldränder und die Verschiebung des Gliders nach vier Generationen.
- Eine unabhängige Modulo-Referenz für alle Board-Größen von 1×1 bis 9×9 über acht Schritte.
- RLE-Roundtrips für die fünf mitgelieferten Muster, das Aufgabenbeispiel, leere Boards, CRLF und fehlerhafte Eingaben.
- CLI-Standardwerte, Parameterfehler, `-r 0`, reproduzierbare Eingaben und Dateifehler.
- Das Warten auf das Thread-Ende bei `cancel()`.

`make test-gui` prüft die dargestellten Farben, normales Programmende, frühes Schließen per `WM_DELETE_WINDOW` und identische RLE-Ergebnisse mit und ohne GUI.

Die Simulation und die GUI wurden unter macOS mit Clang und XQuartz-Bibliotheken getestet; GUI-Tests liefen auf einem virtuellen Xvfb-Display. Für den C++-Kern wurden außerdem AddressSanitizer und UndefinedBehaviorSanitizer erfolgreich eingesetzt. Ein Linux-Build und eine vollständige manuelle Desktop-Prüfung wurden bisher nicht durchgeführt.

Die Sanitizer-Prüfung kann separat wiederholt werden:

```sh
make directories
clang++ -Iinclude -std=c++11 -g -fsanitize=address,undefined \
  -fno-omit-frame-pointer tests/test_life.cc src/board/LocalBoard.cc \
  src/io/Rle.cc src/simulation/Life.cc -o bin/test_sanitized
./bin/test_sanitized
```

## 9. Hinweise für GitHub und zum Lesen des Codes

Für ein eigenständiges Repository den Inhalt dieses Verzeichnisses übernehmen, einschließlich `include/`, `src/`, `boards/`, `tests/`, `Makefile`, `README.md` und `.gitignore`. `README_VI.md` ist eine optionale ergänzende Erklärung. Die `.gitignore` schließt Programme, Objektdateien, Python-Caches und typische lokale Ausgabedateien aus. Eingabemuster in `boards/` werden weiterhin versioniert.

Empfohlene Lesereihenfolge:

1. `Board.h`: Welche Schnittstelle verlangt die Vorgabe?
2. `LocalBoard.h/.cc`: Wie werden Zellen gespeichert und angesprochen?
3. `Life.cc`: Wie entstehen Nachbarschaften und die nächste Generation?
4. `main.cc`: Wie werden die Komponenten verbunden?
5. `Rle.cc`: Wie wird zwischen Textformat und Board umgewandelt?
6. `Makefile` und `tests/`: Wie wird das Projekt gebaut und überprüft?
