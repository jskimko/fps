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

    struct stat buf;
    if (stat(fname, &buf) != 0) {
        printf("error: could not find file '%s'\n", fname);
        return fps::ERROR_GENERIC;
    }

    AVFormatContext *fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, argv[1], nullptr, nullptr) < 0) {
        return fps::ERROR_FFMPEG;
    }

    return fps::SUCCESS;
}
