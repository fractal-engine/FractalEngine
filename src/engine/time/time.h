//
//
//
// -----------------------------------------------------------
// This is a modified version of nuro's time
// https://github.com/jonkwl/nuro/blob/main/nuro-core/time
// -----------------------------------------------------------
//
//
//

#ifndef TIME_H
#define TIME_H

namespace Time
{
	// Calculate times for current frame (handled by application context)
	void Step(double delta_time);

	// Returns the current application time
	double Now();

	// Returns the current application time
	float Nowf();

	// Returns the application time of last frame
	double Last();

	// Returns the application time of last frame
	float Lastf();

	// Returns the current delta time not affected by the time scale
	double UnscaledDelta();

	// Returns the current delta time not affected by the time scale
	double UnscaledDeltaf();

	// Returns the current delta time
	double Delta();

	// Returns the current delta time
	float Deltaf();

	// Returns the current time scale
	double GetTimeScale();

	// Sets the time scale
	void SetTimeScale(double time_scale);
};

#endif // TIME_H
