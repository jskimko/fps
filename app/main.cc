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
    fps::Packet input;
    fps::Packet video_output;
    fps::Packet audio_output;

    // read data.
    while (auto p_in = reader.read_into(input)) {
        if (video_decoder.decode(p_in)) {
            while (auto f = video_decoder.read_into(frame)) {
                if (!video_encoder.encode(f)) { break; }
                while (auto p_out = video_encoder.read_into(video_output)) {
                    writer.write(p_out, fps::MediaType::VIDEO);
                }
            }
        } else if (audio_decoder.decode(p_in)) {
            while (auto f = audio_decoder.read_into(frame)) {
                if (!audio_encoder.encode(f)) { break; }
                while (auto p_out = audio_encoder.read_into(audio_output)) {
                    writer.write(p_out, fps::MediaType::AUDIO);
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
