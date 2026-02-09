// By Kaiwen Wang
/// @file 高效的素数域 GF(p) 实现
/// 基于蒙哥马利乘法（Montgomery Multiplication）
/// 用于 MPC 协议中的模运算

#ifndef MD_ML_GFP_H
#define MD_ML_GFP_H

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <array>

namespace md_ml {

/// 蒙哥马利约简辅助函数
namespace montgomery_helper {
    // 计算模逆元: a^(-1) mod 2^64
    // 使用 Newton 迭代法
    inline uint64_t inv_mod_2pow64(uint64_t a) {
        // 要求 a 是奇数
        uint64_t inv = a;
        // Newton 迭代: x_{n+1} = x_n * (2 - a * x_n)
        // 收敛到 a^(-1) mod 2^k
        for (int i = 0; i < 6; ++i) {  // 2^(2^6) = 2^64
            inv = inv * (2 - a * inv);
        }
        return inv;
    }

    // 计算模逆元: a^(-1) mod 2^32
    inline uint32_t inv_mod_2pow32(uint32_t a) {
        uint32_t inv = a;
        for (int i = 0; i < 5; ++i) {  // 2^(2^5) = 2^32
            inv = inv * (2 - a * inv);
        }
        return inv;
    }

    // 扩展欧几里得算法求模逆元
    inline int64_t extended_gcd(int64_t a, int64_t b, int64_t& x, int64_t& y) {
        if (b == 0) {
            x = 1;
            y = 0;
            return a;
        }
        int64_t x1, y1;
        int64_t gcd = extended_gcd(b, a % b, x1, y1);
        x = y1;
        y = x1 - (a / b) * y1;
        return gcd;
    }

    // 计算 a^(-1) mod p
    inline uint64_t inv_mod_p(uint64_t a, uint64_t p) {
        int64_t x, y;
        int64_t gcd = extended_gcd(a, p, x, y);
        if (gcd != 1) {
            throw std::runtime_error("Modular inverse does not exist");
        }
        return (x % (int64_t)p + (int64_t)p) % (int64_t)p;
    }
}

/// GF(p) 实现 - 32位素数域
/// 使用蒙哥马利乘法进行高效模运算
/// R = 2^32, 元素以蒙哥马利形式存储: ā = aR mod p
template<uint32_t P>
class GFP_32 {
public:
    uint32_t value;  // 蒙哥马利形式的值

    // 静态常量
    static constexpr uint32_t MODULUS = P;
    static constexpr uint32_t R = 0;  // R = 2^32 (隐式)

    // 构造函数
    constexpr GFP_32() : value(0) {}
    
    // 从普通形式构造（自动转换到蒙哥马利形式）
    explicit GFP_32(uint32_t val) {
        value = to_montgomery(val);
    }

    // 从蒙哥马利形式直接构造（内部使用）
    static constexpr GFP_32 from_montgomery(uint32_t mont_val) {
        GFP_32 result;
        result.value = mont_val;
        return result;
    }

    // 转换到普通形式
    uint32_t to_normal() const {
        return from_montgomery_form(value);
    }

    // 加法
    GFP_32 operator+(const GFP_32& other) const {
        uint64_t sum = static_cast<uint64_t>(value) + other.value;
        uint32_t result = static_cast<uint32_t>(sum >= P ? sum - P : sum);
        return from_montgomery(result);
    }

    GFP_32& operator+=(const GFP_32& other) {
        *this = *this + other;
        return *this;
    }

    // 减法
    GFP_32 operator-(const GFP_32& other) const {
        uint32_t result = value >= other.value ? 
                         value - other.value : 
                         value + P - other.value;
        return from_montgomery(result);
    }

    GFP_32& operator-=(const GFP_32& other) {
        *this = *this - other;
        return *this;
    }

    // 取负
    GFP_32 operator-() const {
        return from_montgomery(value == 0 ? 0 : P - value);
    }

    // 乘法（蒙哥马利乘法）
    GFP_32 operator*(const GFP_32& other) const {
        uint32_t result = montgomery_mul(value, other.value);
        return from_montgomery(result);
    }

    GFP_32& operator*=(const GFP_32& other) {
        *this = *this * other;
        return *this;
    }

    // 标量乘法
    GFP_32 operator*(uint32_t scalar) const {
        return *this * GFP_32(scalar);
    }

    // 除法（乘以逆元）
    GFP_32 operator/(const GFP_32& other) const {
        return *this * other.inverse();
    }

    GFP_32& operator/=(const GFP_32& other) {
        *this = *this / other;
        return *this;
    }

    // 求逆元
    GFP_32 inverse() const {
        if (value == 0) {
            throw std::runtime_error("Division by zero in GF(p)");
        }
        // 转回普通形式，求逆，再转回蒙哥马利形式
        uint32_t normal = to_normal();
        uint32_t inv = static_cast<uint32_t>(
            montgomery_helper::inv_mod_p(normal, P)
        );
        return GFP_32(inv);
    }

    // 幂运算（平方-乘法）
    GFP_32 pow(uint64_t exp) const {
        if (exp == 0) return one();
        
        GFP_32 result = one();
        GFP_32 base = *this;
        
        while (exp > 0) {
            if (exp & 1) {
                result *= base;
            }
            base *= base;
            exp >>= 1;
        }
        
        return result;
    }

    // 比较运算
    bool operator==(const GFP_32& other) const {
        return value == other.value;
    }

    bool operator!=(const GFP_32& other) const {
        return value != other.value;
    }

    // 判断是否为零
    bool isZero() const {
        return value == 0;
    }

    // 零元和单位元
    static GFP_32 zero() { return from_montgomery(0); }
    static GFP_32 one() { 
        return GFP_32(1);  // 使用正常的构造函数会自动转换到蒙哥马利形式
    }

    // 输出
    friend std::ostream& operator<<(std::ostream& os, const GFP_32& gf) {
        os << gf.to_normal();
        return os;
    }

    // 输入
    friend std::istream& operator>>(std::istream& is, GFP_32& gf) {
        uint32_t val;
        is >> val;
        gf = GFP_32(val);
        return is;
    }

private:
    // 计算 R^2 mod p (编译期)
    static constexpr uint64_t compute_r2_mod_p() {
        // R = 2^32
        // R^2 = 2^64
        // 计算 2^64 mod p
        uint64_t r2 = 1;
        for (int i = 0; i < 64; ++i) {
            r2 = (r2 << 1) % P;
        }
        return r2;
    }

    // 计算 -p^(-1) mod 2^32 (编译期)
    static constexpr uint32_t compute_p_inv() {
        // 使用 Newton 迭代
        uint32_t inv = P;
        for (int i = 0; i < 5; ++i) {
            inv = inv * (2 - P * inv);
        }
        return -inv;  // 取负
    }

    // 转换到蒙哥马利形式: a -> aR mod p
    static uint32_t to_montgomery(uint32_t a) {
        // aR mod p = montgomery_mul(a, R^2 mod p)
        uint64_t r2 = compute_r2_mod_p();
        return montgomery_mul(a, static_cast<uint32_t>(r2));
    }

    // 从蒙哥马利形式转换: āR -> a mod p
    static uint32_t from_montgomery_form(uint32_t a_mont) {
        // aR * 1 * R^(-1) mod p = a mod p
        return montgomery_redc(static_cast<uint64_t>(a_mont));
    }

    // 蒙哥马利约简: T -> TR^(-1) mod p
    static uint32_t montgomery_redc(uint64_t T) {
        // REDC 算法
        // m = (T mod R) * p' mod R = (T * p') mod R
        uint32_t p_inv = compute_p_inv();
        uint32_t m = static_cast<uint32_t>(T) * p_inv;
        
        // t = (T + m * p) / R
        uint64_t t = (T + static_cast<uint64_t>(m) * P) >> 32;
        
        // 条件减法
        return static_cast<uint32_t>(t >= P ? t - P : t);
    }

    // 蒙哥马利乘法: āb̄ -> abR mod p
    static uint32_t montgomery_mul(uint32_t a, uint32_t b) {
        uint64_t T = static_cast<uint64_t>(a) * b;
        return montgomery_redc(T);
    }
};


/// GF(p) 实现 - 64位素数域
/// 使用蒙哥马利乘法进行高效模运算
/// R = 2^64, 元素以蒙哥马利形式存储: ā = aR mod p
template<uint64_t P>
class GFP_64 {
public:
    uint64_t value;  // 蒙哥马利形式的值

    // 静态常量
    static constexpr uint64_t MODULUS = P;

    // 构造函数
    constexpr GFP_64() : value(0) {}
    
    // 从普通形式构造（自动转换到蒙哥马利形式）
    explicit GFP_64(uint64_t val) {
        value = to_montgomery(val);
    }

    // 从蒙哥马利形式直接构造（内部使用）
    static GFP_64 from_montgomery(uint64_t mont_val) {
        GFP_64 result;
        result.value = mont_val;
        return result;
    }

    // 转换到普通形式
    uint64_t to_normal() const {
        return from_montgomery_form(value);
    }

    // 加法
    GFP_64 operator+(const GFP_64& other) const {
        __uint128_t sum = static_cast<__uint128_t>(value) + other.value;
        uint64_t result = static_cast<uint64_t>(sum >= P ? sum - P : sum);
        return from_montgomery(result);
    }

    GFP_64& operator+=(const GFP_64& other) {
        *this = *this + other;
        return *this;
    }

    // 减法
    GFP_64 operator-(const GFP_64& other) const {
        uint64_t result = value >= other.value ? 
                         value - other.value : 
                         value + P - other.value;
        return from_montgomery(result);
    }

    GFP_64& operator-=(const GFP_64& other) {
        *this = *this - other;
        return *this;
    }

    // 取负
    GFP_64 operator-() const {
        return from_montgomery(value == 0 ? 0 : P - value);
    }

    // 乘法（蒙哥马利乘法）
    GFP_64 operator*(const GFP_64& other) const {
        uint64_t result = montgomery_mul(value, other.value);
        return from_montgomery(result);
    }

    GFP_64& operator*=(const GFP_64& other) {
        *this = *this * other;
        return *this;
    }

    // 标量乘法
    GFP_64 operator*(uint64_t scalar) const {
        return *this * GFP_64(scalar);
    }

    // 除法（乘以逆元）
    GFP_64 operator/(const GFP_64& other) const {
        return *this * other.inverse();
    }

    GFP_64& operator/=(const GFP_64& other) {
        *this = *this / other;
        return *this;
    }

    // 求逆元
    GFP_64 inverse() const {
        if (value == 0) {
            throw std::runtime_error("Division by zero in GF(p)");
        }
        uint64_t normal = to_normal();
        uint64_t inv = montgomery_helper::inv_mod_p(normal, P);
        return GFP_64(inv);
    }

    // 幂运算（平方-乘法）
    GFP_64 pow(uint64_t exp) const {
        if (exp == 0) return one();
        
        GFP_64 result = one();
        GFP_64 base = *this;
        
        while (exp > 0) {
            if (exp & 1) {
                result *= base;
            }
            base *= base;
            exp >>= 1;
        }
        
        return result;
    }

    // 比较运算
    bool operator==(const GFP_64& other) const {
        return value == other.value;
    }

    bool operator!=(const GFP_64& other) const {
        return value != other.value;
    }

    // 判断是否为零
    bool isZero() const {
        return value == 0;
    }

    // 零元和单位元
    static GFP_64 zero() { return from_montgomery(0); }
    static GFP_64 one() { 
        return GFP_64(1);  // 使用正常的构造函数会自动转换到蒙哥马利形式
    }

    // 输出
    friend std::ostream& operator<<(std::ostream& os, const GFP_64& gf) {
        os << gf.to_normal();
        return os;
    }

    // 输入
    friend std::istream& operator>>(std::istream& is, GFP_64& gf) {
        uint64_t val;
        is >> val;
        gf = GFP_64(val);
        return is;
    }

private:
    // 计算 R^2 mod p (运行时)
    static __uint128_t compute_r2_mod_p() {
        // R = 2^64
        // R^2 = 2^128
        __uint128_t r2 = 1;
        for (int i = 0; i < 128; ++i) {
            r2 = (r2 << 1) % P;
        }
        return r2;
    }

    // 计算 -p^(-1) mod 2^64
    static uint64_t compute_p_inv() {
        uint64_t inv = montgomery_helper::inv_mod_2pow64(P);
        return -inv;  // 取负
    }

    // 转换到蒙哥马利形式: a -> aR mod p
    static uint64_t to_montgomery(uint64_t a) {
        __uint128_t r2 = compute_r2_mod_p();
        return montgomery_mul(a, static_cast<uint64_t>(r2));
    }

    // 从蒙哥马利形式转换: āR -> a mod p
    static uint64_t from_montgomery_form(uint64_t a_mont) {
        return montgomery_redc(static_cast<__uint128_t>(a_mont));
    }

    // 蒙哥马利约简: T -> TR^(-1) mod p
    static uint64_t montgomery_redc(__uint128_t T) {
        // m = (T mod R) * p' mod R
        uint64_t p_inv = compute_p_inv();
        uint64_t m = static_cast<uint64_t>(T) * p_inv;
        
        // t = (T + m * p) / R
        __uint128_t t = (T + static_cast<__uint128_t>(m) * P) >> 64;
        
        // 条件减法
        return static_cast<uint64_t>(t >= P ? t - P : t);
    }

    // 蒙哥马利乘法: āb̄ -> abR mod p
    static uint64_t montgomery_mul(uint64_t a, uint64_t b) {
        __uint128_t T = static_cast<__uint128_t>(a) * b;
        return montgomery_redc(T);
    }
};

// ==================== 常用素数预定义 ====================

// 常用的32位素数
using GFP_2147483647 = GFP_32<2147483647U>;   // 2^31 - 1 (Mersenne prime)
using GFP_4294967291 = GFP_32<4294967291U>;   // 2^32 - 5 (接近 2^32 的素数)

// 常用的61位素数（SPDZ2k 常用）
using GFP_2305843009213693951 = GFP_64<2305843009213693951ULL>;  // 2^61 - 1

// 128位安全素数（接近 2^64）
using GFP_18446744073709551557 = GFP_64<18446744073709551557ULL>; // 2^64 - 59


} // namespace md_ml

#endif // MD_ML_GFP_H