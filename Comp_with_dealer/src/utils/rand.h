// By Boshi Yuan
/// @file

#ifndef MD_ML_RAND_H
#define MD_ML_RAND_H


#include <random>
#include <algorithm>
#include <concepts>

namespace md_ml {

/// Generate a random number of type Tp
/// @tparam Tp The type of the random number to be generated, should be an integral type (e.g., uint64_t, uint8_t)
/// @return The generated random number of type Tp
template <std::integral Tp>
inline
Tp getRand() {
    using EngOutput_t = unsigned; // EngOutput_t is the output type of the random number generator
    
    static std::random_device rd;
    static std::independent_bits_engine<std::default_random_engine, 8 * sizeof(EngOutput_t), EngOutput_t> rng(rd());

    // 处理小于 EngOutput_t 的类型
    if constexpr (sizeof(Tp) < sizeof(EngOutput_t)) {
        EngOutput_t temp = rng();
        return static_cast<Tp>(temp);
    } 
    // 处理大于等于 EngOutput_t 的类型
    else {
        // Output of std::independent_bits_engine must be EngOutput_t, so we use sizeof to determine buffer length
        EngOutput_t buf[sizeof(Tp) / sizeof(EngOutput_t)];
        std::generate(std::begin(buf), std::end(buf), []() { return rng(); });

        return *reinterpret_cast<Tp*>(buf);
    }
}

} // namespace md_ml

#endif //MD_ML_RAND_H
