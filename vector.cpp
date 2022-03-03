#include "vector.h"

namespace lab_07 {
std::size_t get_deg_2(std::size_t k) {
    if (k == 0) {
        return 0;
    }
    std::size_t x = 1;
    while (x < k) {
        x *= 2;
    }
    return x;
}
}  // namespace lab_07