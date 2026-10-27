// Optionaler X11-Integrationstest: Pixel prüfen, danach den Schließen-Knopf simulieren.
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <chrono>
#include <thread>
#include <cstring>
#include <fstream>
#include <iostream>

namespace {
// Ein Window-Manager kann das Anwendungsfenster in ein Rahmenfenster einbetten.
// Deshalb den gesamten Fensterbaum statt nur der direkten Root-Kinder durchsuchen.
Window findWindow(Display* display, Window candidate) {
    char* name = nullptr;
    XFetchName(display, candidate, &name);
    const bool matches = name && std::strcmp(name, "VPS-I Game of Life") == 0;
    if (name) XFree(name);
    if (matches) return candidate;
    Window root, parent, *children = nullptr;
    unsigned count = 0;
    if (!XQueryTree(display, candidate, &root, &parent, &children, &count)) return 0;
    Window found = 0;
    for (unsigned i = 0; i < count && !found; ++i)
        found = findWindow(display, children[i]);
    if (children) XFree(children);
    return found;
}

int report(Display* display, int code, const char* message) {
    std::cerr << "GUI-Test: " << message << " (Code " << code << ")\n";
    if (display) XCloseDisplay(display);
    return code;
}
}

int main(int argc, char** argv) {
    Display* display = XOpenDisplay(nullptr);
    if (!display) return report(nullptr, 1, "X-Display nicht erreichbar; DISPLAY und XQuartz prüfen.");
    // Fremde Fenster können während der Suche verschwinden. BadWindow ist dann
    // unkritisch; andere X11-Fehler weiterhin ausgeben.
    XSetErrorHandler([](Display* d, XErrorEvent* e) {
        if (e->error_code != BadWindow) {
            char message[256];
            XGetErrorText(d, e->error_code, message, sizeof(message));
            std::cerr << "GUI-Test X11: " << message << '\n';
        }
        return 0;
    });
    Window found = 0;
    for (int attempt = 0; attempt < 500 && !found; ++attempt) {
        found = findWindow(display, DefaultRootWindow(display));
        if (!found) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!found) return report(display, 2, "Game-of-Life-Fenster nicht gefunden.");
    bool verified = false;
    for (int attempt=0; attempt<100 && !verified; ++attempt) {
        XWindowAttributes attributes;
        if (!XGetWindowAttributes(display, found, &attributes))
            return report(display, 2, "Fenster während der Prüfung geschlossen.");
        if (attributes.map_state != IsViewable) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (attributes.width != 72 || attributes.height != 72)
            return report(display, 3, "Erwartete Fenstergröße: 72 x 72 Pixel.");
        XImage* image = XGetImage(display, found, 0, 0, 72, 72, AllPlanes, ZPixmap);
        if (!image) return report(display, 4, "Fensterpixel nicht lesbar.");
        // XQuartz kann zusätzliche Bits außerhalb der RGB-Masken setzen.
        // Nur die tatsächlichen Farbkanäle vergleichen, nicht das gesamte Pixelwort.
        const unsigned long rgbMask = image->red_mask | image->green_mask | image->blue_mask;
        int alive = 0, dead = 0;
        for (int y=0;y<6;++y) for(int x=0;x<6;++x) {
            unsigned long pixel = XGetPixel(image,x*12+6,y*12+6) & rgbMask;
            alive += pixel == image->green_mask;
            dead += pixel == 0;
        }
        verified = alive == 5 && dead == 31;
        if (verified && argc > 1) {
            std::ofstream out(argv[1], std::ios::binary);
            out << "P6\n72 72\n255\n";
            for(int y=0;y<72;++y) for(int x=0;x<72;++x) {
                unsigned long pixel = XGetPixel(image,x,y);
                out.put((pixel>>16)&255); out.put((pixel>>8)&255); out.put(pixel&255);
            }
        }
        XDestroyImage(image);
        if (!verified) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!verified) return report(display, 5, "Erwartet: 5 grüne und 31 schwarze Zellen. Fenster sichtbar und unverdeckt halten.");
    XEvent event = {};
    event.xclient.type = ClientMessage;
    event.xclient.window = found;
    event.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", False);
    event.xclient.format = 32;
    event.xclient.data.l[0] = XInternAtom(display, "WM_DELETE_WINDOW", False);
    if (!XSendEvent(display, found, False, NoEventMask, &event))
        return report(display, 6, "Schließanforderung konnte nicht gesendet werden.");
    XFlush(display);
    XCloseDisplay(display);
    std::cout << "PASS: GUI renders 5 green/31 black cells and accepts WM_DELETE_WINDOW\n";
}
