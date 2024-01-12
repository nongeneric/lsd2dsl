#pragma once

#include <fmt/format.h>

#include <string>
#include <functional>

class Log {
    std::string _progressName;
    int _total = 0;
    int _current = 0;
    int _lastPercentage = 0;

protected:
    virtual void reportLog(std::string line, bool verbose) = 0;
    virtual void reportProgress(int percentage) = 0;
    virtual void reportProgressReset(std::string name) = 0;

public:
    virtual ~Log() = default;

    template <class... T>
    void regular(fmt::format_string<T...> format, T&&... args) {
        reportLog(fmt::format(format, std::forward<T>(args)...), false);
    }

    template <class... T>
    void verbose(fmt::format_string<T...> format, T&&... args) {
        reportLog(fmt::format(format, std::forward<T>(args)...), true);
    }

    void resetProgress(std::string name, int total);
    void advance();
};
