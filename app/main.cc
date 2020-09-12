#include <cstdio>
#include <sys/stat.h>
#include <fstream>

extern "C" {
#include <libavformat/avformat.h>
}

#include "fps/fps.hh"

int process(AVCodecContext *ctx, AVPacket *pkt, AVFrame *frame);
int add_stream(OutputStream *ostream, AVFormatContext *fmt_ctx, AVCodec **codec, enum AVCodecID codec_id);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage: %s input output\n", argv[0]);
        return fps::ERROR_USAGE;
    }

    const char *in_name = argv[1];
    const char *out_name = argv[2];
    int rc = 0;

    struct stat buf;
    if ((rc = stat(in_name, &buf)) != 0) {
        fprintf(stderr, "error: stat: could not find file '%s'\n", in_name);
        return rc;
    }

    //-----------------//
    //      INPUT      //
    //-----------------//

    // open file.
    AVFormatContext *fmt_ctx = nullptr;
    if ((rc = avformat_open_input(&fmt_ctx, in_name, nullptr, nullptr)) < 0) {
        fprintf(stderr, "error: avformat_open_input: %s\n", in_name);
        return rc;
    }

    // find stream info.
    if ((rc = avformat_find_stream_info(fmt_ctx, nullptr)) < 0) {
        fprintf(stderr, "error: avformat_find_stream_info\n");
        return rc;
    }

    // find streams and codecs.
    AVCodec *video_codec = nullptr;
    AVCodec *audio_codec = nullptr;
    int video_stream_idx;
    int audio_stream_idx;

    if ((rc = video_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &video_codec, 0)) < 0) {
        fprintf(stderr, "error: av_find_best_stream (video)\n");
        return rc;
    }


    if ((rc = audio_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &audio_codec, 0)) < 0) {
        fprintf(stderr, "error: av_find_best_stream (audio)\n");
        return rc;
    }

    // create codecs contexts.
    AVCodecContext *video_ctx = avcodec_alloc_context3(video_codec);
    AVCodecContext *audio_ctx = avcodec_alloc_context3(audio_codec);

    avcodec_parameters_to_context(video_ctx, fmt_ctx->streams[video_stream_idx]->codecpar);
    avcodec_parameters_to_context(audio_ctx, fmt_ctx->streams[audio_stream_idx]->codecpar);

    if ((rc = avcodec_open2(video_ctx, video_codec, nullptr)) < 0) {
        fprintf(stderr, "error: avcodec_open2: could not open video codex\n");
        return rc;
    }
    if ((rc = avcodec_open2(audio_ctx, audio_codec, nullptr)) < 0) {
        fprintf(stderr, "error: avcodec_open2: could not open audio codex\n");
        return rc;
    }

    // dump input info.
    av_dump_format(fmt_ctx, 0, in_name, 0);

    //------------------//
    //      OUTPUT      //
    //------------------//
    //AVOutputFormat *out_fmt = fmt_ctx->oformat;

    //-------------------//
    //      PROCESS      //
    //-------------------//

    // allocate frame buffer.
    AVFrame *frame = nullptr;
    if ((frame = av_frame_alloc()) == nullptr) {
        fprintf(stderr, "error: av_frame_alloc\n");
        return fps::ERROR_FFMPEG;
    }

    // allocate packet buffer.
    AVPacket *pkt = nullptr;
    if ((pkt = av_packet_alloc()) == nullptr) {
        fprintf(stderr, "error: av_packet_alloc\n");
        return fps::ERROR_FFMPEG;
    }

    // read packets (compressed).
    while ((rc = av_read_frame(fmt_ctx, pkt)) == 0) {
        if (pkt->stream_index == video_stream_idx) {
            rc = process(video_ctx, pkt, frame);

        } else if (pkt->stream_index == audio_stream_idx) {
            rc = process(audio_ctx, pkt, frame);
        }
        av_packet_unref(pkt);
        if (rc < 0) { break; }
    }

    // clean up.
    avcodec_free_context(&video_ctx);
    avcodec_free_context(&audio_ctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    return rc;
}

int process(AVCodecContext *ctx, AVPacket *pkt, AVFrame *frame) {
    int rc = 0;

    // send packet to context.
    if ((rc = avcodec_send_packet(ctx, pkt)) < 0) {
        fprintf(stderr, "error: avcodec_send_packet: could not send packet to decoder\n");
        return rc;
    }

    // get frames.
    // avcodec_receive_frame calls av_frame_unref automatically.
    while ((rc = avcodec_receive_frame(ctx, frame)) == 0) {
        printf("@@ pts bets %ld %ld\n", frame->pts, frame->best_effort_timestamp);

        // encode frame.
        if ((rc = avcodec_send_frame(ctx, frame)) < 0) {
            fprintf(stderr, "error: avcodec_send_frame\n");
            break;
        }

        // get packet.
        while ((rc = avcodec_receive_packet(ctx, pkt)) == 0) {
        }

    }

    // unref the last frame.
    av_frame_unref(frame);

    return rc;
}
