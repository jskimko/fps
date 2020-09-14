#include "fps/fps.hh"

#include <cstdio>
#include <stdexcept>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s input output\n", argv[0]);
        return 1;
    }

    const char *name_in = argv[1];
    const char *name_out = argv[2];

    try {

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

    // read data.
    while (auto p_in = reader.read_into(input)) {
        if (video_decoder.decode(p_in)) {
            while (auto f = video_decoder.read_into(frame)) {

                if (prev.is_valid()) {
                    int n = 0;
                    auto interp_frames = fps::Interpolator::linear(prev, frame, n);

                    for (auto &interp_frame : interp_frames) {
                        encode(video_encoder, video_output, interp_frame, fps::MediaType::VIDEO);
                    }
                }
                prev.copy_from(frame);

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

    } catch (std::exception &e) {
        fprintf(stderr, "%s: error: %s\n", argv[0], e.what());
        return 1;
    }

    return 0;
}
