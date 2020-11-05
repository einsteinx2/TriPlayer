#include <cstring>
#include "sources/FLACSource.hpp"
#include "Log.hpp"

FLACSource::FLACSource(const std::string &path) : Source() {
    Log::writeError("[FLAC] Opening file: " + path);
    this->decoder = new FLACDecoder();

    FLAC__StreamDecoderInitStatus initStatus = this->decoder->init(path);
    if (initStatus == FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        // Decode the beginning of the file to read the metadata
        if (this->decoder->process_until_end_of_metadata()) {
            // NOTE: For some reason the FLAC::Decoder:File class's get_sample_rate() and 
            //       get_channels() methods are not working correctly, even though the 
            //       metadata callback is called with the correct data. So instead of using those,
            //       I save the values from the metadata callback into our FLACDecoder subclass
            //       and read them from there.
            // NOTE: This works because the metadata callback is called synchronously before 
            //       process_until_end_of_metadata() returns, so we'll always have the data available here.
            this->sampleRate_ = this->decoder->sampleRate_; //(long)this->decoder->get_sample_rate();
            this->channels_ = this->decoder->channels_; //(int)this->decoder->get_channels();
            this->totalSamples_ = this->decoder->totalSamples_; //(int)this->decoder->get_total_samples();
            this->valid_ = this->decoder->valid_;//true;
			FLAC__uint64 posInBytes = 0;
    		this->decoder->get_decode_position(&posInBytes);
            Log::writeError("[TEST] posInBytes: " + std::to_string(posInBytes) + " sampleRate: " + std::to_string(this->sampleRate_) + " channels: " + std::to_string(this->channels_) + " totalSamples: " + std::to_string(this->totalSamples_));
            Log::writeError("[FLAC] File opened successfully");    
        } else {
            this->valid_ = false;
            Log::writeError("[FLAC] Unable to read metadata for file: " + path);
        }
    } else {
        this->valid_ = false;
        Log::writeError("[FLAC] Unable to open file: " + std::string(FLAC__StreamDecoderInitStatusString[initStatus]));
    }
}

FLACSource::~FLACSource() {
    delete this->decoder;
}

size_t FLACSource::decode(unsigned char * buf, size_t buf_size) {
    // TEST: using max block size in bytes
    // NOTE: In stereo 16bit audio, the block size is the number of 32bit integers representing the frame,
    //       so to calculate the size in bytes, we multiply by 4
    // TODO: Handle mono (should be 16bit chunks) and handle 24bit files (no idea how that's stored, I believe it's padded to power of 2, so probably 64bit chunks)
    size_t blockSizeInBytes = this->decoder->maxBlockSize_ * 4;
    Log::writeError("[TEST] FLACSource::decode called buf_size: " + std::to_string(buf_size) + " blockSizeInBytes: " + std::to_string(blockSizeInBytes));
    this->decoder->set_decode_buffer(buf);

    // Decode audio one block (or is it frame) at a time until the buffer can't fit any more
    // NOTE: Actually one frame contains one block, a frame is just a frame header plus a block
    //       See more info about the format here: https://xiph.org/flac/format.html#blocking
    size_t bytesRead = 0;
    while (bytesRead + blockSizeInBytes < buf_size) {
        // Process a single block of audio (or is it a frame of blocks)
        this->decoder->process_single();

        // Get the actual size decoded (may be less than the max_block_size, especially the last block in the song)
        // get_blocksize() returns the size of the last decoded block (or is it a frame of blocks)
        // that size is in 32bit units, so to get the number of bytes we multiply by 4
        bytesRead += this->decoder->get_blocksize() * 4;

        // Check if we've decoded the whole file
        if (this->decoder->get_state() == FLAC__STREAM_DECODER_END_OF_STREAM) {
            this->done_ = true;
            return bytesRead;
        }
    }

    // Return the total number of bytes decoded
    Log::writeError("[TEST] FLACSource::decode bytesRead: " + std::to_string(bytesRead));
    return bytesRead;
}

void FLACSource::seek(size_t pos) {
    Log::writeError("[TEST] FLACSource::seek called, pos: " + std::to_string(pos));
    this->decoder->seek_absolute(pos);
}

size_t FLACSource::tell() {
	size_t currentSample = this->decoder->currentSampleNumber_;
	Log::writeError("[TEST] FLACSource::tell called currentSample: " + std::to_string(currentSample));
	return currentSample;
}

FLAC__StreamDecoderWriteStatus FLACDecoder::write_callback(const ::FLAC__Frame * frame, const FLAC__int32 * const * buffer) {
    Log::writeError("[TEST] FLACSource::write_callback called");
    if (!this->decodeBuffer_) return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;

	// Get the current decoding sample number to use in the tell() method since there's no other way to get it
	this->currentSampleNumber_ = frame->header.number.sample_number;

	// Read the data into the decode buffer
    for (unsigned int i = 0; i < frame->header.blocksize; i++) {
        // Destructure the data into bytes in little-endian format
        // TODO: Support mono and non-16bit files
        this->decodeBuffer_[0] = (FLAC__int16)buffer[0][i];
        this->decodeBuffer_[1] = (FLAC__int16)buffer[0][i] >> 8;
        this->decodeBuffer_[2] = (FLAC__int16)buffer[1][i];
        this->decodeBuffer_[3] = (FLAC__int16)buffer[1][i] >> 8;
        this->decodeBuffer_ += 4;
    }
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FLACDecoder::metadata_callback(const ::FLAC__StreamMetadata *metadata) {
    Log::writeError("[TEST] metadata_callback called");

    if (metadata) {
        // Read the stream metadata here and store in member variables as we need access
        // to it before decoding has begun. The methods such as get_channels() and 
        // get_sample_rate() only work after decoding actual audio data for some reason
        // even though the data is available in the file header metadata...
        FLAC__StreamMetadata_StreamInfo streamInfo = metadata->data.stream_info;
        this->channels_ = (int)streamInfo.channels;
        this->sampleRate_ = (long)streamInfo.sample_rate;
        this->totalSamples_ = (int)streamInfo.total_samples;
        this->bitsPerSample_ = (size_t)streamInfo.bits_per_sample;
        this->maxBlockSize_ = (size_t)streamInfo.max_blocksize;
        this->valid_ = true;

        Log::writeError("[TEST] metadata_callback sample_rate: " + std::to_string(streamInfo.sample_rate) + " channels: " + std::to_string(streamInfo.channels) + " total_samples: " + std::to_string(streamInfo.total_samples) + " bitsPerSample_: " + std::to_string(bitsPerSample_) + " maxBlockSize_: " + std::to_string(maxBlockSize_));
    } else {
        Log::writeError("[FLAC] metadata_callback metadata is null");
    }
}

void FLACDecoder::error_callback(::FLAC__StreamDecoderErrorStatus status) {
    // TODO: Do we need to do something when this occurs?
    Log::writeError(FLAC__StreamDecoderErrorStatusString[status]);
}

void FLACDecoder::set_decode_buffer(unsigned char *decode_buffer) {
    this->decodeBuffer_ = decode_buffer;
}
