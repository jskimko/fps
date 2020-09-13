#include <cstdio>
#include <cstdlib>
#include <stdexcept>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
}

#include "fps/fps.hh"
#include "private.hh"

namespace fps { 

//---------//
// Context //
//---------//
Context::
Context(std::string name, ContextType type)
    : fname(std::move(name)),
      context(nullptr)
{
    switch (type) {
        case ContextType::INPUT:
            if (avformat_open_input(&context, fname.c_str(), nullptr, nullptr) < 0) {
                throw std::runtime_error("avformat_open_input: " + fname);
            }

            if (avformat_find_stream_info(context, nullptr) < 0) {
                throw std::runtime_error("avformat_find_stream_info");
            }
            break;
        case ContextType::OUTPUT:
            avformat_alloc_output_context2(&context, nullptr, nullptr, fname.c_str());
            if (!context) {
                avformat_alloc_output_context2(&context, NULL, "mpeg", fname.c_str());
            }
            if (!context) {
                throw std::runtime_error("avformat_alloc_output_context2: " + fname);
            }
            break;
        default:
            throw std::runtime_error("unsupported ContextType");
    }
}


Context::
~Context()
{
    if (context) { avformat_close_input(&context); }
}

void
Context::
copy_metadata(Context const &ctx)
{
    av_dict_copy(&context->metadata, ctx.context->metadata, 0);
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

//--------//
// Writer //
//--------//
Writer::
Writer(Context const &ctx, Encoder const &ve, Encoder const &ae)
    : context(ctx),
      video_encoder(ve),
      audio_encoder(ae),
      video_stream(nullptr),
      audio_stream(nullptr)
{
    auto *fmt = ctx.context->oformat;
    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        add_stream(&video_stream, video_encoder);
    }

    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
        add_stream(&audio_stream, audio_encoder);
    }

    if (!(context.context->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&context.context->pb, context.fname.c_str(), AVIO_FLAG_WRITE) < 0) {
            throw std::runtime_error("avio_open");
        }
    }

    if (avformat_write_header(context.context, nullptr) < 0) {
        throw std::runtime_error("avformat_write_header");
    }
}

Writer::
~Writer()
{
    av_write_trailer(context.context);
    if (context.context->pb) { avio_closep(&context.context->pb); }
}

void
Writer::
add_stream(AVStream **stream, Encoder const &encoder)
{
    *stream = avformat_new_stream(context.context, nullptr);
    if (!*stream) {
        throw std::runtime_error("avformat_new_stream");
    }
    (*stream)->id = context.context->nb_streams-1;
    (*stream)->time_base = encoder.context->time_base;

    if (avcodec_parameters_from_context((*stream)->codecpar, encoder.context) < 0) {
        throw std::runtime_error("avcodec_parameters_from_context");
    }
}

bool
Writer::
write(Packet &packet, MediaType type) const
{
    AVStream *stream = nullptr;
    AVCodecContext *codec_ctx = nullptr;

    if (type == MediaType::VIDEO) {
        stream = video_stream;
        codec_ctx = video_encoder.context;
    } else if (type == MediaType::AUDIO) {
        stream = audio_stream;
        codec_ctx = audio_encoder.context;
    } else {
        throw std::runtime_error("unsupported MediaType");
    }

    av_packet_rescale_ts(packet.get(), codec_ctx->time_base, stream->time_base);
    packet.get()->stream_index = stream->index;

    return av_interleaved_write_frame(context.context, packet.get()) == 0;
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

void
Codec::
open()
{
    if (avcodec_open2(context, codec, nullptr) < 0) {
        throw std::runtime_error("avcodec_open2");
    }
}

//---------//
// Decoder //
//---------//
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

//---------//
// Encoder //
//---------//
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
