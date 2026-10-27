/*
 * DrawingWindow.cpp
 *
 *  Erstellt am: 13.09.2009
 *      Autor: mn
 */

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <X11/Xutil.h>
#include "gui/DrawingWindow.h"
#include "misc/debug.h"

using namespace GUI;

DrawingWindow::DrawingWindow(unsigned int width, unsigned int height, string name, bool createThread, bool destroyable)
	: width(width), height(height), name(name), graphicsContext(NULL), destroyable(destroyable), destroyed(false)
{
	// Thread-Unterstützung von Xlib aktivieren
	XInitThreads();

	// Verbindung zum X-Display öffnen
	display = XOpenDisplay(NULL);

	// Fehler prüfen
	if(display == 0)
	{
		cerr << "DrawingWindow::DrawingWindow(..), Error: Could not open display." << endl;
		exit(EXIT_FAILURE);
	}

	// Fenster mit Position, Größe, Rand und Hintergrund erzeugen
	window = XCreateSimpleWindow(display, RootWindow(display, 0), 1, 1, width, height, 0, BlackPixel(display, 0), BlackPixel(display, 0));

	// Fehler prüfen
	if(window == 0)
	{
		cerr << "DrawingWindow::DrawingWindow(..), Error: Could not create window." << endl;
		exit(EXIT_FAILURE);
	}

	// Fenstertitel setzen
	XTextProperty nameProperty;
	size_t nameSize = name.size();
	char *nameCString = new char[nameSize + 1]; // + 1 für das abschließende Nullzeichen
	name.copy(nameCString, nameSize, 0);
	nameCString[nameSize] = 0; // Zeichenkette mit Nullzeichen abschließen

	XStringListToTextProperty((char**) &nameCString, 1, &nameProperty);
	XSetWMName(display, window, &nameProperty);

	delete [] nameCString;
	// Von Xlib reservierte Hilfsdaten nach der Übergabe wieder freigeben.
	XFree(nameProperty.value);

	// Feste Fenstergröße vorgeben, passend zum Pixelpuffer
	XSizeHints* sizeHints = XAllocSizeHints();

	sizeHints->flags = PMinSize | PMaxSize;
	sizeHints->max_height = height;
	sizeHints->min_height = height;
	sizeHints->max_width = width;
	sizeHints->min_width = width;

	XSetWMNormalHints(display, window, sizeHints);
	XFree(sizeHints);

	// Fenster auf dem Display sichtbar machen
	XMapWindow(display, window);

	// Gepufferte X11-Anfragen an den Server senden
	XFlush(display);

	// Grafikkontext vorbereiten

	// Zeichenattribute festlegen
	XGCValues values;
	values.foreground = WhitePixel(display, 0);
	values.background = BlackPixel(display, 0);
	values.line_width = 1;
	values.line_style = LineSolid;
	values.cap_style = CapRound;
	values.join_style = JoinRound;
	values.fill_style = FillSolid;
	values.graphics_exposures = False; // True erzeugt durch XCopyArea viele zusätzliche Ereignisse

	// Zu übernehmende Attribute des Grafikkontexts auswählen
	unsigned long valueMask = GCForeground | GCBackground | GCLineWidth	| GCLineStyle | GCCapStyle | GCJoinStyle | GCFillStyle | GCGraphicsExposures;

	// Grafikkontext erzeugen
	graphicsContext = XCreateGC(display, window, valueMask, &values);
	// GC ist ein Zeiger; ein Fehler wird mit NULL statt mit einem negativen Wert gemeldet.
	if(graphicsContext == NULL) {
		cerr << "DrawingWindow::DrawingWindow(..), Error: Could not create graphics context." << endl;
		exit(EXIT_FAILURE);
	}

	// Pixmap als Zeichenpuffer erzeugen
	pixmap = XCreatePixmap(display, window, width, height, DefaultDepth(display, 0));

	// Grafikkontext für den Pixelpuffer erzeugen
	graphicsContextPixmap = XCreateGC(display, pixmap, valueMask, &values);
	if(graphicsContextPixmap == NULL) {
		cerr << "DrawingWindow::DrawingWindow(..), Error: Could not create graphics context for the pixmap." << endl;
		exit(EXIT_FAILURE);
	}

	// Zusätzlichen Grafikkontext zum Füllen des Pixelpuffers erzeugen
	graphicsContextPixmapFill = XCreateGC(display, pixmap, valueMask, &values);
	if(graphicsContextPixmapFill == NULL) {
		cerr << "DrawingWindow::DrawingWindow(..), Error: Could not create graphics context for the pixmap for filling." << endl;
		exit(EXIT_FAILURE);
	}

	// Standardattribute setzen
	XSetForeground(display, graphicsContextPixmapFill, BlackPixel(display, 0));

	// Fenster und Pixelpuffer leeren
	clear();

	// Thread erzeugen und starten, der run() ausführt
	if(createThread)
		Thread::create();
}

DrawingWindow::~DrawingWindow()
{
	Thread::cancel(); // Ereignisthread abbrechen und dessen Ende abwarten

	// Ab hier ist keine weitere Synchronisation mit dem Ereignisthread nötig
	// Der Ereignisthread ist beendet; Display und Zeichenressourcen sind noch gültig.
	XFreePixmap(display, pixmap);
	XFreeGC(display, graphicsContext);
	XFreeGC(display, graphicsContextPixmap);
	XFreeGC(display, graphicsContextPixmapFill);
	destroyWindow();
}

void DrawingWindow::destroyWindow()
{
	// Display-Verbindung schließen
	destroyed = true;

	XCloseDisplay(display);
	_DEBUG ( cout << "DrawingWindow::destroyWindow(), Info: Window destroyed. " << endl ) ;
}

void DrawingWindow::run()
{
	if(destroyed) return; // Bei geschlossenem Fenster nichts mehr zeichnen

	// Datenstruktur für ein Ereignis aus der Warteschlange
	XEvent event;

	// Gewünschte Ereignistypen auswählen
XLockDisplay(display);
	XSelectInput(display, window, ExposureMask | KeyPressMask | ButtonPressMask);

	// WM_DELETE_WINDOW registrieren, um Schließanforderungen abzufangen
	// Der Fenstermanager sendet diese beim Betätigen der Schließen-Schaltfläche.
	Atom wmDeleteWindowAtom = XInternAtom(display, "WM_DELETE_WINDOW", false);
	XSetWMProtocols(display, window, &wmDeleteWindowAtom, 1); // Erwartet ein Array von Atomen und dessen Länge
XUnlockDisplay(display);

	// Ereignisse bis zur Schließanforderung verarbeiten
	while(!destroyed)
	{
		Thread::testCancel(); // Definierter Abbruchpunkt für pthread_cancel
		usleep(100);

		// Nächstes Ereignis abfragen
	XLockDisplay(display);
		bool cont = !XPending(display);
	XUnlockDisplay(display);

		if(cont) // Vorab prüfen, damit der folgende Aufruf nicht blockiert
		{
			continue;
		}


	XLockDisplay(display);
		XNextEvent(display, &event); // Blockiert, falls kein Ereignis vorliegt
	XUnlockDisplay(display);


		// Ereignistyp auswerten
		switch(event.type) {
		case Expose:
			_DEBUG ( cout << "DrawingWindow::run(), Info: Expose message received. " << endl ) ;
			onExpose(event.xexpose.x, event.xexpose.y, event.xexpose.width, event.xexpose.height);
			break;
		case ClientMessage:
			_DEBUG ( cout << "DrawingWindow::run(), Info: Delete Window request received. " << endl ) ;
			if(destroyable && static_cast<Atom>(event.xclient.data.l[0]) == wmDeleteWindowAtom)
				destroyed = true;
			break;
		default:
			_DEBUG ( cout << "DrawingWindow::run(), Warning: Unhandeled event.type: " << event.type << endl; )
			break;
	    }
	}

	// Bei einer Schließanforderung endet hier der Ereignisthread.
	// Erst der Destruktor schließt das Display, nachdem dieser Thread beendet ist.
	// Im Hauptthread können noch Zeichenaufrufe abgeschlossen werden.
}

void DrawingWindow::onExpose(unsigned int xPos, unsigned int yPos, unsigned int width, unsigned int height) const
{
	if(destroyed) return; // Bei geschlossenem Fenster nichts mehr zeichnen

XLockDisplay(display);
	XCopyArea(display, pixmap, window, graphicsContext, xPos, yPos, width, height, xPos, yPos);
	XFlush(display);
XUnlockDisplay(display);
}

void DrawingWindow::setBackgroundColor(RGBColor color)
{
	if(destroyed) return; // Bei geschlossenem Fenster nichts mehr zeichnen

XLockDisplay(display);
	XSetBackground(display, graphicsContextPixmap, color);
	// clear() verwendet diesen Füllkontext und muss die neue Hintergrundfarbe kennen.
	XSetForeground(display, graphicsContextPixmapFill, color);
XUnlockDisplay(display);
}

void DrawingWindow::setForegroundColor(RGBColor color)
{
	if(destroyed) return; // Bei geschlossenem Fenster nichts mehr zeichnen

XLockDisplay(display);
	XSetForeground(display, graphicsContextPixmap, color);
XUnlockDisplay(display);
}

void DrawingWindow::drawPoint(unsigned int xPos, unsigned int yPos) const
{
	if(destroyed) return; // Bei geschlossenem Fenster nichts mehr zeichnen
XLockDisplay(display);
	XDrawPoint(display, pixmap, graphicsContextPixmap, xPos, yPos);
XUnlockDisplay(display);
	onExpose(xPos, yPos, 1, 1);
}

void DrawingWindow::drawLine(unsigned int xPosStart, unsigned int yPosStart, unsigned int xPosEnd, unsigned int yPosEnd) const
{
	if(destroyed) return; // Bei geschlossenem Fenster nichts mehr zeichnen

XLockDisplay(display);
	XDrawLine(display, pixmap, graphicsContextPixmap, xPosStart, yPosStart, xPosEnd, yPosEnd);
XUnlockDisplay(display);
	onExpose(0, 0, width, height); // Eine Optimierung könnte nur das umschließende Rechteck der Linie aktualisieren.
}

void DrawingWindow::drawRectangle(unsigned int xPos, unsigned int yPos, unsigned int width, unsigned int height) const
{
	if(destroyed) return; // Bei geschlossenem Fenster nichts mehr zeichnen

XLockDisplay(display);
	XDrawRectangle(display, pixmap, graphicsContextPixmap, xPos, yPos, width, height);
XUnlockDisplay(display);
	onExpose(xPos, yPos, width + 1, height + 1); // Ein zusätzliches Pixel berücksichtigt die Randlinie
}

void DrawingWindow::drawFilledRectangle(unsigned int xPos, unsigned int yPos, unsigned int width, unsigned int height) const
{
	if(destroyed) return; // Bei geschlossenem Fenster nichts mehr zeichnen

XLockDisplay(display);
	XFillRectangle(display, pixmap, graphicsContextPixmap, xPos, yPos, width, height);
XUnlockDisplay(display);
	onExpose(xPos, yPos, width, height);
}

void DrawingWindow::drawText(unsigned int xPos, unsigned int yPos, string text)
{
	if(destroyed) return; // Bei geschlossenem Fenster nichts mehr zeichnen

XLockDisplay(display);
	XDrawString(display, pixmap, graphicsContextPixmap, xPos, yPos, text.c_str(), text.length());
XUnlockDisplay(display);
	onExpose(0, 0, width, height);
}

void DrawingWindow::clear() const
{
	if(destroyed) return; // Bei geschlossenem Fenster nichts mehr zeichnen

XLockDisplay(display);
	XFillRectangle(display, pixmap, graphicsContextPixmapFill, 0, 0, width, height);
XUnlockDisplay(display);
	onExpose(0, 0, width, height);
}

bool DrawingWindow::isDestroyed() const
{
	return destroyed;
}
