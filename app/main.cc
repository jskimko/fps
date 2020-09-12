#include <cstdio>
#include <sys/stat.h>

#include "fps/fps.hh"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage: %s input output\n", argv[0]);
        return 1;
    }

    const char *in_name = argv[1];
    const char *out_name = argv[2];

    // check if input file exists.
    struct stat buf;
    if (stat(in_name, &buf) != 0) {
        fprintf(stderr, "error: stat: could not find file '%s'\n", in_name);
        return 1;
    }

    // open input file.
    fps::Context ctx(in_name);

    // find streams and codecs.
    fps::Codec video_codec(ctx, fps::MediaType::VIDEO);
    fps::Codec audio_codec(ctx, fps::MediaType::AUDIO);

    // allocate buffers.
    fps::Processor proc(ctx);

//    // read packets (compressed).
//    while ((rc = av_read_frame(fmt_ctx, pkt)) == 0) {
//        if (pkt->stream_index == video_stream_idx) {
//            rc = process(video_ctx, pkt, frame);
//
//        } else if (pkt->stream_index == audio_stream_idx) {
//            rc = process(audio_ctx, pkt, frame);
//        }
//        av_packet_unref(pkt);
//        if (rc < 0) { break; }
//    }

    return 0;
}

//int process(AVCodecContext *ctx, AVPacket *pkt, AVFrame *frame) {
//    int rc = 0;
//
//    // send packet to context.
//    if ((rc = avcodec_send_packet(ctx, pkt)) < 0) {
//        fprintf(stderr, "error: avcodec_send_packet: could not send packet to decoder\n");
//        return rc;
//    }
//
//    // get frames.
//    // avcodec_receive_frame calls av_frame_unref automatically.
//    while ((rc = avcodec_receive_frame(ctx, frame)) == 0) {
//        printf("@@ pts bets %ld %ld\n", frame->pts, frame->best_effort_timestamp);
//
//        // encode frame.
//        if ((rc = avcodec_send_frame(ctx, frame)) < 0) {
//            fprintf(stderr, "error: avcodec_send_frame\n");
//            break;
//        }
//
//        // get packet.
//        while ((rc = avcodec_receive_packet(ctx, pkt)) == 0) {
//        }
//
//    }
//
//    // unref the last frame.
//    av_frame_unref(frame);
//
//    return rc;
//}
