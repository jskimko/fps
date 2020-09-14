#ifndef INTERPOLATE_HH
#define INTERPOLATE_HH

#include <vector>
#include <algorithm>
#include <type_traits>

#include "fps/buffer.hh"

namespace fps {

class Interpolator {
public:
    Interpolator(uint64_t s) : step(s) {}

    std::vector<Frame> linear(Frame const &a, Frame const &b) {
        if (b.get()->pts < a.get()->pts) { return {}; }

        using pts_t = decltype(a.get()->pts);
        pts_t begin = a.get()->pts / step;
        pts_t end = b.get()->pts / step;
        pts_t n = (begin == end) ? 0 : end - begin - 1;
        pts_t pts_step = (b.get()->pts - a.get()->pts) / (n + 1);

        std::vector<Frame> ret(n);
        total += n;

        for (int i=0; i<n; i++) {
            ret[i].copy_from(a);
            Frame &f = ret[i];
            f.get()->pts += pts_step * (i + 1);

            int len = sizeof(b.get()->data) / sizeof(b.get()->data[0]);
            for (int j=0; j<len; j++) {
                auto *plane_a = a.get()->data[j];
                auto *plane_b  = b.get()->data[j];
                auto *plane_f  = f.get()->data[j];
                auto linesize = b.get()->linesize[j];

                if (!plane_a || !plane_b || !plane_f) { continue; }

                using data_t = std::remove_reference<decltype(plane_f[0])>::type;
                std::transform(&plane_a[0], &plane_a[linesize],
                               &plane_b[0], &plane_f[0],
                               [i, n](data_t a, data_t b) -> data_t {
                                   if (a == b) {
                                       return a;
                                   } else if (a < b) {
                                       return a + (b - a) / (n + 1) * (i + 1);
                                   } else {
                                       return b + (a - b) / (n + 1) * (i + 1);
                                   }
                               }
                );
            }
        }
        return ret;
    }

    uint64_t step;
    uint64_t total;
};

} // namespace fps

#endif // INTERPOLATE_HH
