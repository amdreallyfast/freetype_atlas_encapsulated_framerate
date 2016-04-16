#ifndef ENGINE_STOPWATCH
#define ENGINE_STOPWATCH

// copied from my personal engine project, though without the DLL export
namespace Timing
{
    class Stopwatch
    {
    public:
        bool initialize();
        bool shutdown();

        // delta time is in seconds
        void start();
        double lap();
        double total_time();
        void reset();
    };
}

#endif