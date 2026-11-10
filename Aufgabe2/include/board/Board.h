#ifndef BOARD_H
#define BOARD_H

#include <sys/types.h>

enum life_status_t {invalid,dead,alive};

/**
 * A Board is the matrix for the life-simulation.
 *
 * Subclasses must implement the Board(),readPos() and setPos() functions
 * in order to be instanciated.
 */

class Board {
public:

	/**
	* Create a board with a given size, where the upper left corner is 0,0
	*
	* @param width is the horizontal size of the board
	* @param height is the vertical size of the board
	*/
	Board(unsigned width, unsigned height)
			: width(width), height(height)
	{}

	/**
	 * Because this is a pure virtual class, we should declare 
	 * the destructor also as virtual.
	 */
	virtual ~Board() {}

	/**
	* Get the liveliness of a specified element.
	*
	* @param x is the horizontal position of the element
	* @param y is the vertical position of the element
	* @return dead or alive if element exists, else invalid
	*/
	virtual enum life_status_t readPos(size_t x,size_t y)=0;

	/**
	* Sets a element to a life status. If position or status is invalid,
	* nothing is done.
	*
	* @param x is the horizontal position of the element
	* @param y is the vertical position of the element
	* @param status is the life status to be set
	*/
	virtual void setPos(size_t x, size_t y,enum life_status_t status)=0;

	/**
	* Return width and height 
	*/
	void getDimension(size_t &dimX, size_t &dimY)
	{
		dimX = width;
		dimY = height;
	}

protected:
	size_t width;
	size_t height;

};

#endif
