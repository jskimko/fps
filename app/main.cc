#include "fps/fps.hh"

#include <cstdio>
#include <sys/stat.h>

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

    // i/o.
    fps::Reader reader(ctx);
    //fps::Writer writer(out_name);

    // find streams and codecs.
    fps::Decoder video_decoder(ctx, fps::MediaType::VIDEO);
    fps::Decoder audio_decoder(ctx, fps::MediaType::AUDIO);
    fps::Encoder video_encoder(ctx, video_decoder, fps::MediaType::VIDEO);
    //fps::Encoder audio_encoder(audio_decoder, fps::MediaType::AUDIO);

    // allocate buffers.
    fps::Frame frame;
    fps::Packet input;
    fps::Packet output;

    while (auto p_in = reader.read_into(input)) {
        if (video_decoder.decode(p_in)) {
            while (auto f = video_decoder.read_into(frame)) {
                printf("@@ frame %ld\n", f->pts);

                if (!video_encoder.encode(f)) { break; }
                while (auto p_out = video_encoder.read_into(output)) {
                    printf("@@ packet %ld\n", p_out->pts);
                    //writer.write(p_out);
                }
            }
        } else if (audio_decoder.decode(p_in)) {
        }
    }

    return 0;
}

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
