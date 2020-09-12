#ifndef FPS_HH
#define FPS_HH

extern "C" {
#include <libavformat/avformat.h>
}

#include <string>
#include <functional>

namespace fps {

// forward declarations.
enum class MediaType;
class Context;
class Reader;
//class Writer;
class Decoder;
class Encoder;
class Packet;
class Frame;
template <typename T> class Buffer;
template <typename T> class Unref;

// definitions.

enum class MediaType {
    VIDEO = AVMEDIA_TYPE_VIDEO,
    AUDIO = AVMEDIA_TYPE_AUDIO
};

struct Context {
    Context(std::string fname);
    ~Context();

    std::string fname;
    AVFormatContext *context;
};

struct Reader {
    Reader(Context const &context);

    Unref<Packet> read_into(Packet &packet) const;

    Context const &context;
};

//class Writer {
//public:
//    Writer(Context const &context);
//
//    Context const &context;
//};

class Codec {
protected:
    Codec();
    ~Codec();

public:
    AVCodec *codec;
    AVCodecContext *context;
};

struct Decoder : public Codec {
    Decoder(Context &ctx, MediaType type);

    bool decode(Packet &packet);
    Unref<Frame> read_into(Frame &frame) const;

    int stream_idx;
};

struct Encoder : public Codec {
    Encoder(Context &ctx, Decoder &decoder, MediaType type);

    bool encode(Frame &frame);
    Unref<Packet> read_into(Packet &packet) const;

    AVStream *stream;
};


template <typename T>
class Buffer {
public:
    using type = T;
    T* get() const { return ptr; }

protected:
    Buffer(T* t) : ptr(t) {}
    T *ptr;
};

struct Packet : public Buffer<AVPacket> {
    Packet();
    ~Packet();
};

struct Frame : public Buffer<AVFrame> {
    Frame();
    ~Frame();
};

template <typename Buf>
struct Unref {
    Unref(Buf *b, std::function<void(typename Buf::type*)> f) 
        : buf(b), unref(std::move(f)) {}
    ~Unref() { if (buf && buf->get()) unref(buf->get()); }

    operator Buf&() const { return *buf; }
    explicit operator bool() const { return buf && buf->get(); }
    typename Buf::type *operator->() { return buf->get(); }

    Buf &get() const { return *buf; }

    Buf *buf;
    std::function<void(typename Buf::type*)> unref;
};

}

#endif // FPS_HH
