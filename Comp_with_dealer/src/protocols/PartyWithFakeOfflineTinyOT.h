// By Kaiwen Wang
/// @file Party with fake offline support for TinyOT shares

#ifndef MD_ML_PARTYWITHFAKEOFFLINETINYOT_H
#define MD_ML_PARTYWITHFAKEOFFLINETINYOT_H

#include <string>
#include <fstream>
#include <filesystem>

#include "share/IsTinyOTShare.h"
#include "networking/Party.h"
#include "utils/uint128_io.h"

namespace md_ml {

template <IsTinyOTShare ShrType>
class PartyWithFakeOfflineTinyOT : public Party {
public:
    using ClearType = typename ShrType::ClearType;         // bool
    using SemiShrType = typename ShrType::SemiShrType;     // bool
    using MacType = typename ShrType::MacType;             // F_{2^S}
    using GlobalKeyType = typename ShrType::GlobalKeyType; // F_{2^S}

    PartyWithFakeOfflineTinyOT(std::size_t p_my_id, std::size_t p_num_parties,
                               std::size_t p_port, const std::string& job_name);

    // 读取布尔明文值（用于input gate的owner）
    std::vector<ClearType> ReadBoolClear(std::size_t num_elements);

    // 读取布尔份额（比特）
    std::vector<SemiShrType> ReadBoolShares(std::size_t num_elements);

    // 读取MAC份额
    std::vector<MacType> ReadMacShares(std::size_t num_elements);

    // 读取单个布尔份额
    void ReadBool(SemiShrType& value);

    // 读取单个MAC份额
    void ReadMac(MacType& value);

    [[nodiscard]] std::ifstream& input_file() { return input_file_; }

    [[nodiscard]] GlobalKeyType global_mac_key_shr() const { return global_mac_key_shr_; }

private:
    inline static const std::filesystem::path kFakeOfflineDir{FAKE_OFFLINE_DIR};
    GlobalKeyType global_mac_key_shr_;  // 全局MAC密钥的份额
    std::ifstream input_file_;
};


template <IsTinyOTShare ShrType>
PartyWithFakeOfflineTinyOT<ShrType>::
PartyWithFakeOfflineTinyOT(std::size_t p_my_id, std::size_t p_num_parties,
                           std::size_t p_port, const std::string& job_name)
    : Party(p_my_id, p_num_parties, p_port) {
    // 打开预处理数据文件
    std::string file_name = job_name + (job_name.empty() ? "party-" : "-party-") +
                           std::to_string(p_my_id) + ".txt";
    input_file_.open(kFakeOfflineDir / file_name);

    if (!input_file_.is_open()) {
        throw std::runtime_error("Failed to open offline file: " + file_name);
    }

    // 读取全局MAC密钥的份额
    input_file_ >> global_mac_key_shr_;
}


template <IsTinyOTShare ShrType>
std::vector<typename PartyWithFakeOfflineTinyOT<ShrType>::ClearType>
PartyWithFakeOfflineTinyOT<ShrType>::ReadBoolClear(std::size_t num_elements) {
    std::vector<ClearType> clear_values(num_elements);
    for (std::size_t i = 0; i < num_elements; ++i) {
        int bit_value;
        input_file_ >> bit_value;
        clear_values[i] = (bit_value != 0);
    }
    return clear_values;
}


template <IsTinyOTShare ShrType>
std::vector<typename PartyWithFakeOfflineTinyOT<ShrType>::SemiShrType>
PartyWithFakeOfflineTinyOT<ShrType>::ReadBoolShares(std::size_t num_elements) {
    std::vector<SemiShrType> shares(num_elements);
    for (std::size_t i = 0; i < num_elements; ++i) {
        int bit_value;
        input_file_ >> bit_value;
        shares[i] = (bit_value != 0);
    }
    return shares;
}


template <IsTinyOTShare ShrType>
std::vector<typename PartyWithFakeOfflineTinyOT<ShrType>::MacType>
PartyWithFakeOfflineTinyOT<ShrType>::ReadMacShares(std::size_t num_elements) {
    std::vector<MacType> mac_shares(num_elements);
    for (auto& mac : mac_shares) {
        input_file_ >> mac;
    }
    return mac_shares;
}


template <IsTinyOTShare ShrType>
void PartyWithFakeOfflineTinyOT<ShrType>::ReadBool(SemiShrType& value) {
    int bit_value;
    input_file_ >> bit_value;
    value = (bit_value != 0);
}


template <IsTinyOTShare ShrType>
void PartyWithFakeOfflineTinyOT<ShrType>::ReadMac(MacType& value) {
    input_file_ >> value;
}


} // namespace md_ml

#endif //MD_ML_PARTYWITHFAKEOFFLINETINYOT_H