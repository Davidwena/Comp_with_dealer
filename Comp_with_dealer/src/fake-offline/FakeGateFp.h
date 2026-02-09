// By Kaiwen Wang

#ifndef MD_ML_FAKEGATEFP_H
#define MD_ML_FAKEGATEFP_H

#include <memory>
#include <vector>
#include <array>
#include <cstddef>

#include "share/IsFpShare.h"
#include "fake-offline/FakePartyFp.h"

namespace md_ml {

template <IsFpShare ShrType, std::size_t N, bool UseMac = true>
class FakeGateFp {
public:
    using ClearType = typename ShrType::ClearType;
    using SemiShrType = typename ShrType::SemiShrType;
    using MacType = typename ShrType::MacType;

    static constexpr bool kUseMac = UseMac;

    FakeGateFp(FakePartyFp<ShrType, N, UseMac>& p_fake_party, std::size_t p_dim_row, std::size_t p_dim_col)
        : fake_party_(p_fake_party), dim_row_(p_dim_row), dim_col_(p_dim_col) {}

    FakeGateFp(const std::shared_ptr<FakeGateFp>& p_input_x,
               const std::shared_ptr<FakeGateFp>& p_input_y)
        : fake_party_(p_input_x->fake_party_), input_x_(p_input_x), input_y_(p_input_y) {}

    FakeGateFp(const std::vector<std::shared_ptr<FakeGateFp>>& p_inputs);

    virtual ~FakeGateFp() = default;

    void runOffline();

    [[nodiscard]] const auto& input_x() const { return input_x_; }
    [[nodiscard]] const auto& input_y() const { return input_y_; }
    [[nodiscard]] const auto& inputs() const { return inputs_; }
    [[nodiscard]] auto dim_row() const { return dim_row_; }
    [[nodiscard]] auto dim_col() const { return dim_col_; }
    [[nodiscard]] auto& fake_party() { return fake_party_; }
    [[nodiscard]] const auto& fake_party() const { return fake_party_; }
    [[nodiscard]] auto& lambda_clear() { return lambda_clear_; }
    [[nodiscard]] const auto& lambda_clear() const { return lambda_clear_; }
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
    FakePartyFp<ShrType, N, UseMac>& fake_party_;
    
    std::shared_ptr<FakeGateFp> input_x_{};
    std::shared_ptr<FakeGateFp> input_y_{};
    std::vector<std::shared_ptr<FakeGateFp>> inputs_{};
    
    std::size_t dim_row_ = 1;
    std::size_t dim_col_ = 1;

    std::vector<ClearType> lambda_clear_;
    std::array<std::vector<SemiShrType>, N> lambda_shr_;
    
    // MAC shares only used when UseMac = true
    [[no_unique_address]] std::conditional_t<UseMac, std::array<std::vector<MacType>, N>, std::monostate> lambda_shr_mac_;
};

template <IsFpShare ShrType, std::size_t N, bool UseMac>
FakeGateFp<ShrType, N, UseMac>::FakeGateFp(const std::vector<std::shared_ptr<FakeGateFp>>& p_inputs)
    : fake_party_(p_inputs[0]->fake_party()), inputs_(p_inputs) {
    if (p_inputs.empty()) {
        throw std::runtime_error("FakeGateFp: inputs vector cannot be empty");
    }
}


template <IsFpShare ShrType, std::size_t N, bool UseMac>
void FakeGateFp<ShrType, N, UseMac>::runOffline() {
    if (this->evaluated_offline_)
        return;

    if (input_x_ && !input_x_->evaluated_offline_)
        input_x_->runOffline();
    if (input_y_ && !input_y_->evaluated_offline_)
        input_y_->runOffline();

    if (!inputs_.empty()) {
        for (auto& input : inputs_) {
            if (input && !input->evaluated_offline_) {
                input->runOffline();
            }
        }
    }

    this->doRunOffline();
    this->evaluated_offline_ = true;
}

} // namespace md_ml

#endif //MD_ML_FAKEGATEFP_H