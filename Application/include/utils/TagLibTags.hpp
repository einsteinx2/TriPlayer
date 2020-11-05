#ifndef UTILS_TAGLIBTAGS_HPP
#define UTILS_TAGLIBTAGS_HPP

#include "Types.hpp"
#include <vector>

// Forward declare TagLib classes so we don't need to import the headers
namespace TagLib {
    namespace ID3v2 { 
        class Tag;
    }
    namespace ID3v1 { 
        class Tag;
    }
    namespace Ogg {
        class XiphComment;
    }
}

namespace Utils::TagLibTags {
    // Reads image(s) from file tags using TagLib and returns a vector containing the image data 
    // The vector will be size 0 if no art is found
    // Pass path of file
    std::vector<unsigned char> getArt(std::string);

    // Reads metadata from tags of file using TagLib and returns SongInfo
    // ID is -1 on success (filled), -2 on success (song has no tags), -3 on failure
    // Pass path of file
    Metadata::Song getInfo(std::string);

    // Reads image(s) from ID3 tags using TagLib and returns a vector containing the image data 
    // The vector will be size 0 if no art is found
    // Pass path of file
    void readArtFromID3v2Tag(TagLib::ID3v2::Tag * tag, std::vector<unsigned char> * outData, const std::string &path);

    void readArtFromVorbisTag(TagLib::Ogg::XiphComment * tag, std::vector<unsigned char> * outData, const std::string &path);

    // Reads ID3 tags of file using TagLib and returns SongInfo
    // ID is -1 on success (filled), -2 on success (song has no tags), -3 on failure
    // Pass path of file
    void readInfoFromID3v2Tag(TagLib::ID3v2::Tag * tag, Metadata::Song * outMetadata, const std::string &path);

    void readInfoFromID3v1Tag(TagLib::ID3v1::Tag * tag, Metadata::Song * outMetadata, const std::string &path);

    void readInfoFromVorbisTag(TagLib::Ogg::XiphComment * tag, Metadata::Song * outMetadata, const std::string &path);
};

#endif