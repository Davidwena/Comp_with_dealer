// By Kaiwen Wang

#ifndef MD_ML_FAKEPARTYFP_H
#define MD_ML_FAKEPARTYFP_H

#include <fstream>
#include <filesystem>
#include <string>
#include <array>
#include <numeric>
#include <algorithm>
#include <cstddef>
#include <variant>

#include "share/IsFpShare.h"
#include "utils/rand.h"

namespace md_ml {

/// Security level configuration
/// @tparam UseMac Whether to use MAC for malicious security
/// - true: Malicious security (with MAC)
/// - false: Semi-honest security (without MAC, zero overhead)

template <IsFpShare ShrType, std::size_t N, bool UseMac = true>
class FakePartyFp {
public:
    using ClearType = typename ShrType::ClearType;
    using MacType = typename ShrType::MacType;
    using GlobalKeyType = typename ShrType::GlobalKeyType;
    using SemiShrType = typename ShrType::SemiShrType;

    static constexpr bool kUseMac = UseMac;

    struct AllPartiesShares {
        std::array<SemiShrType, N> value_shares;
        std::array<MacType, N> mac_shares;  // Only used if UseMac = true
    };

    struct AllPartiesSharesVec {
        std::array<std::vector<SemiShrType>, N> value_shares;
        std::array<std::vector<MacType>, N> mac_shares;  // Only used if UseMac = true
    };

    explicit FakePartyFp(const std::string& job_name = std::string());

    auto constexpr static NParties() noexcept { return N; }

    [[nodiscard]] auto& ithPartyFile(std::size_t i) { return output_files_.at(i); }

    AllPartiesShares GenerateAllPartiesShares(ClearType value) const;
    AllPartiesSharesVec GenerateAllPartiesShares(const std::vector<ClearType>& value) const;

    void WriteSharesToAllParites(const std::array<std::vector<SemiShrType>, N>& shares);
    
    // Overload for writing MAC shares (only used when UseMac = true)
    template<bool U = UseMac, typename = std::enable_if_t<U>>
    void WriteMacSharesToAllParties(const std::array<std::vector<MacType>, N>& macs);

    void WriteClearToIthParty(const std::vector<ClearType>& values, std::size_t party_id);
    void WriteClearToAllParties(const std::vector<ClearType>& values);

private:
    inline static const std::filesystem::path kFakeOfflineDir{FAKE_OFFLINE_DIR};
    
    // Only used if UseMac = true
    [[no_unique_address]] std::conditional_t<UseMac, GlobalKeyType, std::monostate> global_key_;
    [[no_unique_address]] std::conditional_t<UseMac, std::array<MacType, N>, std::monostate> key_shares_;
    
    std::array<std::ofstream, N> output_files_;
};


template <IsFpShare ShrType, std::size_t N, bool UseMac>
FakePartyFp<ShrType, N, UseMac>::FakePartyFp(const std::string& job_name) {
    // Open output files
    if (!exists(kFakeOfflineDir)) {
        create_directory(kFakeOfflineDir);
    }
    
    const std::string file_name_suffix = job_name + (job_name.empty() ? "party-" : "-party-");
    for (std::size_t i = 0; i < N; ++i) {
        std::string current_file_name = file_name_suffix + std::to_string(i) + ".txt";
        output_files_[i].open(kFakeOfflineDir / current_file_name);
    }

    // Generate MAC key only if malicious security
    if constexpr (UseMac) {
        global_key_ = GlobalKeyType::zero();
        for (std::size_t i = 0; i < N; ++i) {
            if constexpr (sizeof(MacType) == sizeof(uint32_t)) {
                key_shares_[i] = MacType(getRand<uint32_t>());
            } else {
                key_shares_[i] = MacType(getRand<uint64_t>());
            }
            global_key_ = global_key_ + key_shares_[i];
        }

        // Write MAC key shares
        for (std::size_t i = 0; i < N; ++i) {
            output_files_[i] << key_shares_[i] << '\n';
        }
    }
}


template <IsFpShare ShrType, std::size_t N, bool UseMac>
typename FakePartyFp<ShrType, N, UseMac>::AllPartiesShares 
FakePartyFp<ShrType, N, UseMac>::GenerateAllPartiesShares(ClearType value) const {
    AllPartiesShares all_parties_shares;

    // Generate value shares (always needed)
    auto& value_shares = all_parties_shares.value_shares;
    for (std::size_t i = 0; i < N - 1; ++i) {
        if constexpr (sizeof(SemiShrType) == sizeof(uint32_t)) {
            value_shares[i] = SemiShrType(getRand<uint32_t>());
        } else {
            value_shares[i] = SemiShrType(getRand<uint64_t>());
        }
    }
    value_shares.back() = value;
    for (std::size_t i = 0; i < N - 1; ++i) {
        value_shares.back() = value_shares.back() - value_shares[i];
    }

    // Generate MAC shares only if malicious security
    if constexpr (UseMac) {
        MacType mac = value * global_key_;
        auto& mac_shares = all_parties_shares.mac_shares;
        
        for (std::size_t i = 0; i < N - 1; ++i) {
            if constexpr (sizeof(MacType) == sizeof(uint32_t)) {
                mac_shares[i] = MacType(getRand<uint32_t>());
            } else {
                mac_shares[i] = MacType(getRand<uint64_t>());
            }
        }
        mac_shares.back() = mac;
        for (std::size_t i = 0; i < N - 1; ++i) {
            mac_shares.back() = mac_shares.back() - mac_shares[i];
        }
    }

    return all_parties_shares;
}


template <IsFpShare ShrType, std::size_t N, bool UseMac>
typename FakePartyFp<ShrType, N, UseMac>::AllPartiesSharesVec 
FakePartyFp<ShrType, N, UseMac>::GenerateAllPartiesShares(const std::vector<ClearType>& value) const {
    AllPartiesSharesVec all_parties_shares;
    auto& value_shares = all_parties_shares.value_shares;
    auto& mac_shares = all_parties_shares.mac_shares;

    auto size = value.size();

    std::ranges::for_each(value_shares, [size](auto& vec) { vec.resize(size); });
    if constexpr (UseMac) {
        std::ranges::for_each(mac_shares, [size](auto& vec) { vec.resize(size); });
    }

    for (std::size_t vec_idx = 0; vec_idx < size; ++vec_idx) {
        auto shares_i = GenerateAllPartiesShares(value[vec_idx]);

        for (std::size_t party_idx = 0; party_idx < N; ++party_idx) {
            value_shares[party_idx][vec_idx] = shares_i.value_shares[party_idx];
            if constexpr (UseMac) {
                mac_shares[party_idx][vec_idx] = shares_i.mac_shares[party_idx];
            }
        }
    }

    return all_parties_shares;
}


template <IsFpShare ShrType, std::size_t N, bool UseMac>
void FakePartyFp<ShrType, N, UseMac>::WriteSharesToAllParites(
    const std::array<std::vector<SemiShrType>, N>& shares) {
    for (std::size_t party_idx = 0; party_idx < N; ++party_idx) {
        auto& output_file = ithPartyFile(party_idx);
        std::ranges::for_each(shares[party_idx], 
            [&output_file](auto share) { output_file << share << '\n'; });
    }
}


template <IsFpShare ShrType, std::size_t N, bool UseMac>
template<bool U, typename>
void FakePartyFp<ShrType, N, UseMac>::WriteMacSharesToAllParties(
    const std::array<std::vector<MacType>, N>& macs) {
    static_assert(U == UseMac, "MAC writing only available when UseMac=true");
    
    for (std::size_t party_idx = 0; party_idx < N; ++party_idx) {
        auto& output_file = ithPartyFile(party_idx);
        std::ranges::for_each(macs[party_idx], 
            [&output_file](auto mac) { output_file << mac << '\n'; });
    }
}


template <IsFpShare ShrType, std::size_t N, bool UseMac>
void FakePartyFp<ShrType, N, UseMac>::WriteClearToIthParty(
    const std::vector<ClearType>& values, std::size_t party_id) {
    auto& output_file = ithPartyFile(party_id);
    std::ranges::for_each(values, [&output_file](auto value) { output_file << value << '\n'; });
}


template <IsFpShare ShrType, std::size_t N, bool UseMac>
void FakePartyFp<ShrType, N, UseMac>::WriteClearToAllParties(const std::vector<ClearType>& values) {
    for (std::size_t party_idx = 0; party_idx < N; ++party_idx) {
        WriteClearToIthParty(values, party_idx);
    }
}


} // namespace md_ml

#endif //MD_ML_FAKEPARTYFP_H