// By Kaiwen Wang
/// @file Boolean output gate using TinyOT

#ifndef MD_ML_BOOLEANOUTPUTGATE_H
#define MD_ML_BOOLEANOUTPUTGATE_H

#include <memory>
#include <vector>
#include <iostream>
#include <thread>

#include "protocols/GateTinyOT.h"
#include "share/IsTinyOTShare.h"

namespace md_ml {

/**
 * 布尔输出门：重构TinyOT份额为明文
 *
 * 协议：
 *   输入：<x> = (δx, [[σx]])
 *   输出：x = δx ⊕ σx （重构明文）
 *
 * 在线阶段：
 *   1. 所有方已有 δx（公开值）
 *   2. 每方本地计算：result = δx ⊕ [σx]
 *   3. （可选）打开result得到最终明文x
 */
template <IsTinyOTShare ShrType>
class BooleanOutputGate : public GateTinyOT<ShrType> {
public:
    using ClearType = typename ShrType::ClearType;  // bool
    using SemiShrType = typename ShrType::SemiShrType;

    explicit BooleanOutputGate(const std::shared_ptr<GateTinyOT<ShrType>>& p_input);

    // 获取重构的明文值
    std::vector<ClearType> getClear() const;

private:
    void doReadOfflineFromFile() override;
    void doRunOnline() override;

    std::vector<ClearType> clear_output_;  // 重构的明文结果
};


template <IsTinyOTShare ShrType>
BooleanOutputGate<ShrType>::BooleanOutputGate(const std::shared_ptr<GateTinyOT<ShrType>>& p_input)
    : GateTinyOT<ShrType>(p_input, nullptr) {
    this->set_dim_row(p_input->dim_row());
    this->set_dim_col(p_input->dim_col());
}


template <IsTinyOTShare ShrType>
void BooleanOutputGate<ShrType>::doReadOfflineFromFile() {
    // 输出门不需要预处理数据
}


template <IsTinyOTShare ShrType>
void BooleanOutputGate<ShrType>::doRunOnline() {
    std::size_t size = this->dim_row() * this->dim_col();
    clear_output_.resize(size);

    // 获取输入的δx和[σx]
    const auto& delta_x = this->input_x()->Delta_clear();
    const auto& sigma_x_shr = this->input_x()->lambda_shr();

    // 步骤1: 交换σx的份额并重构（类似SPDZ2k的OutputGate）
    std::vector<SemiShrType> sigma_x_other(size);

    std::thread t1([this, &sigma_x_shr] {
        // 打包比特并发送
        int msgBytes = (sigma_x_shr.size() + 7) / 8;
        std::vector<uint8_t> sendmsg(msgBytes, 0);
        for (size_t i = 0; i < sigma_x_shr.size(); ++i) {
            if (sigma_x_shr[i]) {
                sendmsg[i / 8] |= (1 << (i % 8));
            }
        }
        this->party().SendVecToOther(sendmsg);
    });

    std::thread t2([this, &sigma_x_other, size] {
        // 接收并解包
        int msgBytes = (size + 7) / 8;
        auto rcvmsg = this->party().template ReceiveVecFromOther<uint8_t>(msgBytes);
        for (size_t i = 0; i < size; ++i) {
            sigma_x_other[i] = (rcvmsg[i / 8] >> (i % 8)) & 1;
        }
    });

    t1.join();
    t2.join();

    // 步骤2: 重构σx = σx_0 ⊕ σx_1，然后计算x = δx ⊕ σx
    for (std::size_t i = 0; i < size; ++i) {
        SemiShrType sigma_x_reconstructed = sigma_x_shr[i] ^ sigma_x_other[i];
        clear_output_[i] = delta_x[i] ^ sigma_x_reconstructed;
    }
}


template <IsTinyOTShare ShrType>
std::vector<typename BooleanOutputGate<ShrType>::ClearType>
BooleanOutputGate<ShrType>::getClear() const {
    return clear_output_;
}


} // namespace md_ml

#endif //MD_ML_BOOLEANOUTPUTGATE_H