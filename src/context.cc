extern "C" {
#include <libavformat/avformat.h>
}

#include <stdexcept>
#include "fps/context.hh"

namespace fps {

Context::
Context(std::string name, ContextType type)
    : fname(std::move(name)),
      context(nullptr)
{
    switch (type) {
        case ContextType::INPUT:
            if (avformat_open_input(&context, fname.c_str(), nullptr, nullptr) < 0) {
                throw std::runtime_error("avformat_open_input: " + fname);
            }

            if (avformat_find_stream_info(context, nullptr) < 0) {
                throw std::runtime_error("avformat_find_stream_info");
            }
            break;
        case ContextType::OUTPUT:
            avformat_alloc_output_context2(&context, nullptr, nullptr, fname.c_str());
            if (!context) {
                avformat_alloc_output_context2(&context, NULL, "mpeg", fname.c_str());
            }
            if (!context) {
                throw std::runtime_error("avformat_alloc_output_context2: " + fname);
            }
            break;
        default:
            throw std::runtime_error("unsupported ContextType");
    }
}


Context::
~Context()
{
    if (context) { avformat_close_input(&context); }
}

void
Context::
copy_metadata(Context const &ctx)
{
    av_dict_copy(&context->metadata, ctx.context->metadata, 0);
}

} // namespace fps
