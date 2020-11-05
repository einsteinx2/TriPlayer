#include <filesystem>
#include "sources/MP3Source.hpp"
#include "sources/FLACSource.hpp"
#include "sources/SourceFactory.hpp"
#include "Log.hpp"

Source * SourceFactory::makeSource(const std::string &path) {
    std::string extension = std::filesystem::path(path).extension();
    if (extension == ".mp3" || extension == ".MP3") {
        Log::writeError("SourceFactory makeSource returning MP3Source");
        return new MP3Source(path);
    } else if (extension == ".flac" || extension == ".FLAC") {
        Log::writeError("SourceFactory makeSource returning FLACSource");
        return new FLACSource(path);
    }
    return nullptr;
}