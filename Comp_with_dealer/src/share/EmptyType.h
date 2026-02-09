// By Kaiwen Wang
/// @file EmptyType.h
/// @brief Empty type for conditional compilation

#ifndef MD_ML_EMPTY_TYPE_H
#define MD_ML_EMPTY_TYPE_H

namespace md_ml {

/// Empty type used as placeholder in std::conditional_t
/// when a type is not needed (e.g., MAC shares in semi-honest security)
struct EmptyType {};

} // namespace md_ml

#endif // MD_ML_EMPTY_TYPE_H