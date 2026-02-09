// By Kaiwen Wang
/// @file Concept for Fp share types

#ifndef MD_ML_ISFPSHARE_H
#define MD_ML_ISFPSHARE_H

#include <concepts>

namespace md_ml {

// Concept to check if a type is a Fp share
template<typename T>
concept IsFpShare = requires {
    typename T::ClearType;
    typename T::SemiShrType;
    typename T::MacType;
    typename T::GlobalKeyType;
    { T::kBits } -> std::convertible_to<std::size_t>;
    { T::sBits } -> std::convertible_to<std::size_t>;
    // 对于 Fp 份额，ClearType 和 SemiShrType 应该相同
    requires std::same_as<typename T::ClearType, typename T::SemiShrType>;
    // MAC 类型应该与明文类型相同（都在 Fp 上）
    requires std::same_as<typename T::MacType, typename T::ClearType>;
    // 全局密钥类型应该与明文类型相同
    requires std::same_as<typename T::GlobalKeyType, typename T::ClearType>;
};

// Helper concept: 检查是否为 GFP 类型
template<typename T>
concept IsGFPType = requires(T a, T b) {
    { a + b } -> std::same_as<T>;
    { a - b } -> std::same_as<T>;
    { a * b } -> std::same_as<T>;
    { a / b } -> std::same_as<T>;
    { a == b } -> std::convertible_to<bool>;
    { a.to_normal() };
    { T::zero() } -> std::same_as<T>;
    { T::one() } -> std::same_as<T>;
    { T::MODULUS };
};

} // namespace md_ml

#endif //MD_ML_ISFPSHARE_H