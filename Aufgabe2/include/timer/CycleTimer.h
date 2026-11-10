/*
 * CycleTimer.h
 *
 *  Created on: 13.09.2009
 *      Author: mn
 *
 * This class implements a timer based on the CPU cycles.
 * The result of the stop-method is in CPU ticks.
 * Note: the resulting values are useful for comparing times,
 *       but not for absolute values.
 */


#ifndef CycleTimer_h
#define CycleTimer_h

#include "timer/cycle.h"

class CycleTimer
{
public:
	void start()
	{
		t0 = getticks();
	}

	ticks stop()
	{
		t1 = getticks();
		return elapsed(t1, t0);
	}

private:
	ticks t0, t1;
};

#endif
