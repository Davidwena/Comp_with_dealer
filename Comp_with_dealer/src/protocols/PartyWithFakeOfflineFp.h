// By Kaiwen Wang

#ifndef MD_ML_PartyWithFakeOfflineFp_H
#define MD_ML_PartyWithFakeOfflineFp_H

#include <string>
#include <fstream>
#include <filesystem>

#include "share/IsFpShare.h"
#include "networking/Party.h"
#include "share/EmptyType.h"

namespace md_ml {


template <IsFpShare ShrType, bool UseMac = true>
class PartyWithFakeOfflineFp : public Party {
public:
    using ClearType = typename ShrType::ClearType;
    using MacType = typename ShrType::MacType;
    using GlobalKeyType = typename ShrType::GlobalKeyType;
    using SemiShrType = typename ShrType::SemiShrType;

    static constexpr bool kUseMac = UseMac;

    PartyWithFakeOfflineFp(std::size_t p_my_id, std::size_t p_num_parties, std::size_t p_port,
                           const std::string& job_name);

    std::vector<SemiShrType> ReadShares(std::size_t num_elements);
    std::vector<ClearType> ReadClear(std::size_t num_elements);

    [[nodiscard]] std::ifstream& input_file() { return input_file_; }
    
    // 只在 UseMac=true 时提供访问
    template<bool U = UseMac, typename = std::enable_if_t<U>>
    [[nodiscard]] GlobalKeyType global_key_shr() const { return global_key_shr_; }

private:
    inline static const std::filesystem::path kFakeOfflineDir{FAKE_OFFLINE_DIR};
    
    // 条件成员：只在 UseMac=true 时存在
    std::conditional_t<UseMac, GlobalKeyType, EmptyType> global_key_shr_;
    
    std::ifstream input_file_;
};


template <IsFpShare ShrType, bool UseMac>
PartyWithFakeOfflineFp<ShrType, UseMac>::
PartyWithFakeOfflineFp(std::size_t p_my_id, std::size_t p_num_parties, std::size_t p_port, const std::string& job_name)
    : Party(p_my_id, p_num_parties, p_port) {
    
    // Open the file for input
    std::string file_name = job_name + (job_name.empty() ? "party-" : "-party-") + std::to_string(p_my_id) + ".txt";
    input_file_.open(kFakeOfflineDir / file_name);

    // Read the MAC key only if malicious security
    if constexpr (UseMac) {
        input_file_ >> global_key_shr_;
    }
}


template <IsFpShare ShrType, bool UseMac>
std::vector<typename PartyWithFakeOfflineFp<ShrType, UseMac>::SemiShrType> 
PartyWithFakeOfflineFp<ShrType, UseMac>::ReadShares(std::size_t num_elements) {
    auto shares = std::vector<SemiShrType>(num_elements);
    for (auto& share : shares) {
        input_file_ >> share;
    }
    return shares;
}

template <IsFpShare ShrType, bool UseMac>
std::vector<typename PartyWithFakeOfflineFp<ShrType, UseMac>::ClearType> 
PartyWithFakeOfflineFp<ShrType, UseMac>::ReadClear(std::size_t num_elements) {
    auto clear = std::vector<ClearType>(num_elements);
    for (auto& c : clear) {
        input_file_ >> c;
    }
    return clear;
}

} // namespace md_ml

#endif //MD_ML_PartyWithFakeOfflineFp_H