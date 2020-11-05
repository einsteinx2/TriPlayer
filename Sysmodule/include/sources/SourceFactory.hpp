#ifndef SOURCEFACTORY_HPP
#define SOURCEFACTORY_HPP

#include <string>
#include "sources/Source.hpp"

class SourceFactory {
public:
    static Source * makeSource(const std::string &path);
};

#endif