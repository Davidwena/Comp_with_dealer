// By Kaiwen Wang
/// @file Concept for TinyOT share types

#ifndef MD_ML_ISTINYOTSHARE_H
#define MD_ML_ISTINYOTSHARE_H

#include <concepts>

namespace md_ml {

// Concept to check if a type is a TinyOT share
template<typename T>
concept IsTinyOTShare = requires {
    typename T::ClearType;
    typename T::SemiShrType;
    typename T::MacType;
    typename T::GlobalKeyType;
    { T::kBits } -> std::convertible_to<std::size_t>;
    { T::sBits } -> std::convertible_to<std::size_t>;
    requires std::same_as<typename T::ClearType, bool>;
    requires std::same_as<typename T::SemiShrType, bool>;
};

} // namespace md_ml

#endif //MD_ML_ISTINYOTSHARE_H