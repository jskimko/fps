#include <cstdio>
#include <cstdlib>
#include <stdexcept>

extern "C" {
#include <libavformat/avformat.h>
}

#include "fps/fps.hh"
#include "private.hh"

namespace fps { 

//---------//
// Context //
//---------//
Context::
Context(std::string name)
    : fname(std::move(name)),
      context(nullptr)
{
    if (avformat_open_input(&context, fname.c_str(), nullptr, nullptr) < 0) {
        throw std::runtime_error("avformat_open_input");
    }

    if (avformat_find_stream_info(context, nullptr) < 0) {
        throw std::runtime_error("avformat_find_stream_info");
    }
}


Context::
~Context()
{
    avformat_close_input(&context);
}

//--------//
// Reader //
//--------//
Reader::
Reader(Context const &ctx)
    : context(ctx)
{
}

Unref<Packet>
Reader::
read_into(Packet &packet) const
{
    if (av_read_frame(context.context, packet.get()) < 0) {
        return {nullptr, nullptr};
    }
    return {&packet, &av_packet_unref};
}

//-------//
// Codec //
//-------//
Codec::
Codec()
    : codec(nullptr),
      context(nullptr)
{
}

Codec::
~Codec()
{
    if (context) { avcodec_free_context(&context); }
}

//---------//
// Decoder //
//---------//
Decoder::
Decoder(Context &ctx, MediaType type)
    : Codec(),
      stream_idx(-1)
{
    stream_idx = av_find_best_stream(ctx.context, (AVMediaType) type, -1, -1, &codec, 0);
    if (stream_idx < 0) {
        throw std::runtime_error("av_find_best_stream");
    }

    context = avcodec_alloc_context3(codec);
    if (!context) {
        throw std::runtime_error("avcodec_alloc_context3");
    }

    if (avcodec_parameters_to_context(context, ctx.context->streams[stream_idx]->codecpar) < 0) {
        throw std::runtime_error("avcodec_parameters_to_context");
    }

    if (avcodec_open2(context, codec, nullptr) < 0) {
        throw std::runtime_error("avcodec_open2");
    }
}

bool
Decoder::
decode(Packet &packet)
{
    if (packet.get()->stream_index != stream_idx) { return false; }
    return avcodec_send_packet(context, packet.get()) == 0;
}

Unref<Frame>
Decoder::
read_into(Frame &frame) const
{
    if (avcodec_receive_frame(context, frame.get()) < 0) {
        return {nullptr, nullptr};
    }
    return {&frame, &av_frame_unref};
}

//---------//
// Encoder //
//---------//
Encoder::
Encoder(Context &ctx, Decoder &decoder, MediaType type)
    : Codec(),
      stream(nullptr)
{
    codec = avcodec_find_encoder(decoder.context->codec_id);
    if (!codec) {
        throw std::runtime_error("avcodec_find_encoder");
    }

    context = avcodec_alloc_context3(codec);
    if (!context) {
        throw std::runtime_error("avcodec_alloc_context3");
    }

    switch (type) {
        case MediaType::VIDEO:
            context->width = decoder.context->width;
            context->height = decoder.context->height;
            context->time_base = ctx.context->streams[decoder.stream_idx]->time_base;
            context->pix_fmt = decoder.context->pix_fmt;
            break;
        case MediaType::AUDIO:
            context->time_base = ctx.context->streams[decoder.stream_idx]->time_base;
            context->sample_fmt = decoder.context->sample_fmt;
            context->sample_rate = decoder.context->sample_rate;
            context->channel_layout = decoder.context->channel_layout;
            break;
        default:
            throw std::runtime_error("unsupported MediaType");
    }

    if (avcodec_open2(context, codec, nullptr) < 0) {
        throw std::runtime_error("avcodec_open2");
    }
}

bool
Encoder::
encode(Frame &frame)
{
    return avcodec_send_frame(context, frame.get()) == 0;
}

Unref<Packet>
Encoder::
read_into(Packet &packet) const
{
    if (avcodec_receive_packet(context, packet.get()) < 0) {
        return {nullptr, nullptr};
    }
    return {&packet, &av_packet_unref};
}

//--------//
// Packet //
//--------//
Packet::
Packet()
    : Buffer<AVPacket>(av_packet_alloc())
{
    if (!ptr) {
        throw std::runtime_error("av_packet_alloc");
    }
}

Packet::
~Packet()
{
    av_packet_free(&ptr);
}

//-------//
// Frame //
//-------//
Frame::
Frame()
    : Buffer<AVFrame>(av_frame_alloc())
{
    if (!ptr) {
        throw std::runtime_error("av_frame_alloc");
    }
}

Frame::
~Frame()
{
    av_frame_free(&ptr);
}

} // namespace fps
