// By Kaiwen Wang

#ifndef MD_ML_FAKECIRCUITFP_H
#define MD_ML_FAKECIRCUITFP_H

#include <memory>
#include <vector>
#include <cstddef>

#include "share/IsFpShare.h"
#include "fake-offline/FakePartyFp.h"
#include "fake-offline/FakeGateFp.h"
#include "fake-offline/FakeInputGateFp.h"
#include "fake-offline/FakeOutputGateFp.h"
#include "fake-offline/FakeLTBitsGateFp.h"

namespace md_ml {

/// @brief A fake circuit that uses the fake offline protocol to compute the result for Fp domain
/// @tparam ShrType Share type (must satisfy IsFpShare)
/// @tparam N Number of parties
template <IsFpShare ShrType, std::size_t N, bool UseMac = true>
class FakeCircuitFp {
public:
    using SemiShrType = typename ShrType::SemiShrType;
    using ClearType = typename ShrType::ClearType;

    explicit FakeCircuitFp(FakePartyFp<ShrType, N, UseMac>& p_fake_party) : fake_party_(p_fake_party) {}

    void runOffline();

    void addEndpoint(const std::shared_ptr<FakeGateFp<ShrType, N, UseMac>>& gate);

    std::shared_ptr<FakeInputGateFp<ShrType, N, UseMac>>
    input(std::size_t owner_id, std::size_t dim_row, std::size_t dim_col);

    std::shared_ptr<FakeOutputGateFp<ShrType, N, UseMac>>
    output(const std::shared_ptr<FakeGateFp<ShrType, N, UseMac>>& input_x);

    std::shared_ptr<FakeLTBitsGateFp<ShrType, N, UseMac>>
    ltBits(const std::vector<std::shared_ptr<FakeGateFp<ShrType, N, UseMac>>>& p_input_x);

    [[nodiscard]] auto& endpoints() { return endpoints_; }

private:
    FakePartyFp<ShrType, N, UseMac>& fake_party_;
    std::vector<std::shared_ptr<FakeGateFp<ShrType, N, UseMac>>> gates_;
    std::vector<std::shared_ptr<FakeGateFp<ShrType, N, UseMac>>> endpoints_;
};


template <IsFpShare ShrType, std::size_t N, bool UseMac>
void FakeCircuitFp<ShrType, N, UseMac>::
runOffline() {
    for (const auto& gatePtr : endpoints_) {
        gatePtr->runOffline();
    }
}

template <IsFpShare ShrType, std::size_t N, bool UseMac>
void FakeCircuitFp<ShrType, N, UseMac>::
addEndpoint(const std::shared_ptr<FakeGateFp<ShrType, N, UseMac>>& gate) {
    endpoints_.push_back(gate);
}

template <IsFpShare ShrType, std::size_t N, bool UseMac>
std::shared_ptr<FakeInputGateFp<ShrType, N, UseMac>> FakeCircuitFp<ShrType, N, UseMac>::
input(std::size_t owner_id, std::size_t dim_row, std::size_t dim_col) {
    auto gate = std::make_shared<FakeInputGateFp<ShrType, N, UseMac>>(fake_party_, dim_row, dim_col, owner_id);
    gates_.push_back(gate);
    return gate;
}

template <IsFpShare ShrType, std::size_t N, bool UseMac>
std::shared_ptr<FakeOutputGateFp<ShrType, N, UseMac>> FakeCircuitFp<ShrType, N, UseMac>::
output(const std::shared_ptr<FakeGateFp<ShrType, N, UseMac>>& input_x) {  
    auto gate = std::make_shared<FakeOutputGateFp<ShrType, N, UseMac>>(input_x);
    gates_.push_back(gate);
    return gate;
}

template <IsFpShare ShrType, std::size_t N, bool UseMac>
std::shared_ptr<FakeLTBitsGateFp<ShrType, N, UseMac>> FakeCircuitFp<ShrType, N, UseMac>::
ltBits(const std::vector<std::shared_ptr<FakeGateFp<ShrType, N, UseMac>>>& p_input_x) {
    auto gate = std::make_shared<FakeLTBitsGateFp<ShrType, N, UseMac>>(p_input_x);
    gates_.push_back(gate);
    return gate;
}

} // namespace md_ml

#endif //MD_ML_FAKECIRCUITFP_H