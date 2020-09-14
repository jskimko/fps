#ifndef FPS_HH
#define FPS_HH

extern "C" {
#include <libavformat/avformat.h>
}

#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <type_traits>

namespace fps {

// forward declarations.
enum class ContextType;
enum class MediaType;
class Context;
class Reader;
class Writer;
class Decoder;
class Encoder;
class Packet;
class Frame;
template <typename T> class Buffer;
template <typename T> class Unref;

// definitions.

enum class ContextType {
    INPUT,
    OUTPUT
};

enum class MediaType {
    VIDEO = AVMEDIA_TYPE_VIDEO,
    AUDIO = AVMEDIA_TYPE_AUDIO
};

class Context {
public:
    Context(std::string fname, ContextType type);
    ~Context();

    void copy_metadata(Context const &ctx);

    std::string fname;
    AVFormatContext *context;
};

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


template <typename T>
class Buffer {
public:
    using type = T;
    T* get() const { return ptr; }

protected:
    Buffer(T* t) : ptr(t) {}
    T *ptr;
};

class Packet : public Buffer<AVPacket> {
public:
    Packet();
    ~Packet();
};

class Frame : public Buffer<AVFrame> {
public:
    Frame();
    ~Frame();

    bool is_valid() const;
    void copy_from(Frame const &frame);
};

template <typename Buf>
class Unref {
public:
    Unref(Buf *b, std::function<void(typename Buf::type*)> f) 
        : buf(b), unref(std::move(f)) {}
    ~Unref() { if (buf && buf->get()) unref(buf->get()); }

    operator Buf&() const { return *buf; }
    explicit operator bool() const { return buf && buf->get(); }
    typename Buf::type *operator->() { return buf->get(); }

    Buf &get() const { return *buf; }

private:
    Buf *buf;
    std::function<void(typename Buf::type*)> unref;
};

class Interpolator {
public:
    static std::vector<Frame> linear(Frame const &prev, Frame const &cur, int n) {
        std::vector<Frame> ret(n);
        auto pts_step = (prev.get()->pts < cur.get()->pts)
                      ? (cur.get()->pts - prev.get()->pts) / (n + 1)
                      : (prev.get()->pts - cur.get()->pts) / (n + 1);

        for (int i=0; i<n; i++) {
            ret[i].copy_from(prev);
            Frame &frame = ret[i];
            frame.get()->pts += pts_step * (i + 1);

            int len = sizeof(cur.get()->data) / sizeof(cur.get()->data[0]);
            for (int j=0; j<len; j++) {
                auto *plane_prev = prev.get()->data[j];
                auto *plane_cur  = cur.get()->data[j];
                auto *plane_dst  = frame.get()->data[j];
                auto linesize = cur.get()->linesize[j];

                if (!plane_prev || !plane_cur || !plane_dst) { continue; }

                using data_t = std::remove_reference<decltype(plane_dst[0])>::type;
                std::transform(&plane_prev[0], &plane_prev[linesize],
                               &plane_cur[0], &plane_dst[0],
                               [i, n](data_t a, data_t b) -> data_t {
                                   if (a == b) {
                                       return a;
                                   } else if (a < b) {
                                       return a + (b - a) / (n + 1) * (i + 1);
                                   } else {
                                       return b + (a - b) / (n + 1) * (i + 1);
                                   }
                               }
                );
            }
        }
        return ret;
    }
};

}

#endif // FPS_HH
