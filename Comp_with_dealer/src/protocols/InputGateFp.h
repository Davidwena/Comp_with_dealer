// By Kaiwen Wang

#ifndef INPUTGATEFP_H
#define INPUTGATEFP_H

#include <memory>
#include <vector>

#include "protocols/GateFp.h"
#include "share/IsFpShare.h"
#include "utils/linear_algebra_fp.h"

namespace md_ml {

template <IsFpShare ShrType, bool UseMac = true>
class InputGateFp : public GateFp<ShrType, UseMac> {
public:
    using ClearType = typename ShrType::ClearType;
    using SemiShrType = typename ShrType::SemiShrType;

    InputGateFp(PartyWithFakeOfflineFp<ShrType, UseMac>& p_party,
                std::size_t p_dim_row, std::size_t p_dim_col,
                std::size_t p_owner_id);

    void setInput(const std::vector<ClearType>& input_value);

private:
    void doReadOfflineFromFile() override;
    void doRunOnline() override;

    std::size_t owner_id_;
    std::vector<SemiShrType> lambda_clear_;
    std::vector<SemiShrType> input_value_;
};


template <IsFpShare ShrType, bool UseMac>
InputGateFp<ShrType, UseMac>::InputGateFp(PartyWithFakeOfflineFp<ShrType, UseMac>& p_party,
                                          std::size_t p_dim_row, std::size_t p_dim_col,
                                          std::size_t p_owner_id)
    : GateFp<ShrType, UseMac>(p_party, p_dim_row, p_dim_col), owner_id_(p_owner_id) {}


template <IsFpShare ShrType, bool UseMac>
void InputGateFp<ShrType, UseMac>::setInput(const std::vector<ClearType>& input_value) {
    if (this->party().my_id() != owner_id_)
        throw std::logic_error("Not the owner of input gate, cannot set input");
    if (input_value.size() != this->dim_row() * this->dim_col())
        throw std::invalid_argument("Input vector and gate doesn't match in size");

    input_value_ = std::vector<SemiShrType>(input_value.begin(), input_value.end());
}


template <IsFpShare ShrType, bool UseMac>
void InputGateFp<ShrType, UseMac>::doReadOfflineFromFile() {
    auto size = this->dim_row() * this->dim_col();

    if (this->party().my_id() == owner_id_) {
        this->lambda_clear_ = this->party().ReadShares(size);
    }

    this->lambda_shr() = this->party().ReadShares(size);
    
    // 只在恶意安全时读取 MAC shares
    if constexpr (UseMac) {
        this->lambda_shr_mac() = this->party().ReadShares(size);
    }
}


template <IsFpShare ShrType, bool UseMac>
void InputGateFp<ShrType, UseMac>::doRunOnline() {
    if (this->my_id() == owner_id_) {
        this->Delta_clear() = matrixAdd(input_value_, this->lambda_clear_);
        this->party().SendVecToOther(this->Delta_clear());
    }
    else {
        auto size = this->dim_row() * this->dim_col();
        this->Delta_clear() = this->party().template ReceiveVecFromOther<SemiShrType>(size);
    }
}

} // namespace md_ml

#endif //INPUTGATEFP_H