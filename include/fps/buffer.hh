#ifndef BUFFER_HH
#define BUFFER_HH

#include <functional>

extern "C" {
#include <libavformat/avformat.h>
}

namespace fps {

template <typename T>
class Buffer {
public:
    using type = T;
    T* get() const { return ptr; }

protected:
    Buffer(T* t) : ptr(t) {}
    T *ptr;
};

class Packet : public Buffer<AVPacket> {
public:
    Packet();
    ~Packet();
};

class Frame : public Buffer<AVFrame> {
public:
    Frame();
    ~Frame();

    bool is_valid() const;
    void copy_from(Frame const &frame);
};

template <typename Buf>
class Unref {
public:
    Unref(Buf *b, std::function<void(typename Buf::type*)> f) 
        : buf(b), unref(std::move(f)) {}
    ~Unref() { if (buf && buf->get()) unref(buf->get()); }

    operator Buf&() const { return *buf; }
    explicit operator bool() const { return buf && buf->get(); }
    typename Buf::type *operator->() { return buf->get(); }

    Buf &get() const { return *buf; }

private:
    Buf *buf;
    std::function<void(typename Buf::type*)> unref;
};

} // namespace fps

#endif // BUFFER_HH
