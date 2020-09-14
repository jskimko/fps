#ifndef ENUM_HH
#define ENUM_HH

extern "C" {
#include <libavformat/avformat.h>
}

namespace fps {

enum class ContextType {
    INPUT,
    OUTPUT
};

enum class MediaType {
    VIDEO = AVMEDIA_TYPE_VIDEO,
    AUDIO = AVMEDIA_TYPE_AUDIO
};

} // namespace fps

#endif // ENUM_HH
