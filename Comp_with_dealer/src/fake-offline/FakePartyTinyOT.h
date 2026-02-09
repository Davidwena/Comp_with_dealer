// By Kaiwen Wang
/// @file Fake party that generates all preprocessing data for TinyOT

#ifndef MD_ML_FAKEPARTYTINYOT_H
#define MD_ML_FAKEPARTYTINYOT_H

#include <fstream>
#include <filesystem>
#include <string>
#include <array>
#include <algorithm>
#include <cstddef>

#include "share/IsTinyOTShare.h"
#include "utils/rand.h"
#include "utils/uint128_io.h"
#include "utils/GF2N.h"

namespace md_ml {

/**
 * FakePartyTinyOT generates all preprocessing data for all parties for TinyOT protocol.
 *
 * @tparam ShrType The type of the shares (should be TinyOTShare<S>)
 * @tparam N The number of parties
 */
template <IsTinyOTShare ShrType, std::size_t N>
class FakePartyTinyOT {
public:
    using ClearType = typename ShrType::ClearType;         // bool
    using SemiShrType = typename ShrType::SemiShrType;     // bool
    using MacType = typename ShrType::MacType;             // F_{2^S}
    using GlobalKeyType = typename ShrType::GlobalKeyType; // F_{2^S}

    /// 认证比特结构：[[x]] = ([x], [MAC·x])
    struct AuthenticatedBit {
        std::array<SemiShrType, N> bit_shares;    // XOR秘密共享
        std::array<MacType, N> mac_shares;        // MAC份额（加性秘密共享）
    };

    explicit FakePartyTinyOT(const std::string& job_name = std::string());

    auto constexpr static NParties() noexcept { return N; }

    [[nodiscard]] auto& ithPartyFile(std::size_t i) { return output_files_.at(i); }

    [[nodiscard]] GlobalKeyType global_mac_key() const { return global_mac_key_; }

    // 生成单个认证比特
    AuthenticatedBit GenerateAuthenticatedBit(ClearType bit_value);

private:
    inline static const std::filesystem::path kFakeOfflineDir{FAKE_OFFLINE_DIR};
    GlobalKeyType global_mac_key_;              // 全局MAC密钥
    std::array<MacType, N> mac_key_shares_;     // MAC密钥的份额
    std::array<std::ofstream, N> output_files_;
};


template <IsTinyOTShare ShrType, std::size_t N>
FakePartyTinyOT<ShrType, N>::FakePartyTinyOT(const std::string& job_name) {
    // 创建输出目录
    if (!std::filesystem::exists(kFakeOfflineDir)) {
        std::filesystem::create_directory(kFakeOfflineDir);
    }

    // 打开各方的输出文件
    const std::string file_name_suffix = job_name + (job_name.empty() ? "party-" : "-party-");
    for (std::size_t i = 0; i < N; ++i) {
        std::string current_file_name = file_name_suffix + std::to_string(i) + ".txt";
        output_files_[i].open(kFakeOfflineDir / current_file_name);
    }

    // 生成全局MAC密钥（在GF(2^S)上）
    global_mac_key_ = MacType::zero();
    for (std::size_t i = 0; i < N; ++i) {
        // 生成随机的GF元素
        if constexpr (std::is_same_v<MacType, GF2_32>) {
            mac_key_shares_[i] = MacType(getRand<uint32_t>());
        } else if constexpr (std::is_same_v<MacType, GF2_64>) {
            mac_key_shares_[i] = MacType(getRand<uint64_t>());
        }
        global_mac_key_ += mac_key_shares_[i];  // GF上的加法（XOR）
    }

    // 将MAC密钥份额写入各方文件
    for (std::size_t i = 0; i < N; ++i) {
        output_files_[i] << mac_key_shares_[i] << '\n';
    }
}


template <IsTinyOTShare ShrType, std::size_t N>
typename FakePartyTinyOT<ShrType, N>::AuthenticatedBit
FakePartyTinyOT<ShrType, N>::GenerateAuthenticatedBit(ClearType bit_value) {
    AuthenticatedBit auth_bit;

    // 1. 生成XOR秘密共享：bit_0 ⊕ bit_1 ⊕ ... ⊕ bit_{N-1} = bit_value
    SemiShrType accumulated = bit_value;
    for (std::size_t i = 0; i < N - 1; ++i) {
        auth_bit.bit_shares[i] = getRand<uint32_t>() & 1;  // 随机比特
        accumulated ^= auth_bit.bit_shares[i];
    }
    auth_bit.bit_shares[N - 1] = accumulated;  // 最后一个份额

    // 2. 生成MAC：MAC = bit_value × global_key（在GF(2^S)上）
    // 如果bit_value = 0，则MAC = 0；如果bit_value = 1，则MAC = global_key
    MacType mac = global_mac_key_ * bit_value;  // GF上的乘法

    // 3. MAC的加性秘密共享（在GF(2^S)上，实际是XOR）
    MacType mac_accumulated = mac;
    for (std::size_t i = 0; i < N - 1; ++i) {
        // 生成随机的GF元素
        if constexpr (std::is_same_v<MacType, GF2_32>) {
            auth_bit.mac_shares[i] = MacType(getRand<uint32_t>());
        } else if constexpr (std::is_same_v<MacType, GF2_64>) {
            auth_bit.mac_shares[i] = MacType(getRand<uint64_t>());
        }
        mac_accumulated -= auth_bit.mac_shares[i];  // GF上的减法（等于加法，即XOR）
    }
    auth_bit.mac_shares[N - 1] = mac_accumulated;

    return auth_bit;
}


} // namespace md_ml

#endif //MD_ML_FAKEPARTYTINYOT_H