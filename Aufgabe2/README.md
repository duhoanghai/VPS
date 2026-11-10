# VPS-I – Aufgabe 2: Verteiltes Game of Life (UDP und TCP)

C++11-Lösung zu **Blatt 2 / Aufgabe 2** in *Verteilte Parallele Systeme 1*. Die Anwendung setzt das **Game of Life aus Aufgabe 1** als **Client-Server-System** um: Der `BoardServer` besitzt die zwei Spielfelder, jeder `LifeClient` berechnet nur seinen zugewiesenen Ausschnitt. Die Daten werden über ein gemeinsames `IPNetwork`-Interface wahlweise mit **UDP** oder **TCP** übertragen.

Diese README erklärt die Aufgabe, die Architektur, die Änderungen an der Vorgabe sowie den Build, die Bedienung und die automatisierten Tests. Alle Befehle werden **im Ordner `Aufgabe2/`** ausgeführt. `Aufgabe1/` bleibt unangetastet.

## 1. Was verlangt Blatt 2?

**Teil a – UDP**

- `UDPNetwork` mit `socket`, `bind`, `sendto`, `recvfrom`, `select` und erneutem Versand nach Timeout.
- Synchrone `request`/`reply`-Kommunikation; eine Anfrage bleibt bis zur passenden Antwort offen.
- Jede Nachricht besitzt einen **Diskriminator** (`type`); die Antwort enthält die unveränderte **Sequenznummer** der Anfrage.
- Anmeldung (`logon`), Verteilung des Spielfelds, Lesen und Schreiben einzelner Zellen über das Netz.
- **Barrierensynchronisation**: Erst wenn alle Clients einen Schritt abgeschlossen haben, beginnt die nächste Generation.
- Ein `timestep` in der Nachricht schützt gegen verspätete Barrierenantworten.

**Teil b – TCP**

- `TCPNetwork` mit `socket`, `bind`, `listen`, `accept`, `connect`, `read`/`recv`, `write`/`send`, `close`.
- **Für jede Client-Anfrage eine neue TCP-Verbindung**, nach der Antwort wird sie geschlossen.
- Bei der TCP-Barriere antwortet der Server **sofort** mit `WAIT`, sofern noch Clients rechnen. Der Client fragt später erneut. Das vermeidet das Offenhalten alter Verbindungen.

**Ergebnisprüfung**

- Deterministische Startbelegung benutzen und den Endzustand nach derselben Schrittzahl vergleichen.
- Laufzeit von Lokal-, UDP- und TCP-Variante messen. Die Netzwerkvarianten sind absichtlich ineffizient: Jeder einzelne Zellzugriff erzeugt eine Anfrage, weil die Übung die Kommunikation in den Mittelpunkt stellt.

## 2. Projektstruktur

```text
Aufgabe2/
├── README.md                   Diese Dokumentation
├── README_VI.md                Erklärung auf Vietnamesisch
├── Makefile                    Build und Integrationstests
├── .gitignore
├── include/
│   ├── board/
│   │   ├── Board.h             Board-Interface aus der Vorgabe
│   │   ├── LocalBoard.h        Wiederverwendeter Code aus Aufgabe 1
│   │   └── BoardServer.h       Server-Schnittstelle
│   ├── client/LifeClient.h     Client-Schnittstelle
│   ├── net/
│   │   ├── IPAddress.h         Adress-Hilfsklasse aus der Vorgabe
│   │   ├── IPNetwork.h         Gemeinsame Schnittstelle
│   │   ├── Protocol.h          Nachrichtenformat und Nachrichtentypen
│   │   ├── UDPNetwork.h
│   │   └── TCPNetwork.h
│   ├── io/Rle.h                RLE-Funktionen aus Aufgabe 1
│   └── simulation/Life.h      Lokaler Referenzalgorithmus
├── src/
│   ├── server.cc               Startet den Server
│   ├── client.cc               Startet einen Client
│   ├── local.cc                Lokale Referenz für Vergleich
│   ├── board/{BoardServer,LocalBoard}.cc
│   ├── client/LifeClient.cc
│   ├── net/{UDPNetwork,TCPNetwork}.cc
│   ├── io/Rle.cc
│   └── simulation/Life.cc
├── boards/glider.rle           Deterministische Startbelegung
└── tests/test_integration.py  Local-/UDP-/TCP-Vergleich
```

Die originalen `.brd`-Muster sowie die GUI-/Timer-Header aus der **Vorgabe 2** bleiben ebenfalls erhalten. Die Kernprogramme brauchen keinen X11-Server und starten ohne GUI. `bin/` enthält ausschließlich generierte Programme (`server`, `client`, `local`).

## 3. Kommunikationsmodell

```text
                BoardServer
            ┌──────────────────┐
            │ current_ (Lesen) │
            │ next_    (Schreiben)
            │ Client-Liste     │
            │ barrier()        │
            └────────┬─────────┘
                     │ IPNetwork (UDP oder TCP)
          ┌──────────┼──────────┐
          │          │          │
       LifeClient  LifeClient  LifeClient
         x=0..1     x=2..3     x=4..5
```

Der Server kennt beim Start die erwartete Client-Zahl. Die Aufteilung erfolgt in **Spaltenstreifen**. Jeder Client bekommt `[x1, x2)` und `[y1, y2)`; das rechte/untere Ende gehört nicht mehr zum Ausschnitt. Der Client liest über `READ` auch Nachbarzellen außerhalb des eigenen Streifens, schreibt aber mit `WRITE` **nur in seinen eigenen Bereich**.

Jeder Client berechnet die Regeln **B3/S23** wie Aufgabe 1. Auch die **toroidalen Ränder** (Wrap-around) sind identisch. `current_` wird während eines Zeitschritts nicht verändert; `next_` sammelt neue Werte. Erst die Barriere tauscht die beiden Board-Zeiger. So vermischen sich die Generationen nicht.

### Beispiel für einen Schritt

1. Client sendet `LOGON`; Server vergibt eine eindeutige Client-Nummer und einen Bereich.
2. Client fragt für jede Zelle per `READ` ihren Zustand und die acht Nachbarn ab.
3. Client berechnet den neuen Zustand und sendet `WRITE` für diese Zelle.
4. Nach allen eigenen Zellen sendet der Client `BARRIER` mit seiner `timestep`-Nummer.
5. Der Server prüft, ob **alle angemeldeten, erwarteten Clients** diese Barriere erreicht haben.
6. Sobald alle fertig sind, tauscht der Server die beiden Boards; danach dürfen Clients mit der nächsten Generation fortfahren.
7. Nach der letzten Generation liefert die Barriere `FINISHED`. Die Clients senden `QUIT`, danach schreibt der Server das RLE-Ergebnis und beendet sich.

### Nachrichtenformat (`Protocol.h`)

`Message` ist ein gemeinsames Paket mit festem Layout. Wichtige Felder:

| Feld | Bedeutung |
|---|---|
| `type` | **Diskriminator**: `LOGON`, `READ`, `WRITE`, `BARRIER`, `QUIT`, `RESPONSE` |
| `sequence` | Pro Client hochgezählte Sequenznummer; Antwort übernimmt sie unverändert |
| `timestep` | Zugehörige Generation (0-basiert) |
| `token` | Stabile Kennung, damit TCP-Verbindungen wiedererkannt und `LOGON`-Wiederholungen erkannt werden |
| `clientId` | Vom Server vergebene Nummer |
| `x`, `y`, `value` | Zellkoordinaten und Zustand (0 = tot, 1 = lebendig) |
| `x1`, `x2`, `y1`, `y2` | Zugewiesener Bereich, Ende exklusiv |
| `width`, `height` | Abmessungen des vollständigen Boards |
| `status` | `OK`, `WAIT`, `FINISHED` oder `FAILURE` |

Die Aufgabenstellung erlaubt, die Byteordnung zwischen verschiedenen Architekturen zu ignorieren. Dieses vereinfachte Nachrichtenformat ist daher **nicht** als allgemeines, portables Internetprotokoll gedacht. Für große Felder und sehr lange Sequenzen wären zusätzliche Prüfungen und ein definiertes Serialisierungsformat sinnvoll.

### UDP: Timeout, Sequenznummer und Barriere

`UDPNetwork::request()` sendet ein Datagramm und wartet mit `select()` auf eine Antwort. Bei Timeout wird **dieselbe Nachricht mit derselben `sequence`** erneut versendet. Antworten von einer anderen IP/Port-Kombination, mit falschem Diskriminator oder veralteter Sequenznummer werden ignoriert.

Bei `BARRIER` speichert der Server die noch unbeantworteten UDP-Anfragen. Erst wenn alle Clients fertig sind, verschickt `notifyAll()` die dazugehörigen Antworten. Eine erneut eintreffende, bereits abgeschlossene Barriere bekommt ihre Abschlussantwort nochmals, ohne eine Generation erneut auszulösen.

### TCP: neue Verbindung pro Anfrage

`TCPNetwork::request()` erstellt für jede Anfrage einen Socket, verbindet sich, überträgt eine vollständige Nachricht, liest eine vollständige Antwort und schließt die Verbindung. Der Server `accept()`-iert jede Verbindung einzeln und schließt sie nach `reply()`.

**Besonderheit der TCP-Barriere:** Da der Server in dieser einfachen Lösung nacheinander Anfragen bearbeitet, darf er nicht auf einer offenen TCP-Verbindung warten, bis alle anderen Clients eintreffen. Er schickt `WAIT`. Der Client wartet kurz und sendet eine neue `BARRIER`-Anfrage über eine **neue** TCP-Verbindung. Nach dem Board-Wechsel lautet die Antwort `OK` bzw. `FINISHED`.

## 4. Voraussetzungen und Build

Erforderlich: **Linux/Unix**, `g++` mit C++11-Unterstützung, GNU Make, pthread/POSIX-Sockets sowie Python 3 für die Integrationstests.

```sh
cd Aufgabe2
make
make test
```

| Befehl | Ergebnis |
|---|---|
| `make` | Erstellt `bin/server`, `bin/client`, `bin/local` |
| `make test` | Prüft UDP/TCP mit einem oder mehreren Clients gegen die lokale Version |
| `make clean` | Entfernt `bin/` |
| `./bin/server --help` | Server-Parameter anzeigen |
| `./bin/client --help` | Client-Parameter anzeigen |
| `./bin/local --help` | Lokale Referenz-Parameter anzeigen |

**macOS:** Der ursprüngliche Code nutzt POSIX-Sockets, die auch unter macOS vorhanden sind. Das bereitgestellte Makefile ist allerdings für `g++` konfiguriert und wurde hier auf Linux getestet, **nicht** auf diesem Mac. Für den Netzwerkvergleich ist keine XQuartz-Installation nötig.

## 5. Programm starten – UDP

Öffne **drei Terminalfenster**. In allen Fenstern gilt `cd Aufgabe2`.

**Terminal 1 – Server:**

```sh
./bin/server --protocol udp --port 7654 --clients 2 --steps 10 \
  --input boards/glider.rle --output udp.rle
```

**Terminal 2 – erster Client:**

```sh
./bin/client --protocol udp --host 127.0.0.1 --port 7654
```

**Terminal 3 – zweiter Client:**

```sh
./bin/client --protocol udp --host 127.0.0.1 --port 7654
```

Der Server wartet auf **genau zwei** Clients. Bei nur einem Client kann die Barriere nicht freigegeben werden. Die beiden Clients dürfen in beliebiger Reihenfolge gestartet werden, aber zunächst muss der Server laufen.

## 6. TCP starten und Ergebnisse vergleichen

Danach den Server und zwei Clients mit `--protocol tcp` statt `udp` starten (am besten einen anderen Port benutzen):

```sh
# Terminal 1
./bin/server --protocol tcp --port 7655 --clients 2 --steps 10 \
  --input boards/glider.rle --output tcp.rle

# Terminal 2 & Terminal 3 (jeweils einmal ausfuehren)
./bin/client --protocol tcp --host 127.0.0.1 --port 7655
```

Zusätzlich die lokale Version starten:

```sh
./bin/local --input boards/glider.rle --steps 10 --output local.rle
cmp local.rle udp.rle
cmp local.rle tcp.rle
```

Wenn `cmp` **nichts ausgibt**, stimmen die RLE-Ausgabedateien bytegenau überein. `make test` automatisiert genau diese Art von Vergleich. Die Laufzeiten geben Server und lokale Referenz nach Abschluss in **Sekunden** aus. Bei TCP fallen deutlich mehr Verbindungsaufbauten an; Zeiten hängen stark von Betriebssystem, Board-Größe und Client-Anzahl ab.

### Deterministische Startbelegung ohne Eingabedatei

Server und lokaler Referenzlauf benutzen denselben Mersenne-Twister-Seed und dieselbe B3/S23-Regel:

```sh
./bin/local --width 12 --height 9 --seed 42 --steps 20 --output local.rle
./bin/server --protocol udp --clients 2 --width 12 --height 9 \
  --seed 42 --steps 20 --output udp.rle
# Danach zwei UDP-Clients starten und mit cmp vergleichen.
```

`--steps 0` bedeutet gemäß der Vorgabe unendlich viele Schritte. In diesem Modus endet der Server nicht von selbst und schreibt erst beim regulären Abschluss eine Ausgabedatei; für reproduzierbare Messungen ist deshalb eine positive Schrittzahl empfehlenswert.

## 7. Weshalb die einzelnen Klassen existieren

| Datei / Klasse | Aufgabe |
|---|---|
| `Board` | Abstraktes Interface: Lesen und Schreiben von Zellen |
| `LocalBoard` | Speichert Zellzustände im Speicher; aus Aufgabe 1 übernommen |
| `BoardServer` | Registrierung, Aufteilung, Read/Write und Barrierensynchronisation |
| `LifeClient` | Rechnet Game-of-Life-Regel für seinen Bereich aus |
| `IPAddress` | IPv4-Adresse aus Hostname und Port |
| `IPNetwork` | Einheitliches Interface für `request`, `receive`, `reply` |
| `UDPNetwork` | UDP-Sockets, `select`, Timeout und Wiederholung |
| `TCPNetwork` | TCP-Sockets und separate Verbindungen pro Anfrage |
| `Protocol.h` | Nachrichtenformat mit Typ, Sequenznummer und Generation |
| `Rle` | Deterministische Ein-/Ausgabe eines Spielfelds |
| `Life` | Unabhängige lokale Referenzrechnung für die Ergebnisprüfung |

Die wichtigsten Änderungen an der **bereitgestellten Vorgabe 2** sind die öffentliche Vererbung von `UDPNetwork`, die Implementierungen aller noch leeren Methoden, der zusätzliche `TCPNetwork`-Typ, das explizite Nachrichtenformat, die ausführbaren Hauptprogramme und die Integrationstests. Hilfs- und GUI-Dateien, die das Netzwerkprogramm nicht benötigt, sind kein Bestandteil des Build-Ziels.

## 8. Tests und Einschränkungen

`make test` führt denselben deterministischen Glider mit **1 und 3 Clients** und mehreren Schritten für beide Protokolle aus. Es vergleicht die RLE-Dateien byteweise gegen die lokale Referenz. Die Build- und Integrationstests wurden hier **unter Linux** erfolgreich ausgeführt.

Für die Aufgabenstellung sind folgende Grenzen wichtig:

- Die UDP-Übertragung ist **Best Effort**, mit Wiederholung bei Timeout, aber keine vollständige Exactly-Once-Garantie für beliebige Netzwerkfehler oder Server-Abstürze.
- Ein Client, der dauerhaft ausfällt, blockiert die Barriere; dynamischer Ersatz oder Rebalancing ist nicht implementiert.
- TCP arbeitet bewusst mit einer Verbindung pro Zellanfrage statt einem langlebigen Stream, wie im Aufgabenblatt verlangt.
- Der `QUIT`-Abschluss benötigt noch eine erfolgreiche Antwort; ein verlorenes letztes Datagramm beim Abschalten des Servers wird nicht durch einen speziellen Shutdown-Handshake abgesichert.
- Die Codebasis benutzt IPv4 und ein festes binäres Paketlayout; gemischte Architekturen, TLS und Authentifizierung gehören nicht zum Aufgabenblatt.
- Der lokale Vergleich benutzt dieselben geprüften Game-of-Life-Regeln wie Aufgabe 1. Für lange Laufzeittests (z. B. 10.000 Schritte) kann ein kleines Board gewählt werden; verteilte Zellzugriffe sind absichtlich langsam.

## 9. Empfohlene Lesereihenfolge

1. [`Protocol.h`](include/net/Protocol.h): Was steht in jeder Nachricht?
2. [`IPNetwork.h`](include/net/IPNetwork.h): Welche Funktionen müssen beide Netzklassen anbieten?
3. [`UDPNetwork.cc`](src/net/UDPNetwork.cc): Wie funktionieren Datagramme und Timeout?
4. [`TCPNetwork.cc`](src/net/TCPNetwork.cc): Warum braucht jede Anfrage eine neue Verbindung?
5. [`BoardServer.cc`](src/board/BoardServer.cc): Registrierung, Schreibschutz und Barriere.
6. [`LifeClient.cc`](src/client/LifeClient.cc): Berechnung einzelner Zellen über das Netzwerk.
7. [`server.cc`](src/server.cc), [`client.cc`](src/client.cc): Programmargumente und Start der Objekte.
8. [`local.cc`](src/local.cc) und [`tests/test_integration.py`](tests/test_integration.py): Ergebnis- und Laufzeitvergleich.
