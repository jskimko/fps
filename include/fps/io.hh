#ifndef IO_HH
#define IO_HH

#include "fps/buffer.hh"
#include "fps/enum.hh"

namespace fps {

class Context;
class Encoder;

class Reader {
public:
    Reader(Context const &context);

    Unref<Packet> read_into(Packet &packet) const;

private:
    Context const &context;
};

class Writer {
public:
    Writer(Context const &ctx, Encoder const &video_encoder, Encoder const &audio_encoder);
    ~Writer();

    bool write(Packet &packet, MediaType type) const;

private:
    void add_stream(AVStream **stream, Encoder const &encoder);

    Context const &context;
    Encoder const &video_encoder;
    Encoder const &audio_encoder;
    AVStream *video_stream;
    AVStream *audio_stream;
};

} // namespace fps

#endif // IO_HH
