#include "fps/fps.hh"

#include <cstdio>
#include <stdexcept>

int main(int argc, char *argv[]) {
    try {

    // parse arguments.
    if (argc < 3) {
        fprintf(stderr, "usage: %s input output [fps]\n", argv[0]);
        return 1;
    }

    const char *name_in = argv[1];
    const char *name_out = argv[2];

    int fps = 60;
    if (argc == 4) {
        fps = std::stoi(argv[3]);
        if (fps < 1) {
            fprintf(stderr, "error: desired fps must be greater than 1\n");
            return 1;
        }
    }

    // contexts.
    fps::Context ctx_in(name_in, fps::ContextType::INPUT);
    fps::Context ctx_out(name_out, fps::ContextType::OUTPUT);
    ctx_out.copy_metadata(ctx_in);

    // codecs.
    fps::Decoder video_decoder(ctx_in, fps::MediaType::VIDEO);
    fps::Decoder audio_decoder(ctx_in, fps::MediaType::AUDIO);
    fps::Encoder video_encoder(ctx_in, video_decoder, fps::MediaType::VIDEO);
    fps::Encoder audio_encoder(ctx_in, audio_decoder, fps::MediaType::AUDIO);

    // i/o.
    fps::Reader reader(ctx_in);
    fps::Writer writer(ctx_out, video_encoder, audio_encoder);

    // allocate buffers.
    fps::Frame frame;
    fps::Frame prev;
    fps::Packet input;
    fps::Packet video_output;
    fps::Packet audio_output;

    // common encoding logic.
    auto encode = [&writer](fps::Encoder &encoder, 
                            fps::Packet &packet,
                            fps::Frame &frame,
                            fps::MediaType type) {
        if (!encoder.encode(frame)) { return false; }
        while (auto p_out = encoder.read_into(packet)) {
            writer.write(p_out, type);
        }
        return true;
    };

    // interpolator.
    auto tbps = video_encoder.context->time_base.den / video_encoder.context->time_base.num;
    auto pts_step = tbps / fps;
    fps::Interpolator interpolator(pts_step);

    // read data.
    while (auto p_in = reader.read_into(input)) {
        if (video_decoder.decode(p_in)) {
            while (auto f = video_decoder.read_into(frame)) {
                if (prev.is_valid()) {
                    auto interp_frames = interpolator.linear(prev, f);

                    for (auto &interp_frame : interp_frames) {
                        encode(video_encoder, video_output, interp_frame, fps::MediaType::VIDEO);
                    }

                }
                prev.copy_from(f);

                if (!encode(video_encoder, video_output, f, fps::MediaType::VIDEO)) {
                    break;
                }
            }
        } else if (audio_decoder.decode(p_in)) {
            while (auto f = audio_decoder.read_into(frame)) {
                if (!encode(audio_encoder, audio_output, f, fps::MediaType::AUDIO)) {
                    break;
                }
            }
        }
    }

    printf("%s: interpolated %lu frames\n", argv[0], interpolator.total);

    } catch (std::exception &e) {
        fprintf(stderr, "%s: error: %s\n", argv[0], e.what());
        return 1;
    }

    return 0;
}
