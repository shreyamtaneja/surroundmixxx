#pragma once

#include <cstdint>

class EncoderCallback {
  public:
    // writes to encoded audio to a stream, e.g., a file stream or broadcast stream
    virtual void write(const unsigned char *header, const unsigned char *body,
                       int headerLen, int bodyLen) = 0;
    // gets stream position
    virtual std::int64_t tell() = 0;
    // sets stream position
    virtual void seek(std::int64_t pos) = 0;
    // gets stream length
    virtual std::int64_t filelen() = 0;
};
