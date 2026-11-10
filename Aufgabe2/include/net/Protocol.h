#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <cstdint>

// Alle Nachrichten haben eine feste Groesse; fuer Blatt 2 sind gleiche Rechner/ABIs erlaubt.
enum MessageType : uint32_t { LOGON=1, READ=2, WRITE=3, BARRIER=4, QUIT=5, RESPONSE=6 };
enum ResultCode : uint32_t { OK=0, WAIT=1, FINISHED=2, FAILURE=3 };

struct Message {
    uint32_t type = 0;        // Diskriminator: bestimmt die Art der Nachricht.
    uint32_t sequence = 0;    // UDP: Antwort muss die Sequenznummer der Anfrage tragen.
    uint32_t timestep = 0;    // Generation, zu der die Anfrage gehoert.
    uint32_t token = 0;       // Stabile Client-ID auch bei wechselnden TCP-Verbindungen.
    uint32_t clientId = 0;
    int32_t x = 0, y = 0, value = 0;
    int32_t x1 = 0, x2 = 0, y1 = 0, y2 = 0;  // Exklusives Ende der Region.
    int32_t width = 0, height = 0;
    uint32_t status = OK;
};
#endif
