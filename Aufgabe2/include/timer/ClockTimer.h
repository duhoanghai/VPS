/*
 * ClockTimer.h
 *
 *  Created on: 13.09.2009
 *      Author: mn
 *
 * This class implements a Timer based on the system clock.
 * The result of the stop-method is in microseconds.
 * Note: the start and stop methods use a system-call.
 */

#ifndef ClockTimer_h
#define ClockTimer_h

#include <sys/time.h>
#include <ctime>
#include <iostream>

using namespace std;

class ClockTimer
{
public:
	void start()
	{
		gettimeofday(&t0, NULL);
	}

	long stop()
	{
		int result;
		result = gettimeofday(&t1, NULL);
		cout << result << endl;
		return (t1.tv_sec - t0.tv_sec) * 1000000 + (t1.tv_usec - t0.tv_usec);
	}

private:
	timeval t0, t1;
};

#endif
