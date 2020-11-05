#include <filesystem>
#include "Log.hpp"
#include "utils/MP3.hpp"
#include "utils/TagLibTags.hpp"

// TagLib
#include <mpegfile.h>

namespace Utils::MP3 {
    // Searches and returns an appropriate image
    std::vector<unsigned char> getArt(std::string path) {
        std::vector<unsigned char> v;

        // Open the file to read the metadata
        TagLib::MPEG::File audioFile(path.c_str(), false);
        if (audioFile.isValid()) {
            // Read the ID3v2 tag
            // NOTE: ID3v1 does not support album art
            if (audioFile.hasID3v2Tag()) {
                TagLib::ID3v2::Tag * tag = audioFile.ID3v2Tag();
                if (tag) {
                    TagLibTags::readArtFromID3v2Tag(tag, &v, path);
                } else {
                    Log::writeError("[MP3] Failed to parse metadata for: " + path);
                }
            } else {
                Log::writeWarning("[MP3] No ID3v2 tags were found in: " + path);
            }
        } else {
            Log::writeError("[MP3] Unable to open file: " + path);
        }

        return v;
    }

    // Checks for tag type and reads the metadata
    Metadata::Song getInfo(std::string path) {
        // Default info to return
        Metadata::Song m;
        m.ID = -3;
        m.title = std::filesystem::path(path).stem();      // Title defaults to file name
        m.artist = "Unknown Artist";                       // Artist defaults to unknown
        m.album = "Unknown Album";                         // Same for album
        m.trackNumber = 0;                                 // Initially 0 to indicate not set
        m.discNumber = 0;                                  // Initially 0 to indicate not set
        m.duration = 0;                                    // Initially 0 to indicate not set

        // Open the file to read the tags
        TagLib::MPEG::File audioFile(path.c_str(), true, TagLib::AudioProperties::Average);
        if (audioFile.isValid()) {
            // Read the audio properties to get the duration
            TagLib::MPEG::Properties * audioProperties = audioFile.audioProperties();
            if (audioProperties) {
                m.duration = (unsigned int)audioProperties->lengthInSeconds();
            } else {
                Log::writeError("[MP3] Failed to read audio properties of file: " + path);
            }

            // Confirm the tags exist and read them
            if (audioFile.hasID3v2Tag()) {
                TagLib::ID3v2::Tag * tag = audioFile.ID3v2Tag();
                if (tag) {
                    TagLibTags::readInfoFromID3v2Tag(tag, &m, path);
                } else {
                    m.ID = -2;
                    Log::writeWarning("[MP3] No tags were found in: " + path);
                }
            } else if (audioFile.hasID3v1Tag()) {
                // NOTE: ID3v1 does not have a disc number tag
                TagLib::ID3v1::Tag * tag = audioFile.ID3v1Tag();
                if (tag) {
                    TagLibTags::readInfoFromID3v1Tag(tag, &m, path);
                } else {
                    m.ID = -2;
                    Log::writeWarning("[MP3] No tags were found in: " + path);
                }
            } else {
                m.ID = -2;
                Log::writeWarning("[MP3] No ID3 metadata present in: " + path);
            }
        } else {
            Log::writeError("[MP3] Unable to open file: " + path);
        }

        return m;
    }
};