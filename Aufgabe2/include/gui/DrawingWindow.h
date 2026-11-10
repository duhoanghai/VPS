/*
 * DrawingWindow.h
 *
 *  Created on: 13.09.2009
 *      Author: mn
 *
 * This class implements a simple Xlib based window for drawing primitives on the screen.
 * The event loop runs in an own Thread which (normally) is started in the constructor.
 *
 */

#ifndef DrawingWindow_h
#define DrawingWindow_h

#include <X11/Xlib.h>
#include <string>
#include <list>
#include "thread/Thread.h"

using namespace std;

namespace GUI
{

	/*
	 * This helper class realises a color Type, which can be set with RBG values on the one hand,
	 * and which is also compatible with the color type used by Xlib on the other hand.
	 */

	class RGBColor
	{
	public:
		RGBColor(unsigned char r, unsigned char g, unsigned char b)
			: color( (((unsigned long)r) << 16) | (((unsigned long)g) << 8) | ((unsigned long)b) )
		{}
		// conversion operator
		operator unsigned long() const { return color; }

	private:
		unsigned long color;
	};

	// 16 VGA colors
	const RGBColor BLACK(  0x00, 0x00, 0x00);
	const RGBColor MAROON( 0x80, 0x00, 0x00);
	const RGBColor GREEN(  0x00, 0x80, 0x00);
	const RGBColor OLIVE(  0x80, 0x80, 0x00);
	const RGBColor NAVY(   0x00, 0x00, 0x80);
	const RGBColor PURPLE( 0x80, 0x00, 0x80);
	const RGBColor TEAL(   0x00, 0x80, 0x80);
	const RGBColor SILVER( 0xC0, 0xC0, 0xC0);

	const RGBColor GREY(   0x80, 0x80, 0x80);
	const RGBColor RED(    0xFF, 0x00 ,0x00);
	const RGBColor LIME(   0x00, 0xFF ,0x00);
	const RGBColor YELLOW( 0xFF, 0xFF ,0x00);
	const RGBColor BLUE(   0x00, 0x00 ,0xFF);
	const RGBColor FUCHSIA(0xFF, 0x00 ,0xFF); // mangenta
	const RGBColor AQUA(   0x00, 0xFF ,0xFF); // cyan
	const RGBColor WHITE(  0xFF, 0xFF ,0xFF);


	/*
	 * This is the window-class mentioned in the comment at the beginning of this file.
	 */
	class DrawingWindow : public Thread
	{
	public:
		DrawingWindow(unsigned int width, unsigned int height, string name = "Drawing Window", bool createThread = true, bool destroyable = false);
		virtual ~DrawingWindow();

		virtual void run(); // the event loop (normally run in a separate thread)

		void setBackgroundColor(RGBColor color);
		void setForegroundColor(RGBColor color);
		void drawPoint(unsigned int xPos, unsigned int yPos) const;
		void drawLine(unsigned int xPosStart, unsigned int yPosStart, unsigned int xPosEnd, unsigned int yPosEnd) const;
		void drawRectangle(unsigned int xPos, unsigned int yPos, unsigned int width, unsigned int height) const;
		void drawFilledRectangle(unsigned int xPos, unsigned int yPos, unsigned int width, unsigned int height) const;
		void drawText(unsigned int xPos, unsigned int yPos, string text);
		void clear() const; // clear the window with the background color

		bool isDestroyed() const; // returns true if the user closed the window

	private:
		void destroyWindow();
		void onExpose(unsigned int xPos, unsigned int yPos, unsigned int width, unsigned int height) const;

		unsigned int width; // window width
		unsigned int height; // window height
		string name; // title of the window

		// Xlib specific members
		Display* display;
		Window window;
		Pixmap pixmap; // buffer for drawing
		// contexts for drawing which describe, colors, line style etc.
		GC graphicsContext;
		GC graphicsContextPixmap;
		GC graphicsContextPixmapFill;

		bool destroyable; // flag which controls whether the window is closeable by the user, or is terminated with the main program
		bool destroyed; // true if the window is closed

	};

} // namespace GUI

#endif
