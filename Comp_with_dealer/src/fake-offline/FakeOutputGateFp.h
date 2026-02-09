// By Kaiwen Wang

#ifndef MD_ML_FAKEOUTPUTGATEFP_H
#define MD_ML_FAKEOUTPUTGATEFP_H

#include <algorithm>

#include "utils/rand.h"
#include "share/IsFpShare.h"
#include "fake-offline/FakeGateFp.h"

namespace md_ml {

template <IsFpShare ShrType, std::size_t N, bool UseMac = true>
class FakeOutputGateFp : public FakeGateFp<ShrType, N, UseMac> {
public:
    using ClearType = typename ShrType::ClearType;
    using SemiShrType = typename ShrType::SemiShrType;

    explicit FakeOutputGateFp(const std::shared_ptr<FakeGateFp<ShrType, N, UseMac>>& p_input_x);

private:
    void doRunOffline() override;
};


template <IsFpShare ShrType, std::size_t N, bool UseMac>
FakeOutputGateFp<ShrType, N, UseMac>::FakeOutputGateFp(const std::shared_ptr<FakeGateFp<ShrType, N, UseMac>>& p_input_x)
    : FakeGateFp<ShrType, N, UseMac>(p_input_x, nullptr) {
    this->set_dim_row(p_input_x->dim_row());
    this->set_dim_col(p_input_x->dim_col());
}

template <IsFpShare ShrType, std::size_t N, bool UseMac>
void FakeOutputGateFp<ShrType, N, UseMac>::doRunOffline() {}  // Do nothing

} // namespace md_ml

#endif //MD_ML_FAKEOUTPUTGATEFP_H