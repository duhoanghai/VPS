/*
 * DrawingWindow.cpp
 *
 *  Created on: 13.09.2009
 *      Author: mn
 */

#include <iostream>
#include <cstdlib>
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
		exit(0);
	}

	// create a window with parameters: display, parent, position x, position y, width, height, border-width, border, background
	window = XCreateSimpleWindow(display, RootWindow(display, 0), 1, 1, width, height, 0, BlackPixel(display, 0), BlackPixel(display, 0));

	// check on error
	if(window == 0)
	{
		cerr << "DrawingWindow::DrawingWindow(..), Error: Could not create window." << endl;
		exit(0);
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

	// set window manager normal hints to make the window non-resizeable (so we can use a fixed size pixel buffer)
	XSizeHints* sizeHints = XAllocSizeHints();

	sizeHints->flags = PMinSize | PMaxSize;
	sizeHints->max_height = height;
	sizeHints->min_height = height;
	sizeHints->max_width = width;
	sizeHints->min_width = width;

	XSetWMNormalHints(display, window, sizeHints);

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
	values.graphics_exposures = True;

	// selection of values for the GC creation
	unsigned long valueMask = GCForeground | GCBackground | GCLineWidth	| GCLineStyle | GCCapStyle | GCJoinStyle | GCFillStyle | GCGraphicsExposures;

	// create the context
	graphicsContext = XCreateGC(display, window, valueMask, &values);
	if(graphicsContext < 0) {
		cerr << "DrawingWindow::DrawingWindow(..), Error: Could not create graphics context." << endl;
		exit(0);
	}

	// create a pixmap as buffer for drawing
	pixmap = XCreatePixmap(display, window, width, height, DefaultDepth(display, 0));

	// create the context for the pixmap
	graphicsContextPixmap = XCreateGC(display, pixmap, valueMask, &values);
	if(graphicsContextPixmap < 0) {
		cerr << "DrawingWindow::DrawingWindow(..), Error: Could not create graphics context for the pixmap." << endl;
		exit(0);
	}

	// create another context for filling the pixmap
	graphicsContextPixmapFill = XCreateGC(display, pixmap, valueMask, &values);
	if(graphicsContextPixmapFill < 0) {
		cerr << "DrawingWindow::DrawingWindow(..), Error: Could not create graphics context for the pixmap for filling." << endl;
		exit(0);
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

	XFreePixmap(display, pixmap);
	if(!destroyed)
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
	XSelectInput(display, window, ExposureMask | KeyPressMask | ButtonPressMask);

	// setup WM_DELETE_WINDOW protocol to intercept the destruction of the window
	// by the window manager (e.g. when users decides to close the window)
	Atom wmDeleteWindowAtom = XInternAtom(display, "WM_DELETE_WINDOW", false);
	XSetWMProtocols(display, window, &wmDeleteWindowAtom, 1); // takes array of atoms and count

	// endless loop which processes events
	while(!destroyed)
	{
		Thread::testCancel(); // creates a cancellation point
		usleep(100);

		// read an event
		if(!XPending(display)) // pre-check if there are events to prevent blocking
			continue;

		XNextEvent(display, &event); // blocking if no events are pending

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
			if(destroyable) // if not destroyable => ignore close button
				destroyed = true;
			break;
		default:
			_DEBUG ( cout << "DrawingWindow::run(), Warning: Unhandeled event.type: " << event.type << endl; )
			break;
	    }
	}

	// this point is reached when the window is destroyed/closed, the window thread ends here
	// program will continue at the blocking call of Thread::join()
	destroyWindow();
}

void DrawingWindow::onExpose(unsigned int xPos, unsigned int yPos, unsigned int width, unsigned int height) const
{
	if(destroyed) return; // check if window is closed

	XCopyArea(display, pixmap, window, graphicsContext, xPos, yPos, width, height, xPos, yPos);
	XFlush(display);
}

void DrawingWindow::setBackgroundColor(RGBColor color)
{
	if(destroyed) return; // check if window is closed

	XSetBackground(display, graphicsContextPixmap, color);
}

void DrawingWindow::setForegroundColor(RGBColor color)
{
	if(destroyed) return; // check if window is closed

	XSetForeground(display, graphicsContextPixmap, color);
}

void DrawingWindow::drawPoint(unsigned int xPos, unsigned int yPos) const
{
	if(destroyed) return; // check if window is closed

	XDrawPoint(display, pixmap, graphicsContextPixmap, xPos, yPos);
	onExpose(xPos, yPos, 1, 1);
}

void DrawingWindow::drawLine(unsigned int xPosStart, unsigned int yPosStart, unsigned int xPosEnd, unsigned int yPosEnd) const
{
	if(destroyed) return; // check if window is closed

	XDrawLine(display, pixmap, graphicsContextPixmap, xPosStart, yPosStart, xPosEnd, yPosEnd);
	onExpose(0, 0, width, height); // NOTE: not the whole window needs to be updated, the bounding box of the line could be used (with min, max functions)
}

void DrawingWindow::drawRectangle(unsigned int xPos, unsigned int yPos, unsigned int width, unsigned int height) const
{
	if(destroyed) return; // check if window is closed

	XDrawRectangle(display, pixmap, graphicsContextPixmap, xPos, yPos, width, height);
	onExpose(xPos, yPos, width + 1, height + 1); // where 1 is line size
}

void DrawingWindow::drawFilledRectangle(unsigned int xPos, unsigned int yPos, unsigned int width, unsigned int height) const
{
	if(destroyed) return; // check if window is closed

	XFillRectangle(display, pixmap, graphicsContextPixmap, xPos, yPos, width, height);
	onExpose(xPos, yPos, width + 1, height + 1); //  where 1 is line size
}

void DrawingWindow::drawText(unsigned int xPos, unsigned int yPos, string text)
{
	if(destroyed) return; // check if window is closed

	XDrawString(display, pixmap, graphicsContextPixmap, xPos, yPos, text.c_str(), text.length());
	onExpose(0, 0, width, height);
}

void DrawingWindow::clear() const
{
	if(destroyed) return; // check if window is closed

	XFillRectangle(display, pixmap, graphicsContextPixmapFill, 0, 0, width, height);
	onExpose(0, 0, width, height);
}

bool DrawingWindow::isDestroyed() const
{
	return destroyed;
}
