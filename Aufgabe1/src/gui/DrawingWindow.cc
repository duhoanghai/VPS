/*
 * DrawingWindow.cpp
 *
 *  Created on: 13.09.2009
 *      Author: mn
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
	// enable multithreading in Xlib
	XInitThreads();

	// open X display
	display = XOpenDisplay(NULL);

	// check on error
	if(display == 0)
	{
		cerr << "DrawingWindow::DrawingWindow(..), Error: Could not open display." << endl;
		exit(EXIT_FAILURE);
	}

	// create a window with parameters: display, parent, position x, position y, width, height, border-width, border, background
	window = XCreateSimpleWindow(display, RootWindow(display, 0), 1, 1, width, height, 0, BlackPixel(display, 0), BlackPixel(display, 0));

	// check on error
	if(window == 0)
	{
		cerr << "DrawingWindow::DrawingWindow(..), Error: Could not create window." << endl;
		exit(EXIT_FAILURE);
	}

	// set window name
	XTextProperty nameProperty;
	size_t nameSize = name.size();
	char *nameCString = new char[nameSize + 1]; // + 1 for null-termination
	name.copy(nameCString, nameSize, 0);
	nameCString[nameSize] = 0; // null-terminate string

	XStringListToTextProperty((char**) &nameCString, 1, &nameProperty);
	XSetWMName(display, window, &nameProperty);

	delete [] nameCString;
	// Von Xlib reservierte Hilfsdaten nach der Übergabe wieder freigeben.
	XFree(nameProperty.value);

	// set window manager normal hints to make the window non-resizeable (so we can use a fixed size pixel buffer)
	XSizeHints* sizeHints = XAllocSizeHints();

	sizeHints->flags = PMinSize | PMaxSize;
	sizeHints->max_height = height;
	sizeHints->min_height = height;
	sizeHints->max_width = width;
	sizeHints->min_width = width;

	XSetWMNormalHints(display, window, sizeHints);
	XFree(sizeHints);

	// map the window to the display
	XMapWindow(display, window);

	// flush the clients output buffer (X client = this application)
	XFlush(display);

	// set up the graphics context

	// set up some values
	XGCValues values;
	values.foreground = WhitePixel(display, 0);
	values.background = BlackPixel(display, 0);
	values.line_width = 1;
	values.line_style = LineSolid;
	values.cap_style = CapRound;
	values.join_style = JoinRound;
	values.fill_style = FillSolid;
	values.graphics_exposures = False; // True leads to an fast-growing event queue caused by XCopyArea

	// selection of values for the GC creation
	unsigned long valueMask = GCForeground | GCBackground | GCLineWidth	| GCLineStyle | GCCapStyle | GCJoinStyle | GCFillStyle | GCGraphicsExposures;

	// create the context
	graphicsContext = XCreateGC(display, window, valueMask, &values);
	// GC ist ein Zeiger; ein Fehler wird mit NULL statt mit einem negativen Wert gemeldet.
	if(graphicsContext == NULL) {
		cerr << "DrawingWindow::DrawingWindow(..), Error: Could not create graphics context." << endl;
		exit(EXIT_FAILURE);
	}

	// create a pixmap as buffer for drawing
	pixmap = XCreatePixmap(display, window, width, height, DefaultDepth(display, 0));

	// create the context for the pixmap
	graphicsContextPixmap = XCreateGC(display, pixmap, valueMask, &values);
	if(graphicsContextPixmap == NULL) {
		cerr << "DrawingWindow::DrawingWindow(..), Error: Could not create graphics context for the pixmap." << endl;
		exit(EXIT_FAILURE);
	}

	// create another context for filling the pixmap
	graphicsContextPixmapFill = XCreateGC(display, pixmap, valueMask, &values);
	if(graphicsContextPixmapFill == NULL) {
		cerr << "DrawingWindow::DrawingWindow(..), Error: Could not create graphics context for the pixmap for filling." << endl;
		exit(EXIT_FAILURE);
	}

	// set some default attributes
	XSetForeground(display, graphicsContextPixmapFill, BlackPixel(display, 0));

	// clear window/pixmap
	clear();

	// create (and start) pthread running the run-method
	if(createThread)
		Thread::create();
}

DrawingWindow::~DrawingWindow()
{
	Thread::cancel(); // cancel event-loop-thread

	// no further locking required from here
	// Der Ereignisthread ist beendet; Display und Zeichenressourcen sind noch gültig.
	XFreePixmap(display, pixmap);
	XFreeGC(display, graphicsContext);
	XFreeGC(display, graphicsContextPixmap);
	XFreeGC(display, graphicsContextPixmapFill);
	destroyWindow();
}

void DrawingWindow::destroyWindow()
{
	// close display
	destroyed = true;

	XCloseDisplay(display);
	_DEBUG ( cout << "DrawingWindow::destroyWindow(), Info: Window destroyed. " << endl ) ;
}

void DrawingWindow::run()
{
	if(destroyed) return; // check if window is closed

	// event data structure for reading events from the event queue
	XEvent event;

	// define which input sources should be used for events
XLockDisplay(display);
	XSelectInput(display, window, ExposureMask | KeyPressMask | ButtonPressMask);

	// setup WM_DELETE_WINDOW protocol to intercept the destruction of the window
	// by the window manager (e.g. when users decides to close the window)
	Atom wmDeleteWindowAtom = XInternAtom(display, "WM_DELETE_WINDOW", false);
	XSetWMProtocols(display, window, &wmDeleteWindowAtom, 1); // takes array of atoms and count
XUnlockDisplay(display);

	// endless loop which processes events
	while(!destroyed)
	{
		Thread::testCancel(); // creates a cancellation point
		usleep(100);

		// read an event
	XLockDisplay(display);
		bool cont = !XPending(display);
	XUnlockDisplay(display);

		if(cont) // pre-check if there are events to prevent blocking
		{
//			cerr << "c";
			continue;
		}


	XLockDisplay(display);
//		cerr << "p";
		XNextEvent(display, &event); // blocking if no events are pending
	XUnlockDisplay(display);

		//if(!XCheckWindowEvent(display, window, ExposureMask, &event)) // non blocking
		//	continue;

		// evaluate the event type
		switch(event.type) {
		case Expose:
			_DEBUG ( cout << "DrawingWindow::run(), Info: Expose message received. " << endl ) ;
			onExpose(event.xexpose.x, event.xexpose.y, event.xexpose.width, event.xexpose.height);
			break;
		case ClientMessage:
			_DEBUG ( cout << "DrawingWindow::run(), Info: Delete Window request received. " << endl ) ;
			if(destroyable && static_cast<Atom>(event.xclient.data.l[0]) == wmDeleteWindowAtom) // if not destroyable => ignore close button
				destroyed = true;
			break;
		default:
			_DEBUG ( cout << "DrawingWindow::run(), Warning: Unhandeled event.type: " << event.type << endl; )
			break;
	    }
	}

	// this point is reached when the window is destroyed/closed, the window thread ends here
	// Erst der Destruktor schließt das Display, nachdem dieser Thread beendet ist.
	// Im Hauptthread können noch Zeichenaufrufe abgeschlossen werden.
}

void DrawingWindow::onExpose(unsigned int xPos, unsigned int yPos, unsigned int width, unsigned int height) const
{
	if(destroyed) return; // check if window is closed

XLockDisplay(display);
	XCopyArea(display, pixmap, window, graphicsContext, xPos, yPos, width, height, xPos, yPos);
	XFlush(display);
XUnlockDisplay(display);
}

void DrawingWindow::setBackgroundColor(RGBColor color)
{
	if(destroyed) return; // check if window is closed

XLockDisplay(display);
	XSetBackground(display, graphicsContextPixmap, color);
	// clear() verwendet diesen Füllkontext und muss die neue Hintergrundfarbe kennen.
	XSetForeground(display, graphicsContextPixmapFill, color);
XUnlockDisplay(display);
}

void DrawingWindow::setForegroundColor(RGBColor color)
{
	if(destroyed) return; // check if window is closed

XLockDisplay(display);
	XSetForeground(display, graphicsContextPixmap, color);
XUnlockDisplay(display);
}

void DrawingWindow::drawPoint(unsigned int xPos, unsigned int yPos) const
{
	if(destroyed) return; // check if window is closed
XLockDisplay(display);
	XDrawPoint(display, pixmap, graphicsContextPixmap, xPos, yPos);
XUnlockDisplay(display);
	onExpose(xPos, yPos, 1, 1);
}

void DrawingWindow::drawLine(unsigned int xPosStart, unsigned int yPosStart, unsigned int xPosEnd, unsigned int yPosEnd) const
{
	if(destroyed) return; // check if window is closed

XLockDisplay(display);
	XDrawLine(display, pixmap, graphicsContextPixmap, xPosStart, yPosStart, xPosEnd, yPosEnd);
XUnlockDisplay(display);
	onExpose(0, 0, width, height); // NOTE: not the whole window needs to be updated, the bounding box of the line could be used (with min, max functions)
}

void DrawingWindow::drawRectangle(unsigned int xPos, unsigned int yPos, unsigned int width, unsigned int height) const
{
	if(destroyed) return; // check if window is closed

XLockDisplay(display);
	XDrawRectangle(display, pixmap, graphicsContextPixmap, xPos, yPos, width, height);
XUnlockDisplay(display);
	onExpose(xPos, yPos, width + 1, height + 1); // where 1 is line size
}

void DrawingWindow::drawFilledRectangle(unsigned int xPos, unsigned int yPos, unsigned int width, unsigned int height) const
{
	if(destroyed) return; // check if window is closed

XLockDisplay(display);
	XFillRectangle(display, pixmap, graphicsContextPixmap, xPos, yPos, width, height);
XUnlockDisplay(display);
	onExpose(xPos, yPos, width, height);
}

void DrawingWindow::drawText(unsigned int xPos, unsigned int yPos, string text)
{
	if(destroyed) return; // check if window is closed

XLockDisplay(display);
	XDrawString(display, pixmap, graphicsContextPixmap, xPos, yPos, text.c_str(), text.length());
XUnlockDisplay(display);
	onExpose(0, 0, width, height);
}

void DrawingWindow::clear() const
{
	if(destroyed) return; // check if window is closed

XLockDisplay(display);
	XFillRectangle(display, pixmap, graphicsContextPixmapFill, 0, 0, width, height);
XUnlockDisplay(display);
	onExpose(0, 0, width, height);
}

bool DrawingWindow::isDestroyed() const
{
	return destroyed;
}
