// By Kaiwen Wang
/// @file Boolean input gate for TinyOT

#ifndef MD_ML_BOOLEANINPUTGATE_H
#define MD_ML_BOOLEANINPUTGATE_H

#include <memory>
#include <vector>
#include <stdexcept>

#include "protocols/GateTinyOT.h"
#include "share/IsTinyOTShare.h"

namespace md_ml {

/**
 * Boolean input gate using TinyOT.
 *
 * 输入门允许一方输入布尔值，并生成TinyOT认证份额。
 * 输出：<x> = (δx, [[σx]])
 */
template <IsTinyOTShare ShrType>
class BooleanInputGate : public GateTinyOT<ShrType> {
public:
    using ClearType = typename ShrType::ClearType;         // bool
    using SemiShrType = typename ShrType::SemiShrType;     // bool
    using MacType = typename ShrType::MacType;             // F_{2^S}

    BooleanInputGate(PartyWithFakeOfflineTinyOT<ShrType>& p_party,
                     std::size_t p_input_party,
                     std::size_t p_dim_row,
                     std::size_t p_dim_col);

    void setInput(const std::vector<ClearType>& p_input);

private:
    void doReadOfflineFromFile() override;
    void doRunOnline() override;

    std::size_t input_party_;
    std::vector<ClearType> input_;
    std::vector<SemiShrType> lambda_clear_;
};


template <IsTinyOTShare ShrType>
BooleanInputGate<ShrType>::
BooleanInputGate(PartyWithFakeOfflineTinyOT<ShrType>& p_party,
                 std::size_t p_input_party,
                 std::size_t p_dim_row,
                 std::size_t p_dim_col)
    : GateTinyOT<ShrType>(p_party, p_dim_row, p_dim_col),
      input_party_(p_input_party) {}


template <IsTinyOTShare ShrType>
void BooleanInputGate<ShrType>::setInput(const std::vector<ClearType>& p_input) {
    if (p_input.size() != this->dim_row() * this->dim_col()) {
        throw std::invalid_argument("Input size does not match gate dimensions");
    }
    input_ = p_input;
}


template <IsTinyOTShare ShrType>
void BooleanInputGate<ShrType>::doReadOfflineFromFile() {
    auto size = this->dim_row() * this->dim_col();
    if (this->party().my_id() == input_party_) {
        this->lambda_clear_ = this->party().ReadBoolShares(size);
    }
    // 读取随机掩码 [[σx]]
    this->lambda_shr() = this->party().ReadBoolShares(size);
    this->lambda_shr_mac() = this->party().ReadMacShares(size);
}


template <IsTinyOTShare ShrType>
void BooleanInputGate<ShrType>::doRunOnline() {
    auto size = this->dim_row() * this->dim_col();

    this->Delta_clear().resize(size);
    if (this->my_id() == input_party_) {
        // Input party: 计算 δx = x ⊕ λx，然后发送给其他方
        for (size_t i = 0; i < size; ++i) {
            this->Delta_clear()[i] = input_[i] ^ lambda_clear_[i];
        }
        // 打包比特并发送
        int msgBytes = (size + 7) / 8;
        std::vector<uint8_t> sendmsg(msgBytes, 0);
        for (size_t i = 0; i < size; ++i) {
            if (this->Delta_clear()[i]) {
                sendmsg[i / 8] |= (1 << (i % 8));
            }
        }
        this->party().SendVecToOther(sendmsg);
    }
    else {
        // 其他方: 接收 δx
        int msgBytes = (size + 7) / 8;
        auto rcvmsg = this->party().template ReceiveVecFromOther<uint8_t>(msgBytes);
        // 解包比特
        for (size_t i = 0; i < size; ++i) {
            this->Delta_clear()[i] = (rcvmsg[i / 8] >> (i % 8)) & 1;
        }
    // 现在所有方都有相同的 δx（打开的掩码值）
    // 份额 [[σx]] 存储在 lambda_shr() 和 lambda_shr_mac()
    }


    } // namespace md_ml

}
#endif //MD_ML_BOOLEANINPUTGATE_H