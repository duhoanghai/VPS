/*
 * Thread.h
 *
 *  Created on: 13.09.2009
 *      Author: mn
 *
 * This class wraps a part of the pthread api to use threads in an object oriented context.
 * Other classes can use this class as a base class and must implement the run method.
 */

#ifndef Thread_h
#define Thread_h

#include <iostream>
#include <pthread.h>
#include <stdexcept>
#include "misc/debug.h"

using namespace std;

class Thread
{
public:
	// Vor create() existiert noch kein Thread, auf den gewartet werden darf.
	Thread() : thread(), running(false) {}
	virtual ~Thread()
	{
		cancel();
	}

	void cancel()
	{
		if(running)
		{
			// running erst in join() zurücksetzen: Sonst würde dessen Prüfung
			// das Warten überspringen und Ressourcen könnten zu früh freigegeben werden.
			int res = pthread_cancel(thread);
			if(res != 0)
				cerr << "Thread::~Thread(), pthread_join failed." << endl;
			join();
		}
	}

	void create()
	{
		_DEBUG ( cout << "Thread::create() called" << endl; )
		int res = pthread_create(&thread, NULL, helper, this);
		if(res != 0)
			throw std::runtime_error("Thread::create(): pthread_create failed");
		// Erst nach erfolgreicher Erstellung ist der Thread joinbar.
		running = true;
	}

	virtual void run() = 0;

	void join()
	{
		if (!running) return;
		_DEBUG ( cout << "Thread::join() called" << endl; )
		int res = pthread_join(thread, NULL);
		running = false;
		if(res != 0)
			cerr << "Thread::join(), pthread_join failed." << endl;
	}

protected:
	void testCancel()
	{
		pthread_testcancel();
	}

private:
	pthread_t thread;
	bool running;

	static void* helper(void* args)
	{
		((Thread*)args)->run();
		return NULL;
	}

};

#endif
