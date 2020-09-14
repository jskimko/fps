#ifndef CODEC_HH
#define CODEC_HH

extern "C" {
struct AVCodec;
struct AVCodecContext;
}

#include "fps/enum.hh"
#include "fps/buffer.hh"

namespace fps {

class Context;

class Codec {
protected:
    Codec();
    ~Codec();

    void open();

public:
    AVCodec *codec;
    AVCodecContext *context;

};

class Decoder : public Codec {
public:
    Decoder(Context &ctx, MediaType type);

    bool decode(Packet &packet);
    Unref<Frame> read_into(Frame &frame) const;

    int stream_index;
};

class Encoder : public Codec {
public:
    Encoder(Context &ctx, Decoder &decoder, MediaType type);

    bool encode(Frame &frame);
    Unref<Packet> read_into(Packet &packet) const;
};

} // namespace fps

#endif // CODEC_HH
