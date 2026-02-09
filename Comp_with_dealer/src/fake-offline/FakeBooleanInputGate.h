// By Kaiwen Wang
/// @file Fake offline phase for Boolean input gate

#ifndef MD_ML_FAKEBOOLEANINPUTGATE_H
#define MD_ML_FAKEBOOLEANINPUTGATE_H

#include <memory>
#include <algorithm>

#include "utils/rand.h"
#include "share/IsTinyOTShare.h"
#include "fake-offline/FakeGateTinyOT.h"

namespace md_ml {

/**
 * 生成Boolean输入门的预处理数据
 *
 * 需要生成：随机掩码 [[σx]]
 */
template <IsTinyOTShare ShrType, std::size_t N>
class FakeBooleanInputGate : public FakeGateTinyOT<ShrType, N> {
public:
    using ClearType = typename ShrType::ClearType;         // bool
    using SemiShrType = typename ShrType::SemiShrType;     // bool
    using MacType = typename ShrType::MacType;             // F_{2^S}

    FakeBooleanInputGate(FakePartyTinyOT<ShrType, N>& p_fake_party,
                         std::size_t p_input_party,
                         std::size_t p_dim_row,
                         std::size_t p_dim_col);

private:
    void doRunOffline() override;

    std::size_t input_party_;
};


template <IsTinyOTShare ShrType, std::size_t N>
FakeBooleanInputGate<ShrType, N>::
FakeBooleanInputGate(FakePartyTinyOT<ShrType, N>& p_fake_party,
                     std::size_t p_input_party,
                     std::size_t p_dim_row,
                     std::size_t p_dim_col)
    : FakeGateTinyOT<ShrType, N>(p_fake_party, p_dim_row, p_dim_col),
      input_party_(p_input_party) {}


template <IsTinyOTShare ShrType, std::size_t N>
void FakeBooleanInputGate<ShrType, N>::doRunOffline() {
    auto size = this->dim_row() * this->dim_col();

    // 收集所有的bit shares和mac shares
    std::array<std::vector<SemiShrType>, N> all_bit_shares;
    std::array<std::vector<MacType>, N> all_mac_shares;

    for (auto& vec : all_bit_shares) {
        vec.reserve(size);
    }
    for (auto& vec : all_mac_shares) {
        vec.reserve(size);
    }

    // 生成随机掩码 [[σx]]
    for (size_t idx = 0; idx < size; ++idx) {
        ClearType sigma_x = getRand<uint32_t>() & 1;
        auto auth_sigma = this->fake_party().GenerateAuthenticatedBit(sigma_x);

        // 存储明文值
        this->lambda_clear().push_back(sigma_x);

        // 收集各方的shares和MACs
        for (std::size_t party_idx = 0; party_idx < N; ++party_idx) {
            all_bit_shares[party_idx].push_back(auth_sigma.bit_shares[party_idx]);
            all_mac_shares[party_idx].push_back(auth_sigma.mac_shares[party_idx]);
        }
    }

    // 按照与FakeInputGate相同的顺序写入文件：
    // 1. 将掩码值写入input_party的文件
    for (const auto& clear_val : this->lambda_clear()) {
        this->fake_party().ithPartyFile(input_party_) << (clear_val ? 1 : 0) << '\n';
    }

    // 2. 将所有方的秘密共享写入文件
    for (std::size_t party_idx = 0; party_idx < N; ++party_idx) {
        for (const auto& bit_share : all_bit_shares[party_idx]) {
            this->fake_party().ithPartyFile(party_idx) << (bit_share ? 1 : 0) << '\n';
        }
    }

    // 3. 将所有方的MAC写入文件
    for (std::size_t party_idx = 0; party_idx < N; ++party_idx) {
        for (const auto& mac_share : all_mac_shares[party_idx]) {
            this->fake_party().ithPartyFile(party_idx) << mac_share << '\n';
        }
    }
}


} // namespace md_ml

#endif //MD_ML_FAKEBOOLEANINPUTGATE_H