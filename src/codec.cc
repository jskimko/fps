#include "fps/codec.hh"
#include "fps/context.hh"
#include "fps/buffer.hh"

extern "C" {
#include <libavformat/avformat.h>
}

namespace fps {

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

void
Codec::
open()
{
    if (avcodec_open2(context, codec, nullptr) < 0) {
        throw std::runtime_error("avcodec_open2");
    }
}

Decoder::
Decoder(Context &ctx, MediaType type)
    : Codec(),
      stream_index(-1)
{
    stream_index = av_find_best_stream(ctx.context, (AVMediaType) type, -1, -1, &codec, 0);
    if (stream_index < 0) {
        throw std::runtime_error("av_find_best_stream");
    }

    context = avcodec_alloc_context3(codec);
    if (!context) {
        throw std::runtime_error("avcodec_alloc_context3");
    }

    if (avcodec_parameters_to_context(context, ctx.context->streams[stream_index]->codecpar) < 0) {
        throw std::runtime_error("avcodec_parameters_to_context");
    }

    open();
}

bool
Decoder::
decode(Packet &packet)
{
    if (packet.get()->stream_index != stream_index) { return false; }
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

Encoder::
Encoder(Context &ctx, Decoder &decoder, MediaType type)
    : Codec()
{
    codec = avcodec_find_encoder(decoder.context->codec_id);
    if (!codec) {
        throw std::runtime_error("avcodec_find_encoder");
    }

    context = avcodec_alloc_context3(codec);
    if (!context) {
        throw std::runtime_error("avcodec_alloc_context3");
    }

    context->time_base = ctx.context->streams[decoder.stream_index]->time_base;
    context->bit_rate = decoder.context->bit_rate;

    switch (type) {
        case MediaType::VIDEO:
            context->width = decoder.context->width;
            context->height = decoder.context->height;
            context->pix_fmt = decoder.context->pix_fmt;
            context->framerate = decoder.context->framerate;
            context->gop_size = decoder.context->gop_size;
            context->max_b_frames = decoder.context->max_b_frames;
            break;
        case MediaType::AUDIO:
            context->sample_fmt = decoder.context->sample_fmt;
            context->sample_rate = decoder.context->sample_rate;
            context->channel_layout = decoder.context->channel_layout;
            context->channels = decoder.context->channels;
            break;
        default:
            throw std::runtime_error("unsupported MediaType");
    }

    open();
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

} // namespace fps
