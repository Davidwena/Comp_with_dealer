// By Kaiwen Wang
/// @file Fake Boolean output gate (no preprocessing needed)

#ifndef MD_ML_FAKEBOOLEANOUTPUTGATE_H
#define MD_ML_FAKEBOOLEANOUTPUTGATE_H

#include <memory>
#include "fake-offline/FakeGateTinyOT.h"
#include "share/IsTinyOTShare.h"

namespace md_ml {

/**
 * 布尔输出门的预处理（实际上不需要预处理数据）
 *
 * 输出门只是重构份额为明文，不需要额外的预处理数据。
 */
template <IsTinyOTShare ShrType, std::size_t N>
class FakeBooleanOutputGate : public FakeGateTinyOT<ShrType, N> {
public:
    using ClearType = typename ShrType::ClearType;

    explicit FakeBooleanOutputGate(const std::shared_ptr<FakeGateTinyOT<ShrType, N>>& p_input);

private:
    void doRunOffline() override;
};


template <IsTinyOTShare ShrType, std::size_t N>
FakeBooleanOutputGate<ShrType, N>::FakeBooleanOutputGate(
    const std::shared_ptr<FakeGateTinyOT<ShrType, N>>& p_input)
    : FakeGateTinyOT<ShrType, N>(p_input, nullptr) {
    this->set_dim_row(p_input->dim_row());
    this->set_dim_col(p_input->dim_col());
}


template <IsTinyOTShare ShrType, std::size_t N>
void FakeBooleanOutputGate<ShrType, N>::doRunOffline() {
    // 输出门不需要生成预处理数据
    // 不向文件写入任何内容
}


} // namespace md_ml

#endif //MD_ML_FAKEBOOLEANOUTPUTGATE_H