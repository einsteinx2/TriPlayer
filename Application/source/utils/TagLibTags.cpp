#include <filesystem>
#include "Log.hpp"
#include "utils/TagLibTags.hpp"
#include "utils/MP3.hpp"
#include "utils/FLAC.hpp"

// TagLib
#include <id3v2tag.h>
#include <id3v1tag.h>
#include <attachedpictureframe.h>
#include <xiphcomment.h>

namespace Utils::TagLibTags {
    // Searches and returns an appropriate image
    std::vector<unsigned char> getArt(std::string path) {
        std::string extension = std::filesystem::path(path).extension();
        if (extension == ".mp3" || extension == ".MP3") {
            return MP3::getArt(path);
        } else if (extension == ".flac" || extension == ".FLAC") {
            return FLAC::getArt(path);
        } else {
            Log::writeError("[TagLibTag] getArt unsupported file type: " + path);
            std::vector<unsigned char> v;
            return v;
        }
    }

    // Checks for tag type and reads the metadata
    Metadata::Song getInfo(std::string path) {
        std::string extension = std::filesystem::path(path).extension();
        if (extension == ".mp3" || extension == ".MP3") {
            return MP3::getInfo(path);
        } else if (extension == ".flac" || extension == ".FLAC") {
            return FLAC::getInfo(path);
        } else {
            // Default info to return
            Metadata::Song m;
            m.ID = -3;
            m.title = std::filesystem::path(path).stem();      // Title defaults to file name
            m.artist = "Unknown Artist";                       // Artist defaults to unknown
            m.album = "Unknown Album";                         // Same for album
            m.trackNumber = 0;                                 // Initially 0 to indicate not set
            m.discNumber = 0;                                  // Initially 0 to indicate not set
            m.duration = 0;                                    // Initially 0 to indicate not set
            return m;
        }
    }

    // Searches and returns an appropriate image
    void readArtFromID3v2Tag(TagLib::ID3v2::Tag * tag, std::vector<unsigned char> * outData, const std::string &path) {
        if (!outData) {
            Log::writeError("[TagLibTag] readArtFromID3v2Tag outData is null: " + path);
            return;
        }

        if (tag) {
            // Extract the cover art tag frame
            TagLib::ID3v2::FrameList coverFrameList = tag->frameListMap()["APIC"];
            if (!coverFrameList.isEmpty()) {
                TagLib::ID3v2::AttachedPictureFrame * coverFrame = (TagLib::ID3v2::AttachedPictureFrame *)coverFrameList.front();
                if (coverFrame) {
                    // Check the image format and extract it if supported
                    TagLib::String mType = coverFrame->mimeType();
                    if (mType == "image/jpg" || mType == "image/jpeg" || mType == "image/png") {
                        TagLib::ByteVector byteVector = coverFrame->picture();
                        outData->assign(byteVector.begin(), byteVector.end());
                    } else {
                        Log::writeInfo("[TagLibTag] No suitable art found in: " + path);
                    }
                } else {
                    Log::writeInfo("[TagLibTag] No suitable art found in: " + path);
                }
            } else {
                Log::writeInfo("[TagLibTag] No suitable art found in: " + path);
            }
        } else {
            Log::writeError("[TagLibTag] Failed to parse metadata for: " + path);
        }
    }

    // Searches and returns an appropriate image
    void readArtFromVorbisTag(TagLib::Ogg::XiphComment * tag, std::vector<unsigned char> * outData, const std::string &path) {
        if (!outData) {
            Log::writeError("[TagLibTag] readArtFromVorbisTag outData is null: " + path);
            return;
        }

        if (tag) {
            TagLib::FLAC::Picture * picture = tag->pictureList().front();
            if (picture) {
                TagLib::ByteVector byteVector = picture->data();
                outData->assign(byteVector.begin(), byteVector.end());
            } else {
                Log::writeInfo("[TagLibTag] No suitable art found in: " + path);
            }
        } else {
            Log::writeError("[TagLibTag] Failed to parse metadata for: " + path);
        }
    }

    // Reads the ID3v2 metadata
    void readInfoFromID3v2Tag(TagLib::ID3v2::Tag * tag, Metadata::Song * outMetadata, const std::string &path) {
        if (!outMetadata) {
            Log::writeError("[TagLibTag] readInfoFromID3v2Tag outMetadata is null: " + path);
            return;
        }

        if (tag) {
            outMetadata->ID = -1;
            TagLib::String title = tag->title();
            if (!title.isEmpty()) outMetadata->title = title.to8Bit(true);
            TagLib::String artist = tag->artist();
            if (!artist.isEmpty()) outMetadata->artist = artist.to8Bit(true);
            TagLib::String album = tag->album();
            if (!album.isEmpty()) outMetadata->album = album.to8Bit(true);
            outMetadata->trackNumber = tag->track();
            TagLib::ID3v2::FrameListMap frameListMap = tag->frameListMap();
            TagLib::ID3v2::FrameList discNumberList = frameListMap["TPOS"];
            if (!discNumberList.isEmpty()) {
                TagLib::String discNumber = discNumberList.front()->toString();
                if (!discNumber.isEmpty()) {
                    // NOTE: This correctly handles cases where the format is "1/2" rather than "1". 
                    //       In both cases, it will return 1.
                    outMetadata->discNumber = discNumber.toInt();
                }
            }
        } else {
            outMetadata->ID = -2;
            Log::writeWarning("[TagLibTag] No tags were found in: " + path);
        }
    }

    // Reads the ID3v1 metadata
    void readInfoFromID3v1Tag(TagLib::ID3v1::Tag * tag, Metadata::Song * outMetadata, const std::string &path) {
        if (!outMetadata) {
            Log::writeError("[TagLibTag] readInfoFromID3v2Tag outMetadata is null: " + path);
            return;
        }

        if (tag) {
            outMetadata->ID = -1;
            TagLib::String title = tag->title();
            if (!title.isEmpty()) outMetadata->title = title.to8Bit(true);
            TagLib::String artist = tag->artist();
            if (!artist.isEmpty()) outMetadata->artist = artist.to8Bit(true);
            TagLib::String album = tag->album();
            if (!album.isEmpty()) outMetadata->album = album.to8Bit(true);
            outMetadata->trackNumber = tag->track();
        } else {
            outMetadata->ID = -2;
            Log::writeWarning("[TagLibTag] No tags were found in: " + path);
        }
    }

    // Reads the Vorbis metadata
    void readInfoFromVorbisTag(TagLib::Ogg::XiphComment * tag, Metadata::Song * outMetadata, const std::string &path) {
        if (!outMetadata) {
            Log::writeError("[TagLibTag] readInfoFromVorbisTag outMetadata is null: " + path);
            return;
        }

        if (tag) {
            outMetadata->ID = -1;
            TagLib::String title = tag->title();
            if (!title.isEmpty()) outMetadata->title = title.to8Bit(true);
            TagLib::String artist = tag->artist();
            if (!artist.isEmpty()) outMetadata->artist = artist.to8Bit(true);
            TagLib::String album = tag->album();
            if (!album.isEmpty()) outMetadata->album = album.to8Bit(true);
            outMetadata->trackNumber = tag->track();
            TagLib::Ogg::FieldListMap fieldListMap = tag->fieldListMap();
            TagLib::StringList discNumberList = fieldListMap["DISCNUMBER"];
            if (!discNumberList.isEmpty()) {
                TagLib::String discNumber = discNumberList.front();
                outMetadata->discNumber = discNumber.toInt();
            }
        } else {
            outMetadata->ID = -2;
            Log::writeWarning("[TagLibTag] No tags were found in: " + path);
        }
    }
};