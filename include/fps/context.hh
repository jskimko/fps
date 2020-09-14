#ifndef CONTEXT_HH
#define CONTEXT_HH

#include <string>

#include "fps/enum.hh"

extern "C" {
struct AVFormatContext;
};

namespace fps {

class Context {
public:
    Context(std::string fname, ContextType type);
    ~Context();

    void copy_metadata(Context const &ctx);

    std::string fname;
    AVFormatContext *context;
};

} // namespace fps

#endif // CONTEXT_HH
