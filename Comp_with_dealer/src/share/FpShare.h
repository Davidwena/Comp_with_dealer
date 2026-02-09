// By Kaiwen Wang
/// @file Authenticated share over prime field Fp for malicious security
/// 使用素数域 GF(p) 实现 MAC

#ifndef MD_ML_FPSHARE_H
#define MD_ML_FPSHARE_H

#include <cstddef>
#include <vector>
#include <algorithm>
#include <execution>
#include <cmath>

#include "utils/GFP.h"

namespace md_ml {

// Fp份额用于算术电路（F_p域上的计算）
// 份额形式：<x> = (δx, [[σx]])
// 其中：
//   - δx: 打开的掩码值 δx = x + σx (mod p)，公开值（所有方相同）
//   - [[σx]]: 掩码的认证份额 = ([σx], [MAC·σx])
//     - [σx]: 掩码的加性秘密共享（在F_p上）
//     - [MAC·σx]: MAC份额（在F_p上）

/// Fp 份额模板类
/// @tparam GFP_Type 素数域类型，可以是 GFP_32<P> 或 GFP_64<P>
template <typename GFP_Type>
class FpShare {
public:
    using ClearType = GFP_Type;                  // 明文类型：F_p 域元素
    using SemiShrType = GFP_Type;                // 掩码份额：F_p 域元素（加性秘密共享）
    using MacType = GFP_Type;                    // MAC类型：F_p 域元素
    using GlobalKeyType = GFP_Type;              // 全局MAC密钥：F_p 域元素

    // 整数类型：用于表示完整的无符号整数（如比较协议中的 R）
    using IntegerType = typename std::conditional<
        sizeof(decltype(GFP_Type::value)) == 4,
        uint32_t,
        uint64_t
    >::type;

private:
    // 编译期计算比特数（必须在 kBits 之前定义）
    static constexpr std::size_t compute_bits() {
        auto p = GFP_Type::MODULUS;
        std::size_t bits = 0;
        while (p > 0) {
            bits++;
            p >>= 1;
        }
        return bits;
    }

public:
    // 明文比特数：log2(p) 向上取整
    constexpr static std::size_t kBits = compute_bits();
    
    // MAC安全参数：与明文相同的比特数
    constexpr static std::size_t sBits = kBits;

    // 获取模数 p
    static constexpr auto modulus() {
        return GFP_Type::MODULUS;
    }

    // RemoveUpperBits: 确保值在 [0, p) 范围内
    // 对于 Fp 域，这个操作通常已经由 GFP_Type 内部保证
    static SemiShrType RemoveUpperBits(SemiShrType value) {
        return value;  // GFP_Type 自动处理模运算
    }

    static std::vector<SemiShrType> RemoveUpperBits(const std::vector<SemiShrType>& values) {
        return values;  // GFP_Type 自动处理模运算
    }

    static void RemoveUpperBitsInplace(std::vector<SemiShrType>& values) {
        // No-op for Fp values (already normalized by GFP_Type)
    }
};

// ==================== 常用配置 ====================

// 基于 32 位素数的份额类型

/// 使用 2^16 - 15 (接近 2^16 的素数，65521)
using FpShare16 = FpShare<GFP_32<65521U>>;

/// 使用 2^31 - 1 (Mersenne 素数，31位)
using FpShare_2147483647 = FpShare<GFP_32<2147483647U>>;  

// 基于 64 位素数的份额类型

/// 使用 2^61 - 1 (SPDZ2k 常用，61位)
using FpShare61 = FpShare<GFP_64<2305843009213693951ULL>>;  


// 推荐配置
using FpShare31 = FpShare_2147483647;  // 32位场景


} // namespace md_ml

#endif //MD_ML_FPSHARE_H