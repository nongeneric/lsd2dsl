#include <chrono>

class Stopwatch
{
    std::chrono::time_point<std::chrono::high_resolution_clock> past =
        std::chrono::high_resolution_clock::now();

public:
    unsigned elapsedMs() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - past).count();
    }
};
