// By Kaiwen Wang
/// @file TinyOT authenticated Boolean share for malicious security
/// 使用正确的伽罗瓦域 GF(2^S) 实现 MAC

#ifndef MD_ML_TINYOTSHARE_H
#define MD_ML_TINYOTSHARE_H

#include <cstddef>
#include <vector>
#include <algorithm>
#include <execution>

#include "utils/GF2N.h"

namespace md_ml {

// TinyOT份额用于布尔电路（F_2域上的计算）
// 份额形式：<x> = (δx, [[σx]])
// 其中：
//   - δx: 打开的掩码值 δx = x ⊕ σx，公开值（所有方相同）
//   - [[σx]]: 掩码的认证份额 = ([σx], [MAC·σx])
//     - [σx]: 掩码的XOR秘密共享（在F_2上）
//     - [MAC·σx]: MAC份额（在GF(2^S)上）

/// TinyOT 份额模板类
/// @tparam S MAC安全参数（32或64）
template <std::size_t S>
class TinyOTShare {
    static_assert(S == 32 || S == 64, "S must be 32 or 64");

public:
    using ClearType = bool;                      // 明文类型：布尔值（F_2）
    using SemiShrType = bool;                    // 掩码份额：布尔值（XOR秘密共享）

    // 整数类型：用于表示完整的无符号整数（如比较协议中的 R）
    using IntegerType = typename std::conditional<S == 32, uint32_t, uint64_t>::type;
    
    // MAC类型：根据S选择GF(2^32)或GF(2^64)
    using MacType = typename std::conditional<S == 32, GF2_32, GF2_64>::type;
    using GlobalKeyType = MacType;               // 全局MAC密钥：GF(2^S)

    constexpr static std::size_t kBits = 1;      // 明文比特数
    constexpr static std::size_t sBits = S;      // MAC安全参数

    // RemoveUpperBits is not needed for boolean shares (always 1 bit)
    static SemiShrType RemoveUpperBits(SemiShrType value) {
        return value;
    }

    static std::vector<SemiShrType> RemoveUpperBits(const std::vector<SemiShrType>& values) {
        return values;
    }

    static void RemoveUpperBitsInplace(std::vector<SemiShrType>& values) {
        // No-op for boolean values
    }
};

using TinyOTShare64 = TinyOTShare<64>;
using TinyOTShare32 = TinyOTShare<32>;

} // namespace md_ml

#endif //MD_ML_TINYOTSHARE_H