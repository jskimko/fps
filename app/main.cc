#include <cstdio>
#include <sys/stat.h>

extern "C" {
#include <libavformat/avformat.h>
}

#include "fps/fps.hh"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("usage: %s file.mp4\n", argv[0]);
        return fps::ERROR_USAGE;
    }

    const char *fname = argv[1];
    int rc = 0;

    struct stat buf;
    if ((rc = stat(fname, &buf)) != 0) {
        fprintf(stderr, "error: stat: could not find file '%s'\n", fname);
        return rc;
    }

    // open file.
    AVFormatContext *fmt_ctx = nullptr;
    if ((rc = avformat_open_input(&fmt_ctx, fname, nullptr, nullptr)) < 0) {
        fprintf(stderr, "error: avformat_open_input: %s\n", fname);
        return rc;
    }

    // find additional information.
    if ((rc = avformat_find_stream_info(fmt_ctx, nullptr)) < 0) {
        fprintf(stderr, "error: avformat_find_stream_info\n");
        return rc;
    }

    // find video stream.
    AVCodec *dec = nullptr;
    int vs_idx;
    if ((rc = vs_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0)) < 0) {
        fprintf(stderr, "error: av_find_best_stream: could not find video stream\n");
        return rc;
    }

    // create decoder context.
    AVCodecContext *dec_ctx = nullptr;
    if ((dec_ctx = avcodec_alloc_context3(dec)) == nullptr) {
        fprintf(stderr, "error: avcodec_alloc_context3: could not allocate decoder context\n");
        return fps::ERROR_FFMPEG;
    }

    // fill codec context.
    avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[vs_idx]->codecpar);

    // init video decoder.
    if ((rc = avcodec_open2(dec_ctx, dec, nullptr)) < 0) {
        fprintf(stderr, "error: avcodec_open2: could not open video decoder\n");
        return rc;
    }

    // allocate frame buffer.
    AVFrame *frame = nullptr;

    if ((frame = av_frame_alloc()) == nullptr) {
        fprintf(stderr, "error: av_frame_alloc\n");
        return fps::ERROR_FFMPEG;
    }

    // read packets (compressed).
    AVPacket pkt;
    while ((rc = av_read_frame(fmt_ctx, &pkt)) == 0) {
        // check if video stream packet.
        if (pkt.stream_index == vs_idx) {
            // send packet to decoder.
            if ((rc = avcodec_send_packet(dec_ctx, &pkt)) < 0) {
                fprintf(stderr, "error: avcodec_send_packet: could not send packet to decoder\n");
                break;
            }

            // get frames (decoded).
            // avcodec_receive_frame calls av_frame_unref automatically.
            while ((rc = avcodec_receive_frame(dec_ctx, frame)) == 0) {
                printf("@@ pts bets %ld %ld\n", frame->pts, frame->best_effort_timestamp);
//                frame->pts = frame->best_effort_timestamp;
            }

            // unref the last frame.
            av_frame_unref(frame);
        }
        av_packet_unref(&pkt);
    }

    // clean up.
    avcodec_free_context(&dec_ctx);
    av_frame_free(&frame);

    return rc;
}
