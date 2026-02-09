// By Kaiwen Wang

#ifndef MD_ML_LINEAR_ALGEBRA_FP_H
#define MD_ML_LINEAR_ALGEBRA_FP_H

#include <vector>
#include <algorithm>
#include <execution>
#include <concepts>

#include <Eigen/Core>

namespace md_ml {

// Linear algebra operations for Fp domain types (GFP_32<P>, GFP_64<P>)
// These functions mirror the interface of linear_algebra.h but work with custom GFP types
//
// Key difference from linear_algebra.h:
// - No std::integral constraint (works with any type supporting +, -, *, /)
// - Uses same optimization strategies (std::transform with parallel execution)
//
// WARNING:
// Dimensions are not checked here for efficiency.
// They should be checked in gate constructors before the online phase.

// Concept: Type must support arithmetic operations
template<typename T>
concept ArithmeticType = requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
    { a - b } -> std::convertible_to<T>;
    { a * b } -> std::convertible_to<T>;
};

template <ArithmeticType T>
inline
std::vector<T> matrixAdd(const std::vector<T>& x, const std::vector<T>& y) {
    std::vector<T> output(x.size());

#ifdef _LIBCPP_HAS_NO_INCOMPLETE_PSTL
    std::transform(x.begin(), x.end(), y.begin(), output.begin(), std::plus<T>());
#else
    std::transform(std::execution::par_unseq, x.begin(), x.end(), y.begin(), output.begin(), std::plus<T>());
#endif

    return output;
}


template <ArithmeticType T>
inline
void matrixAddAssign(std::vector<T>& x, const std::vector<T>& y) {
#ifdef _LIBCPP_HAS_NO_INCOMPLETE_PSTL
    std::transform(x.begin(), x.end(), y.begin(), x.begin(), std::plus<T>());
#else
    std::transform(std::execution::par_unseq, x.begin(), x.end(), y.begin(), x.begin(), std::plus<T>());
#endif
}


template <ArithmeticType T1, ArithmeticType T2>
inline
std::vector<T1> matrixAddConstant(const std::vector<T1>& x, T2 constant) {
    std::vector<T1> output(x.size());
#ifdef _LIBCPP_HAS_NO_INCOMPLETE_PSTL
    std::transform(x.begin(), x.end(), output.begin(),
                   [constant](const T1& val) { return val + constant; });
#else
    std::transform(std::execution::par_unseq, x.begin(), x.end(), output.begin(),
                   [constant](const T1& val) { return val + constant; });
#endif
    return output;
}


template <ArithmeticType T>
inline
std::vector<T> matrixSubtract(const std::vector<T>& x, const std::vector<T>& y) {
    std::vector<T> output(x.size());

#ifdef _LIBCPP_HAS_NO_INCOMPLETE_PSTL
    std::transform(x.begin(), x.end(), y.begin(), output.begin(), std::minus<T>());
#else
    std::transform(std::execution::par_unseq, x.begin(), x.end(), y.begin(), output.begin(), std::minus<T>());
#endif

    return output;
}


template <ArithmeticType T>
inline
void matrixSubtractAssign(std::vector<T>& x, const std::vector<T>& y) {
#ifdef _LIBCPP_HAS_NO_INCOMPLETE_PSTL
    std::transform(x.begin(), x.end(), y.begin(), x.begin(), std::minus<T>());
#else
    std::transform(std::execution::par_unseq, x.begin(), x.end(), y.begin(), x.begin(), std::minus<T>());
#endif
}


// matrix scalar product
template <ArithmeticType T>
inline
std::vector<T> matrixScalar(const std::vector<T>& x, const T& scalar) {
    std::vector<T> output(x.size());
#ifdef _LIBCPP_HAS_NO_INCOMPLETE_PSTL
    std::transform(x.begin(), x.end(), output.begin(), 
                   [&scalar](const T& val) { return scalar * val; });
#else
    std::transform(std::execution::par_unseq, x.begin(), x.end(), output.begin(),
                   [&scalar](const T& val) { return scalar * val; });
#endif
    return output;
}

template <ArithmeticType T>
inline
void matrixScalarAssign(std::vector<T>& x, const T& scalar) {
#ifdef _LIBCPP_HAS_NO_INCOMPLETE_PSTL
    std::transform(x.begin(), x.end(), x.begin(), 
                   [&scalar](const T& val) { return scalar * val; });
#else
    std::transform(std::execution::par_unseq, x.begin(), x.end(), x.begin(), 
                   [&scalar](const T& val) { return scalar * val; });
#endif
}


template <ArithmeticType T>
inline
std::vector<T> matrixElemMultiply(const std::vector<T>& x, const std::vector<T>& y) {
    std::vector<T> output(x.size());

#ifdef _LIBCPP_HAS_NO_INCOMPLETE_PSTL
    std::transform(x.begin(), x.end(), y.begin(), output.begin(), std::multiplies<T>());
#else
    std::transform(std::execution::par_unseq,
                   x.begin(), x.end(), y.begin(), output.begin(), std::multiplies<T>());
#endif

    return output;
}


// Matrix multiplication for Fp types
// Note: Eigen doesn't support custom types directly, so we implement matrix multiplication manually
template <ArithmeticType T>
inline
void matrixMultiply(const T* lhs, const T* rhs, T* output,
                    std::size_t dim_row, std::size_t dim_mid, std::size_t dim_col) {
    // Manual matrix multiplication: output[i][j] = sum_k lhs[i][k] * rhs[k][j]
    // Using row-major order storage
    
    for (std::size_t i = 0; i < dim_row; ++i) {
        for (std::size_t j = 0; j < dim_col; ++j) {
            T sum{};  // Default initialization (should be zero for GFP types)
            if constexpr (requires { T::zero(); }) {
                sum = T::zero();  // Use zero() if available
            }
            for (std::size_t k = 0; k < dim_mid; ++k) {
                sum = sum + lhs[i * dim_mid + k] * rhs[k * dim_col + j];
            }
            output[i * dim_col + j] = sum;
        }
    }
}


template <ArithmeticType T>
inline
std::vector<T> matrixMultiply(const std::vector<T>& lhs, const std::vector<T>& rhs,
                              std::size_t dim_row, std::size_t dim_mid, std::size_t dim_col) {
    std::vector<T> output(dim_row * dim_col);
    matrixMultiply(lhs.data(), rhs.data(), output.data(), dim_row, dim_mid, dim_col);
    return output;
}


} // namespace md_ml

#endif //MD_ML_LINEAR_ALGEBRA_FP_H