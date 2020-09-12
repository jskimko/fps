#include <cstdio>
#include <cstdlib>
#include <stdexcept>

extern "C" {
#include <libavformat/avformat.h>
}

#include "fps/fps.hh"
#include "private.hh"

namespace fps { 

Context::
Context(std::string name)
    : fname(std::move(name)),
      ctx(nullptr)
{
    if (avformat_open_input(&ctx, fname.c_str(), nullptr, nullptr) < 0) {
        throw std::runtime_error("avformat_open_input");
    }

    if (avformat_find_stream_info(ctx, nullptr) < 0) {
        throw std::runtime_error("avformat_find_stream_info");
    }
}


Context::
~Context()
{
    avformat_close_input(&ctx);
}

Codec::
Codec(Context &context, MediaType type)
    : codec(nullptr),
      ctx(nullptr)
{
    AVMediaType av_type;
    switch (type) {
        case MediaType::VIDEO:
            av_type = AVMEDIA_TYPE_VIDEO;
            break;
        case MediaType::AUDIO:
            av_type = AVMEDIA_TYPE_AUDIO;
            break;
        default:
            throw std::runtime_error("invalid MediaType");
    }

    idx = av_find_best_stream(context.ctx, av_type, -1, -1, &codec, 0);
    if (idx < 0) { throw std::runtime_error("av_find_best_stream"); }

    ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(ctx, context.ctx->streams[idx]->codecpar);

    if (avcodec_open2(ctx, codec, nullptr) < 0) {
        throw std::runtime_error("avcodec_open2");
    }
}

Codec::
~Codec()
{
    avcodec_free_context(&ctx);
}

Processor::
Processor(Context const &context)
    : ctx(context),
      frame(av_frame_alloc()),
      packet(av_packet_alloc())
{
    if (!frame) {
        throw std::runtime_error("av_frame_alloc");
    }
    if (!packet) {
        throw std::runtime_error("av_packet_alloc");
    }
}

Processor::
~Processor()
{
    av_frame_free(&frame);
    av_packet_free(&packet);
}

} // namespace fps
