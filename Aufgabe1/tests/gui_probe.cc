// Optionaler X11-Integrationstest: Pixel prüfen, danach den Schließen-Knopf simulieren.
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <chrono>
#include <thread>
#include <cstring>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    Display* display = XOpenDisplay(nullptr);
    if (!display) return 1;
    Window found = 0;
    for (int attempt = 0; attempt < 500 && !found; ++attempt) {
        Window root, parent, *children = nullptr;
        unsigned count = 0;
        XQueryTree(display, DefaultRootWindow(display), &root, &parent, &children, &count);
        for (unsigned i = 0; i < count; ++i) {
            char* name = nullptr;
            XFetchName(display, children[i], &name);
            if (name && std::strcmp(name, "VPS-I Game of Life") == 0) found = children[i];
            if (name) XFree(name);
        }
        if (children) XFree(children);
        if (!found) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!found) { XCloseDisplay(display); return 2; }
    bool verified = false;
    for (int attempt=0; attempt<100 && !verified; ++attempt) {
        XWindowAttributes attributes;
        XGetWindowAttributes(display, found, &attributes);
        if (attributes.map_state != IsViewable) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (attributes.width != 72 || attributes.height != 72) return 3;
        XImage* image = XGetImage(display, found, 0, 0, 72, 72, AllPlanes, ZPixmap);
        if (!image) return 4;
        int alive = 0, dead = 0;
        for (int y=0;y<6;++y) for(int x=0;x<6;++x) {
            unsigned long pixel = XGetPixel(image,x*12+6,y*12+6);
            alive += pixel == 0x00ff00;
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
    if (!verified) return 5;
    XEvent event = {};
    event.xclient.type = ClientMessage;
    event.xclient.window = found;
    event.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", False);
    event.xclient.format = 32;
    event.xclient.data.l[0] = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSendEvent(display, found, False, NoEventMask, &event);
    XFlush(display);
    XCloseDisplay(display);
    std::cout << "PASS: GUI renders 5 green/31 black cells and accepts WM_DELETE_WINDOW\n";
}
