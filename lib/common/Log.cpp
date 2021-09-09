#include "Log.h"

#include <assert.h>

void Log::resetProgress(std::string name, int total) {
    _progressName = name;
    _total = total;
    _current = 0;
    _lastPercentage = 0;
    reportProgressReset(_progressName);
}

void Log::advance() {
    ++_current;
    assert(_current <= _total);
    auto percentage = static_cast<int>(_current * 100. / _total);
    if (percentage > _lastPercentage) {
        _lastPercentage = percentage;
        reportProgress(percentage);
    }
}
