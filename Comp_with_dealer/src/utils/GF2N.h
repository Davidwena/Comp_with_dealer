// By Kaiwen Wang
/// @file 高效的伽罗瓦域 GF(2^N) 实现
/// 用于 TinyOT 协议中的 MAC 计算

#ifndef MD_ML_GF2N_H
#define MD_ML_GF2N_H

#include <cstdint>
#include <cstring>
#include <array>
#include <iostream>

#ifdef __x86_64__
#include <immintrin.h>
#include <wmmintrin.h>
#endif

namespace md_ml {

/// GF(2^32) 实现
/// 不可约多项式: f(x) = x^32 + x^2 + 1
/// 与 MD-SONIC 保持一致
class GF2_32 {
public:
    uint32_t value;

    // 构造函数
    constexpr GF2_32() : value(0) {}
    constexpr explicit GF2_32(uint32_t val) : value(val) {}

    // 加法（在 GF(2^n) 上就是 XOR）
    constexpr GF2_32 operator+(const GF2_32& other) const {
        return GF2_32(value ^ other.value);
    }

    constexpr GF2_32& operator+=(const GF2_32& other) {
        value ^= other.value;
        return *this;
    }

    // 减法（在 GF(2^n) 上等于加法，即 XOR）
    constexpr GF2_32 operator-(const GF2_32& other) const {
        return GF2_32(value ^ other.value);
    }

    constexpr GF2_32& operator-=(const GF2_32& other) {
        value ^= other.value;
        return *this;
    }

    // 乘法（GF(2^32) 上的多项式乘法）
    GF2_32 operator*(const GF2_32& other) const;

    GF2_32& operator*=(const GF2_32& other) {
        *this = *this * other;
        return *this;
    }

    // 与布尔值的乘法（用于 MAC 计算）
    constexpr GF2_32 operator*(bool bit) const {
        return bit ? *this : GF2_32(0);
    }

    // 比较运算
    constexpr bool operator==(const GF2_32& other) const {
        return value == other.value;
    }

    constexpr bool operator!=(const GF2_32& other) const {
        return value != other.value;
    }

    // 判断是否为零
    constexpr bool isZero() const {
        return value == 0;
    }

    // 零元和单位元
    static constexpr GF2_32 zero() { return GF2_32(0); }
    static constexpr GF2_32 one() { return GF2_32(1); }

    // 输出
    friend std::ostream& operator<<(std::ostream& os, const GF2_32& gf) {
        os << gf.value;
        return os;
    }

    // 输入
    friend std::istream& operator>>(std::istream& is, GF2_32& gf) {
        is >> gf.value;
        return is;
    }

private:
    // Carry-less 乘法（64位结果）
    static uint64_t clmul32(uint32_t a, uint32_t b);

    // 模约简: 将 64位结果约简到 32位，模不可约多项式
    // f(x) = x^32 + x^2 + 1
    static uint32_t reduce(uint64_t product);
};

/// GF(2^64) 实现
/// 不可约多项式: f(x) = x^64 + x^4 + x^3 + 1
/// 与 MD-SONIC 保持一致
class GF2_64 {
public:
    uint64_t value;

    // 构造函数
    constexpr GF2_64() : value(0) {}
    constexpr explicit GF2_64(uint64_t val) : value(val) {}

    // 加法（在 GF(2^n) 上就是 XOR）
    constexpr GF2_64 operator+(const GF2_64& other) const {
        return GF2_64(value ^ other.value);
    }

    constexpr GF2_64& operator+=(const GF2_64& other) {
        value ^= other.value;
        return *this;
    }

    // 减法（在 GF(2^n) 上等于加法，即 XOR）
    constexpr GF2_64 operator-(const GF2_64& other) const {
        return GF2_64(value ^ other.value);
    }

    constexpr GF2_64& operator-=(const GF2_64& other) {
        value ^= other.value;
        return *this;
    }

    // 乘法（GF(2^64) 上的多项式乘法）
    GF2_64 operator*(const GF2_64& other) const;

    GF2_64& operator*=(const GF2_64& other) {
        *this = *this * other;
        return *this;
    }

    // 与布尔值的乘法（用于 MAC 计算）
    constexpr GF2_64 operator*(bool bit) const {
        return bit ? *this : GF2_64(0);
    }

    // 比较运算
    constexpr bool operator==(const GF2_64& other) const {
        return value == other.value;
    }

    constexpr bool operator!=(const GF2_64& other) const {
        return value != other.value;
    }

    // 判断是否为零
    constexpr bool isZero() const {
        return value == 0;
    }

    // 零元和单位元
    static constexpr GF2_64 zero() { return GF2_64(0); }
    static constexpr GF2_64 one() { return GF2_64(1); }

    // 输出
    friend std::ostream& operator<<(std::ostream& os, const GF2_64& gf) {
        os << gf.value;
        return os;
    }

    // 输入
    friend std::istream& operator>>(std::istream& is, GF2_64& gf) {
        is >> gf.value;
        return is;
    }

private:
    // Carry-less 乘法（128位结果）
    static void clmul64(uint64_t a, uint64_t b, uint64_t& low, uint64_t& high);

    // 模约简: 将 128位结果约简到 64位，模不可约多项式
    // f(x) = x^64 + x^4 + x^3 + 1
    static uint64_t reduce(uint64_t low, uint64_t high);
};


//
// ==================== GF2_32 实现 ====================
//

// Carry-less 乘法（软件实现）
inline uint64_t GF2_32::clmul32(uint32_t a, uint32_t b) {
#if defined(__x86_64__) && defined(__PCLMUL__)
    // 使用 PCLMULQDQ 指令（硬件加速）
    __m128i va = _mm_set_epi64x(0, a);
    __m128i vb = _mm_set_epi64x(0, b);
    __m128i vc = _mm_clmulepi64_si128(va, vb, 0x00);
    return _mm_extract_epi64(vc, 0);
#else
    // 软件实现：位并行算法
    uint64_t result = 0;
    uint64_t temp_a = a;
    uint64_t temp_b = b;

    for (int i = 0; i < 32; ++i) {
        if (temp_b & 1) {
            result ^= temp_a;
        }
        temp_a <<= 1;
        temp_b >>= 1;
    }

    return result;
#endif
}

// 模约简: f(x) = x^32 + x^2 + 1
// 在二进制中: 1 0000 0000 0000 0000 0000 0000 0000 0101 = 0x1_0000_0005
inline uint32_t GF2_32::reduce(uint64_t product) {
    // 约简策略: x^32 ≡ x^2 + 1 (mod f(x))
    // 多项式 p(x) = x^2 + 1 = 0x5

    uint32_t low = static_cast<uint32_t>(product);
    uint32_t high = static_cast<uint32_t>(product >> 32);

    // 高效约简：处理高32位
    // 对于 high 中的每个比特 i，如果设置，则 low ^= (0x5 << i)

    // 处理 high 的低30位（因为 i+2 < 32 对于 i < 30）
    low ^= high;                    // x^32 的 1 项
    low ^= (high << 2);             // x^32 的 x^2 项

    // 处理 high 的高2位（位30-31），它们会产生超过32位的结果
    uint32_t overflow = high >> 30;
    if (overflow) {
        // 这些位需要递归约简
        low ^= overflow;            // 1 项
        low ^= (overflow << 2);     // x^2 项

        // 再次检查是否有新的溢出（只有 x^2 项可能溢出）
        uint32_t overflow2 = (overflow >> 30);
        if (overflow2) {
            low ^= overflow2;       // 1 项
            // overflow2 << 2 不会超过32位，因为 overflow2 最多是 2 位
        }
    }

    return low;
}

// GF(2^32) 乘法
inline GF2_32 GF2_32::operator*(const GF2_32& other) const {
    // 步骤1: Carry-less 乘法得到 64位结果
    uint64_t product = clmul32(value, other.value);

    // 步骤2: 模约简到 32位
    uint32_t result = reduce(product);

    return GF2_32(result);
}


//
// ==================== GF2_64 实现 ====================
//

// Carry-less 乘法（软件实现）
inline void GF2_64::clmul64(uint64_t a, uint64_t b, uint64_t& low, uint64_t& high) {
#if defined(__x86_64__) && defined(__PCLMUL__)
    // 使用 PCLMULQDQ 指令（硬件加速）
    __m128i va = _mm_set_epi64x(0, a);
    __m128i vb = _mm_set_epi64x(0, b);
    __m128i vc = _mm_clmulepi64_si128(va, vb, 0x00);
    low = _mm_extract_epi64(vc, 0);
    high = _mm_extract_epi64(vc, 1);
#else
    // 软件实现：Karatsuba 算法
    uint64_t a_low = a & 0xFFFFFFFF;
    uint64_t a_high = a >> 32;
    uint64_t b_low = b & 0xFFFFFFFF;
    uint64_t b_high = b >> 32;

    // 分别计算
    uint64_t z0 = 0, z1 = 0, z2 = 0;

    // z0 = a_low * b_low (64位结果，实际最多64位)
    for (int i = 0; i < 32; ++i) {
        if (b_low & (1ULL << i)) {
            z0 ^= (a_low << i);
        }
    }

    // z2 = a_high * b_high
    for (int i = 0; i < 32; ++i) {
        if (b_high & (1ULL << i)) {
            z2 ^= (a_high << i);
        }
    }

    // z1 = (a_low + a_high) * (b_low + b_high) - z0 - z2
    uint64_t a_sum = a_low ^ a_high;
    uint64_t b_sum = b_low ^ b_high;
    z1 = 0;
    for (int i = 0; i < 32; ++i) {
        if (b_sum & (1ULL << i)) {
            z1 ^= (a_sum << i);
        }
    }
    z1 ^= z0 ^ z2;

    // 组合: result = z0 + z1 * 2^32 + z2 * 2^64
    low = z0 ^ (z1 << 32);
    high = (z1 >> 32) ^ z2;
#endif
}

// 模约简: f(x) = x^64 + x^4 + x^3 + 1
// 在二进制中: 1 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 1001
//           = 0x1_0000_0000_0000_0019
inline uint64_t GF2_64::reduce(uint64_t low, uint64_t high) {
    // 约简策略: x^64 ≡ x^4 + x^3 + 1 (mod f(x))
    // 多项式 p(x) = x^4 + x^3 + 1 = 0x19

    // 高效约简：处理高64位
    // 对于 high 中的每个比特 i，如果设置，则 low ^= (0x19 << i)

    // 处理 high 的低60位（因为 i+4 < 64 对于 i < 60）
    low ^= high;                    // x^64 的 1 项
    low ^= (high << 3);             // x^64 的 x^3 项
    low ^= (high << 4);             // x^64 的 x^4 项

    // 处理 high 的高4位（位60-63），它们会产生超过64位的结果
    uint64_t overflow = high >> 60;
    if (overflow) {
        // 这些位需要递归约简
        low ^= overflow;            // 1 项
        low ^= (overflow << 3);     // x^3 项
        low ^= (overflow << 4);     // x^4 项

        // 再次检查是否有新的溢出（只有 x^4 项可能溢出）
        uint64_t overflow2 = (overflow >> 60);
        if (overflow2) {
            low ^= overflow2;       // 1 项
            low ^= (overflow2 << 3); // x^3 项
            // overflow2 << 4 不会超过64位
        }
    }

    return low;
}

// GF(2^64) 乘法
inline GF2_64 GF2_64::operator*(const GF2_64& other) const {
    // 步骤1: Carry-less 乘法得到 128位结果
    uint64_t low, high;
    clmul64(value, other.value, low, high);

    // 步骤2: 模约简到 64位
    uint64_t result = reduce(low, high);

    return GF2_64(result);
}


} // namespace md_ml

#endif // MD_ML_GF2N_H