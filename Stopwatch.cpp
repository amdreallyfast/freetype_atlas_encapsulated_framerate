#include "Stopwatch.h"

// this is a big header, but necessary to get access to LARGE_INTEGER
// Note: We can't just include winnt.h, in which LARGE_INTEGER is defined,
// because there are some macros that this header file needs that are defined
// further up in the header hierarchy.  So just include Windows.h and be done
// with it.
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//static LARGE_INTEGER g_cpu_timer_frequency;
static double g_inverse_cpu_timer_frequency;
static LARGE_INTEGER g_start_counter;
static LARGE_INTEGER g_last_lap_counter;

static inline double counter_to_seconds(const LARGE_INTEGER counter)
{
    return ((double)counter.QuadPart * g_inverse_cpu_timer_frequency);
}

namespace Timing
{

    bool Stopwatch::initialize()
    {
        // the "performance frequency only changes on system reset, so it's ok
        // to do it only during initialization
        // Note: If it succeeds, it returns non-zero, not a bool as C++ knows it.
        // Rather, it returns a BOOL a typedef of an int.
        // http://msdn.microsoft.com/en-us/library/windows/desktop/ms644905(v=vs.85).aspx
        LARGE_INTEGER cpuFreq;
        bool success = (0 != QueryPerformanceFrequency(&cpuFreq));
        g_inverse_cpu_timer_frequency = 1.0 / cpuFreq.QuadPart;

        return success;
    }

    bool Stopwatch::shutdown()
    {
        // nothing happens in shutdown, but return true to keep up the interface 
        // expectations of boolean return values
        return true;
    }

    void Stopwatch::start()
    {
        // give the counters their first values
        // Note: "On systems that run Windows XP or later, the function will always succeed and will thus never return zero."
        // http://msdn.microsoft.com/en-us/library/windows/desktop/ms644904(v=vs.85).aspx
        QueryPerformanceCounter(&g_start_counter);
        g_last_lap_counter.QuadPart = g_start_counter.QuadPart;
    }

    double Stopwatch::lap()
    {
        // get the current time
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);

        // calculate delta time relative to previous frame
        LARGE_INTEGER delta_large_int;
        delta_large_int.QuadPart = now.QuadPart - g_last_lap_counter.QuadPart;
        double delta_time = counter_to_seconds(delta_large_int);

        g_last_lap_counter.QuadPart = now.QuadPart;

        return delta_time;
    }

    double Stopwatch::total_time()
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);

        double delta_time = counter_to_seconds(now);
        return delta_time;
    }

    void Stopwatch::reset()
    {
        // reset the values by giving them new start values
        this->start();
    }
}
