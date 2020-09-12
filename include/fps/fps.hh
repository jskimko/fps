#ifndef FPS_HH
#define FPS_HH

#include <string>

class AVFormatContext;
class AVCodec;
class AVCodecContext;
class AVFrame;
class AVPacket;

namespace fps {

enum class MediaType {
    VIDEO,
    AUDIO,
    NONE
};

class Context {
public:
    Context(std::string fname);
    ~Context();

    std::string fname;
    AVFormatContext *ctx;
};

class Codec {
public:
    Codec(Context &ctx, MediaType type);
    ~Codec();

    AVCodec *codec;
    AVCodecContext *ctx;
    int idx;
};

class Processor {
public:
    Processor(Context const &ctx);
    ~Processor();

    Context const &ctx;
    AVFrame *frame;
    AVPacket *packet;
};

}

#endif // FPS_HH
