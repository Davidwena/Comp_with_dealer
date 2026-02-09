// By Kaiwen Wang

#ifndef OUTPUTGATEFP_H
#define OUTPUTGATEFP_H

#include <memory>
#include <vector>
#include <thread>
#include <utils/print_vector.h>

#include "protocols/GateFp.h"
#include "share/IsFpShare.h"
#include "utils/linear_algebra_fp.h"

namespace md_ml {

template <IsFpShare ShrType, bool UseMac = true>
class OutputGateFp : public GateFp<ShrType, UseMac> {
public:
    using ClearType = typename ShrType::ClearType;
    using SemiShrType = typename ShrType::SemiShrType;

    explicit OutputGateFp(const std::shared_ptr<GateFp<ShrType, UseMac>>& p_input_x);

    std::vector<ClearType> getClear() const;

private:
    void doReadOfflineFromFile() override;
    void doRunOnline() override;

    std::vector<SemiShrType> lambda_clear_;
    std::vector<SemiShrType> output_value_;
};


template <IsFpShare ShrType, bool UseMac>
OutputGateFp<ShrType, UseMac>::OutputGateFp(const std::shared_ptr<GateFp<ShrType, UseMac>>& p_input_x)
    : GateFp<ShrType, UseMac>(p_input_x, nullptr) {
    this->set_dim_row(p_input_x->dim_row());
    this->set_dim_col(p_input_x->dim_col());
}


template <IsFpShare ShrType, bool UseMac>
void OutputGateFp<ShrType, UseMac>::doReadOfflineFromFile() {} // Do nothing


template <IsFpShare ShrType, bool UseMac>
void OutputGateFp<ShrType, UseMac>::doRunOnline() {
    auto size = this->dim_row() * this->dim_col();

    std::thread t1([this] {
        this->party().SendVecToOther(this->input_x()->lambda_shr());
    });

    std::thread t2([this, size] {
        this->lambda_clear_ = this->party().template ReceiveVecFromOther<SemiShrType>(size);
    });

    t1.join();
    t2.join();

    matrixAddAssign(lambda_clear_, this->input_x()->lambda_shr());
    output_value_ = matrixSubtract(this->input_x()->Delta_clear(), lambda_clear_);
}


template <IsFpShare ShrType, bool UseMac>
std::vector<typename OutputGateFp<ShrType, UseMac>::ClearType> 
OutputGateFp<ShrType, UseMac>::getClear() const {
    return std::vector<ClearType>(output_value_.begin(), output_value_.end());
}

} // namespace md_ml

#endif //OUTPUTGATEFP_H