#ifndef Timer_H
#define Timer_H

#include <ctime>
#include <iostream>
#include <iomanip>
#include <string>

class Timer
{
private:
	bool running;
	clock_t start_clock;
	time_t start_time;
	double acc_time;

public:
	// 'running' is initially false.  A Timer needs to be explicitly started
	// using 'start' or 'restart'
	Timer() :
		running(false), start_clock(0), start_time(0), acc_time(0)
	{
	}

        double elapsed_time();
	void start();
	void restart();
	void stop();

}; // class Timer

//===========================================================================
// Return the total time that the Timer has been in the "running"
// state since it was first "started" or last "restarted".  For
// "short" time periods (less than an hour), the actual cpu time
// used is reported instead of the elapsed time.

inline double Timer::elapsed_time()
{
	time_t acc_sec = time(0) - start_time;
	if (acc_sec < 3600)
		return (clock() - start_clock) / (1.0 * CLOCKS_PER_SEC);
	else
		return (1.0 * acc_sec);

} // Timer::elapsed_time

//===========================================================================
// Start a Timer.  If it is already running, let it continue running.
// Print an optional message.

inline void Timer::start()
{
	// Return immediately if the Timer is already running
	if (running)
		return;

	// Set Timer status to running and set the start time
	running = true;
	start_clock = clock();
	start_time = time(0);

} // Timer::start

//===========================================================================
// Turn the Timer off and start it again from 0.  Print an optional message.

inline void Timer::restart()
{
	// Set Timer status to running, reset accumulated time, and set start time
	running = true;
	acc_time = 0;
	start_clock = clock();
	start_time = time(0);

} // Timer::restart

//===========================================================================
// Stop the Timer and print an optional message.

inline void Timer::stop()
{
	if (running)
		acc_time += elapsed_time();
	running = false;

} // Timer::stop

#endif // Timer_H
