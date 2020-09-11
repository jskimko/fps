#include <cstdio>
#include <sys/stat.h>

extern "C" {
#include <libavformat/avformat.h>
}

#include "fps/fps.hh"

int decode(AVCodecContext *ctx, AVPacket *pkt, AVFrame *frame);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage: %s input output\n", argv[0]);
        return fps::ERROR_USAGE;
    }

    const char *fin = argv[1];
    const char *fout = argv[2];
    int rc = 0;

    struct stat buf;
    if ((rc = stat(fin, &buf)) != 0) {
        fprintf(stderr, "error: stat: could not find file '%s'\n", fin);
        return rc;
    }

    // open file.
    AVFormatContext *fmt_ctx = nullptr;
    if ((rc = avformat_open_input(&fmt_ctx, fin, nullptr, nullptr)) < 0) {
        fprintf(stderr, "error: avformat_open_input: %s\n", fin);
        return rc;
    }

    // find stream info.
    if ((rc = avformat_find_stream_info(fmt_ctx, nullptr)) < 0) {
        fprintf(stderr, "error: avformat_find_stream_info\n");
        return rc;
    }

    // find streams and decoders.
    AVCodec *video_dec = nullptr;
    AVCodec *audio_dec = nullptr;
    int video_stream_idx;
    int audio_stream_idx;

    if ((rc = video_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &video_dec, 0)) < 0) {
        fprintf(stderr, "error: av_find_best_stream (video)\n");
        return rc;
    }


    if ((rc = audio_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &audio_dec, 0)) < 0) {
        fprintf(stderr, "error: av_find_best_stream (audio)\n");
        return rc;
    }

    // create decoder contexts.
    AVCodecContext *video_dec_ctx = avcodec_alloc_context3(video_dec);
    AVCodecContext *audio_dec_ctx = avcodec_alloc_context3(audio_dec);

    avcodec_parameters_to_context(video_dec_ctx, fmt_ctx->streams[video_stream_idx]->codecpar);
    avcodec_parameters_to_context(audio_dec_ctx, fmt_ctx->streams[audio_stream_idx]->codecpar);

    if ((rc = avcodec_open2(video_dec_ctx, video_dec, nullptr)) < 0) {
        fprintf(stderr, "error: avcodec_open2: could not open video decoder\n");
        return rc;
    }
    if ((rc = avcodec_open2(audio_dec_ctx, audio_dec, nullptr)) < 0) {
        fprintf(stderr, "error: avcodec_open2: could not open audio decoder\n");
        return rc;
    }

    // dump input info.
    av_dump_format(fmt_ctx, 0, fin, 0);

    // allocate frame buffer.
    AVFrame *frame = nullptr;
    if ((frame = av_frame_alloc()) == nullptr) {
        fprintf(stderr, "error: av_frame_alloc\n");
        return fps::ERROR_FFMPEG;
    }

    // read packets (compressed).
    AVPacket pkt;
    while ((rc = av_read_frame(fmt_ctx, &pkt)) == 0) {
        if (pkt.stream_index == video_stream_idx) {
            decode(video_dec_ctx, &pkt, frame);
        } else if (pkt.stream_index == audio_stream_idx) {
            decode(audio_dec_ctx, &pkt, frame);
        }
        av_packet_unref(&pkt);
        if (rc < 0) { break; }
    }

    // clean up.
    avcodec_free_context(&video_dec_ctx);
    av_frame_free(&frame);

    return rc;
}

int decode(AVCodecContext *ctx, AVPacket *pkt, AVFrame *frame) {
    int rc = 0;

    // send packet to decoder.
    if ((rc = avcodec_send_packet(ctx, pkt)) < 0) {
        fprintf(stderr, "error: avcodec_send_packet: could not send packet to decoder\n");
        return rc;
    }

    // get frames (decoded).
    // avcodec_receive_frame calls av_frame_unref automatically.
    while ((rc = avcodec_receive_frame(ctx, frame)) == 0) {
        printf("@@ pts bets %ld %ld\n", frame->pts, frame->best_effort_timestamp);
    }

    // unref the last frame.
    av_frame_unref(frame);

    return rc;
}
