#include "fps/io.hh"
#include "fps/context.hh"
#include "fps/codec.hh"

extern "C" {
#include <libavformat/avformat.h>
}

namespace fps {

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

} // namespace fps
