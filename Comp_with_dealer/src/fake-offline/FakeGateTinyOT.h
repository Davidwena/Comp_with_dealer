// By Kaiwen Wang
/// @file Base class for fake gates using TinyOT

#ifndef MD_ML_FAKEGATETINYOT_H
#define MD_ML_FAKEGATETINYOT_H

#include <vector>
#include <memory>

#include "share/IsTinyOTShare.h"
#include "fake-offline/FakePartyTinyOT.h"

namespace md_ml {

template <IsTinyOTShare ShrType, std::size_t N>
class FakeGateTinyOT {
public:
    using ClearType = typename ShrType::ClearType;         // bool
    using SemiShrType = typename ShrType::SemiShrType;     // bool
    using MacType = typename ShrType::MacType;             // F_{2^S}

    FakeGateTinyOT(FakePartyTinyOT<ShrType, N>& p_fake_party,
                   std::size_t p_dim_row, std::size_t p_dim_col);

    FakeGateTinyOT(const std::shared_ptr<FakeGateTinyOT>& p_input_x,
                   const std::shared_ptr<FakeGateTinyOT>& p_input_y);
    
    //用于处理多输入的逻辑
    FakeGateTinyOT(const std::vector<std::shared_ptr<FakeGateTinyOT>>& p_inputs);

    FakeGateTinyOT(const std::vector<std::shared_ptr<FakeGateTinyOT>>& p_inputs, 
                    FakePartyTinyOT<ShrType, N>& p_fake_party);

    virtual ~FakeGateTinyOT() = default;

    void RunOffline();

    [[nodiscard]] auto& fake_party() { return fake_party_; }

    [[nodiscard]] std::size_t dim_row() const { return dim_row_; }
    [[nodiscard]] std::size_t dim_col() const { return dim_col_; }

    [[nodiscard]] auto input_x() { return input_x_; }
    [[nodiscard]] auto input_y() { return input_y_; }
    [[nodiscard]] const auto& inputs() const { return inputs_; }

    [[nodiscard]] const std::vector<ClearType>& lambda_clear() const { return lambda_clear_; }
    [[nodiscard]] std::vector<ClearType>& lambda_clear() { return lambda_clear_; }

    [[nodiscard]] auto& lambda_shr() { return lambda_shr_; }
    [[nodiscard]] const auto& lambda_shr() const { return lambda_shr_; }

    [[nodiscard]] auto& lambda_shr_mac() { return lambda_shr_mac_; }
    [[nodiscard]] const auto& lambda_shr_mac() const { return lambda_shr_mac_; }

protected:
    void set_dim_row(std::size_t p_dim_row) { dim_row_ = p_dim_row; }
    void set_dim_col(std::size_t p_dim_col) { dim_col_ = p_dim_col; }

private:
    virtual void doRunOffline() = 0;

    bool evaluated_offline_ = false;

    FakePartyTinyOT<ShrType, N>& fake_party_;

    std::shared_ptr<FakeGateTinyOT> input_x_{};
    std::shared_ptr<FakeGateTinyOT> input_y_{};

    //处理多输入
    std::vector<std::shared_ptr<FakeGateTinyOT>> inputs_{};

    std::size_t dim_row_ = 1;
    std::size_t dim_col_ = 1;

    std::vector<ClearType> lambda_clear_;  // 掩码明文值

    // The shares of $\lambda_z$-values held by the parties
    std::array<std::vector<SemiShrType>, N> lambda_shr_;

    // The shared MAC of the $\lambda_z$-values held by the parties
    std::array<std::vector<MacType>, N> lambda_shr_mac_;
};


template <IsTinyOTShare ShrType, std::size_t N>
FakeGateTinyOT<ShrType, N>::FakeGateTinyOT(FakePartyTinyOT<ShrType, N>& p_fake_party,
                                            std::size_t p_dim_row, std::size_t p_dim_col)
    : fake_party_(p_fake_party), dim_row_(p_dim_row), dim_col_(p_dim_col) {}


template <IsTinyOTShare ShrType, std::size_t N>
FakeGateTinyOT<ShrType, N>::FakeGateTinyOT(const std::shared_ptr<FakeGateTinyOT>& p_input_x,
                                            const std::shared_ptr<FakeGateTinyOT>& p_input_y)
    : fake_party_(p_input_x->fake_party()), input_x_(p_input_x), input_y_(p_input_y) {}

template <IsTinyOTShare ShrType, std::size_t N>
FakeGateTinyOT<ShrType, N>::FakeGateTinyOT(const std::vector<std::shared_ptr<FakeGateTinyOT>>& p_inputs)
    : fake_party_(p_inputs[0]->fake_party()), inputs_(p_inputs) {
        if (p_inputs.empty()) {
            throw ::std::runtime_error("FakeGateTinyOT: inputs vector cannot be empty");
        }
    }

template <IsTinyOTShare ShrType, std::size_t N>
FakeGateTinyOT<ShrType, N>::FakeGateTinyOT(const std::vector<std::shared_ptr<FakeGateTinyOT>>& p_inputs, 
                                            FakePartyTinyOT<ShrType, N>& p_fake_party) 
    : inputs_(p_inputs), fake_party_(p_fake_party) {}


template <IsTinyOTShare ShrType, std::size_t N>
void FakeGateTinyOT<ShrType, N>::RunOffline() {
    if (this->evaluated_offline_)
        return;

    if (input_x_ && !input_x_->evaluated_offline_)
        input_x_->RunOffline();
    if (input_y_ && !input_y_->evaluated_offline_)
        input_y_->RunOffline();

    // 处理多输入门（递归调用）
    if (!inputs_.empty()) {
        for (auto& input : inputs_) {
            if (input && !input->evaluated_offline_) {
                input->RunOffline();
            }
        }
    }

    this->doRunOffline();

    this->evaluated_offline_ = true;
}


} // namespace md_ml

#endif //MD_ML_FAKEGATETINYOT_H