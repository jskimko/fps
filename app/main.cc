#include "fps/fps.hh"

#include <cstdio>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage: %s input output\n", argv[0]);
        return 1;
    }

    const char *name_in = argv[1];
    const char *name_out = argv[2];

    // check if input file exists.
    struct stat buf;
    if (stat(name_in, &buf) != 0) {
        fprintf(stderr, "error: stat: could not find file '%s'\n", name_in);
        return 1;
    }

    // get contexts.
    fps::Context ctx_in(name_in, fps::ContextType::INPUT);
    fps::Context ctx_out(name_out, fps::ContextType::OUTPUT);

    // find streams and codecs.
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
    fps::Packet output;

    // read data.
    while (auto p_in = reader.read_into(input)) {
        if (video_decoder.decode(p_in)) {
            while (auto f = video_decoder.read_into(frame)) {
                printf("@@ frame %ld\n", f->pts);

                if (!video_encoder.encode(f)) { break; }
                while (auto p_out = video_encoder.read_into(output)) {
                    printf("@@ packet %ld\n", p_out->pts);
                    writer.write(p_out);
                }

            }
        } else if (audio_decoder.decode(p_in)) {
        }
    }

    return 0;
}
