/*
 * DrawingWindow.h
 *
 *  Erstellt am: 13.09.2009
 *      Autor: mn
 *
 * Diese Klasse stellt ein einfaches Xlib-Fenster zum Zeichnen geometrischer Elemente bereit.
 * Die Ereignisschleife läuft normalerweise in einem im Konstruktor gestarteten Thread.
 *
 */

#ifndef DrawingWindow_h
#define DrawingWindow_h

#include <X11/Xlib.h>
#include <string>
#include <list>
#include <atomic>
#include "thread/Thread.h"

using namespace std;

namespace GUI
{

	/*
	 * Diese Hilfsklasse beschreibt eine Farbe aus RGB-Komponenten
	 * und stellt die Konvertierung in den von Xlib verwendeten Farbtyp bereit.
	 */

	class RGBColor
	{
	public:
		RGBColor(unsigned char r, unsigned char g, unsigned char b)
			: color( (((unsigned long)r) << 16) | (((unsigned long)g) << 8) | ((unsigned long)b) )
		{}
		// Konvertierungsoperator
		operator unsigned long() const { return color; }

	private:
		unsigned long color;
	};

	// 16 VGA-Farben
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
	 * Fensterklasse mit Zeichenfunktionen und eigener Ereignisschleife.
	 */
	class DrawingWindow : public Thread
	{
	public:
		DrawingWindow(unsigned int width, unsigned int height, string name = "Drawing Window", bool createThread = true, bool destroyable = false);
		virtual ~DrawingWindow();

		virtual void run(); // Ereignisschleife, normalerweise in einem eigenen Thread

		void setBackgroundColor(RGBColor color);
		void setForegroundColor(RGBColor color);
		void drawPoint(unsigned int xPos, unsigned int yPos) const;
		void drawLine(unsigned int xPosStart, unsigned int yPosStart, unsigned int xPosEnd, unsigned int yPosEnd) const;
		void drawRectangle(unsigned int xPos, unsigned int yPos, unsigned int width, unsigned int height) const;
		void drawFilledRectangle(unsigned int xPos, unsigned int yPos, unsigned int width, unsigned int height) const;
		void drawText(unsigned int xPos, unsigned int yPos, string text);
		void clear() const; // Fenster mit der Hintergrundfarbe leeren

		bool isDestroyed() const; // Meldet, ob das Fenster geschlossen wurde

	private:
		void destroyWindow();
		void onExpose(unsigned int xPos, unsigned int yPos, unsigned int width, unsigned int height) const;

		unsigned int width; // Fensterbreite
		unsigned int height; // Fensterhöhe
		string name; // Fenstertitel

		// Xlib-spezifische Daten
		Display* display;
		Window window;
		Pixmap pixmap; // Zeichenpuffer
		// Grafikkontexte mit Farben, Linienstilen und weiteren Zeichenattributen
		GC graphicsContext;
		GC graphicsContextPixmap;
		GC graphicsContextPixmapFill;

		bool destroyable; // Erlaubt das Schließen durch den Benutzer; sonst schließt das Hauptprogramm das Fenster
		std::atomic<bool> destroyed; // Atomar: Ereignisthread schreibt, Hauptthread liest.

	};

} // namespace GUI

#endif
