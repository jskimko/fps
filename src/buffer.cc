#include "fps/buffer.hh"

namespace fps {

Packet::
Packet()
    : Buffer<AVPacket>(av_packet_alloc())
{
    if (!ptr) {
        throw std::runtime_error("av_packet_alloc");
    }
}

Packet::
~Packet()
{
    av_packet_free(&ptr);
}

Frame::
Frame()
    : Buffer<AVFrame>(av_frame_alloc())
{
    if (!ptr) {
        throw std::runtime_error("av_frame_alloc");
    }
}

Frame::
~Frame()
{
    av_frame_free(&ptr);
}

bool 
Frame::
is_valid() const
{
    return ptr->data[0];
}

void
Frame::
copy_from(Frame const &frame)
{
    if (ptr) { av_frame_free(&ptr); }
    ptr = av_frame_clone(frame.ptr);
}

} // namespace fps
