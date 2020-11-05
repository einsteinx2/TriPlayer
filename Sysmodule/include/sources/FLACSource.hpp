#ifndef SOURCES_FLACSOURCE_HPP
#define SOURCES_FLACSOURCE_HPP

#include <string>
#include <FLAC++/decoder.h>
#include "sources/Source.hpp"

// Forward declaration as we only need the pointers here
namespace NX {
    class File;
};

class FLACDecoder;

class FLACSource : public Source {
private:
    FLACDecoder * decoder;
    NX::File * file;
public:
    FLACSource(const std::string &);
    ~FLACSource();

    size_t decode(unsigned char *, size_t);
    void seek(size_t);
    size_t tell();
};

class FLACDecoder : public FLAC::Decoder::File {
    unsigned char * decodeBuffer_ = nullptr;

    int channels_ = 0;
    long sampleRate_ = 0;
    int totalSamples_ = 0;
    size_t bitsPerSample_ = 0;
    size_t maxBlockSize_ = 0;
    bool valid_ = false;
protected:
    FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 *const *buffer) override;
    void error_callback(::FLAC__StreamDecoderErrorStatus status) override;
    void metadata_callback(const ::FLAC__StreamMetadata *metadata) override;
public:
    void set_decode_buffer(unsigned char *decode_buffer);

    friend FLACSource;
};

#endif
