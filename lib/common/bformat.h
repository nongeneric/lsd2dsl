#pragma once

#include <boost/format.hpp>

template <class F, class... T>
std::string bformat(F format, T... args) {
    return str((boost::format(format) % ... % args));
}
