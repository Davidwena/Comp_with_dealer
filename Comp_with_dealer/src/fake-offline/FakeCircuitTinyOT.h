// By Kaiwen Wang
/// @file Fake circuit for TinyOT Boolean operations

#ifndef MD_ML_FAKECIRCUITTINYOT_H
#define MD_ML_FAKECIRCUITTINYOT_H

#include <memory>
#include <vector>
#include <cstddef>

#include "share/IsTinyOTShare.h"
#include "fake-offline/FakePartyTinyOT.h"
#include "fake-offline/FakeGateTinyOT.h"
#include "fake-offline/FakeBooleanInputGate.h"
#include "fake-offline/FakeBooleanAndGateBatch.h"
#include "fake-offline/FakeBooleanOutputGate.h"
#include "fake-offline/FakeBooleanAndGateK.h"
#include "fake-offline/FakeLTBitsGate.h"

namespace md_ml {

/**
 * FakeCircuitTinyOT: 用于TinyOT布尔电路的预处理电路类
 *
 * 功能：
 * - 管理TinyOT fake门的创建
 * - 生成预处理数据（认证三元组、掩码等）
 * - 提供与CircuitTinyOT对应的工厂方法
 */
template <IsTinyOTShare ShrType, std::size_t N>
class FakeCircuitTinyOT {
public:
    using ClearType = typename ShrType::ClearType;  // bool
    using IntegerType = typename ShrType::IntegerType;

    explicit FakeCircuitTinyOT(FakePartyTinyOT<ShrType, N>& p_fake_party)
        : fake_party_(p_fake_party) {}

    void runOffline();
    void addEndpoint(const std::shared_ptr<FakeGateTinyOT<ShrType, N>>& gate);

    std::shared_ptr<FakeBooleanInputGate<ShrType, N>>
    input(std::size_t owner_id, std::size_t dim_row, std::size_t dim_col);

    std::shared_ptr<FakeBooleanAndGateBatch<ShrType, N>>
    booleanAndBatch(const std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>>& input_x_vec,
                    const std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>>& input_y_vec);

    //k = inputs.size()
    std::shared_ptr<FakeBooleanAndGateK<ShrType, N>>
    booleanAndK(const std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>>& inputs);

    std::shared_ptr<FakeBooleanOutputGate<ShrType, N>>
    output(const std::shared_ptr<FakeGateTinyOT<ShrType, N>>& input);

    std::shared_ptr<FakeLTBitsGate<ShrType, N>>
    ltbits(const std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>>& inputs,
            std::size_t max_branch);

    [[nodiscard]] auto& endpoints() { return endpoints_; }

private:
    FakePartyTinyOT<ShrType, N>& fake_party_;
    std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>> gates_;
    std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>> endpoints_;
};


template <IsTinyOTShare ShrType, std::size_t N>
void FakeCircuitTinyOT<ShrType, N>::runOffline() {
    for (const auto& gate : endpoints_) {
        gate->RunOffline();
    }
}


template <IsTinyOTShare ShrType, std::size_t N>
void FakeCircuitTinyOT<ShrType, N>::addEndpoint(const std::shared_ptr<FakeGateTinyOT<ShrType, N>>& gate) {
    endpoints_.push_back(gate);
}


template <IsTinyOTShare ShrType, std::size_t N>
std::shared_ptr<FakeBooleanInputGate<ShrType, N>> FakeCircuitTinyOT<ShrType, N>::
input(std::size_t owner_id, std::size_t dim_row, std::size_t dim_col) {
    auto gate = std::make_shared<FakeBooleanInputGate<ShrType, N>>(fake_party_, owner_id, dim_row, dim_col);
    gates_.push_back(gate);
    return gate;
}


template <IsTinyOTShare ShrType, std::size_t N>
std::shared_ptr<FakeBooleanAndGateBatch<ShrType, N>> FakeCircuitTinyOT<ShrType, N>::
booleanAndBatch(const std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>>& input_x_vec,
                const std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>>& input_y_vec) {
    auto gate = std::make_shared<FakeBooleanAndGateBatch<ShrType, N>>(input_x_vec, input_y_vec);
    gates_.push_back(gate);
    return gate;
}

template <IsTinyOTShare ShrType, std::size_t N>
std::shared_ptr<FakeBooleanAndGateK<ShrType, N>> FakeCircuitTinyOT<ShrType, N>::
booleanAndK(const std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>>& inputs) {
    auto gate = std::make_shared<FakeBooleanAndGateK<ShrType, N>>(inputs);
    gates_.push_back(gate);
    return gate;
}


template <IsTinyOTShare ShrType, std::size_t N>
std::shared_ptr<FakeBooleanOutputGate<ShrType, N>> FakeCircuitTinyOT<ShrType, N>::
output(const std::shared_ptr<FakeGateTinyOT<ShrType, N>>& input) {
    auto gate = std::make_shared<FakeBooleanOutputGate<ShrType, N>>(input);
    gates_.push_back(gate);
    return gate;
}

template <IsTinyOTShare ShrType, std::size_t N>
std::shared_ptr<FakeLTBitsGate<ShrType, N>> FakeCircuitTinyOT<ShrType, N>::
ltbits(const std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>>& inputs,
        std::size_t max_branch) {
    auto gate = std::make_shared<FakeLTBitsGate<ShrType, N>>(inputs, max_branch);
    gates_.push_back(gate);
    return gate;
}


} // namespace md_ml

#endif //MD_ML_FAKECIRCUITTINYOT_H