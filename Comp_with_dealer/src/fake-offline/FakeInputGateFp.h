// By Kaiwen Wang

#ifndef MD_ML_FAKEINPUTGATEFP_H
#define MD_ML_FAKEINPUTGATEFP_H

#include <memory>
#include <algorithm>

#include "utils/rand.h"
#include "share/IsFpShare.h"
#include "fake-offline/FakeGateFp.h"

namespace md_ml {

template <IsFpShare ShrType, std::size_t N, bool UseMac = true>
class FakeInputGateFp : public FakeGateFp<ShrType, N, UseMac> {
public:
    using ClearType = typename ShrType::ClearType;

    FakeInputGateFp(FakePartyFp<ShrType, N, UseMac>& p_fake_party,
                    std::size_t p_dim_row, std::size_t p_dim_col,
                    std::size_t p_owner_id);

private:
    void doRunOffline() override;

    size_t owner_id_;
};


template <IsFpShare ShrType, std::size_t N, bool UseMac>
FakeInputGateFp<ShrType, N, UseMac>::FakeInputGateFp(
    FakePartyFp<ShrType, N, UseMac>& p_fake_party,
    std::size_t p_dim_row, std::size_t p_dim_col,
    std::size_t p_owner_id)
    : FakeGateFp<ShrType, N, UseMac>(p_fake_party, p_dim_row, p_dim_col), owner_id_(p_owner_id) {}


template <IsFpShare ShrType, std::size_t N, bool UseMac>
void FakeInputGateFp<ShrType, N, UseMac>::doRunOffline() {
    auto size = this->dim_row() * this->dim_col();

    // Generate random lambda values
    this->lambda_clear().resize(size);
    
    if constexpr (sizeof(ClearType) == sizeof(uint32_t)) {
        std::ranges::generate(this->lambda_clear(), []() {
            return ClearType(getRand<uint32_t>());
        });
    } else {
        std::ranges::generate(this->lambda_clear(), []() {
            return ClearType(getRand<uint64_t>());
        });
    }

    // Generate shares (with or without MAC based on UseMac)
    auto shares_and_macs = this->fake_party().GenerateAllPartiesShares(this->lambda_clear());
    this->lambda_shr() = std::move(shares_and_macs.value_shares);
    
    if constexpr (UseMac) {
        this->lambda_shr_mac() = std::move(shares_and_macs.mac_shares);
    }

    // Write preprocessing data
    this->fake_party().WriteClearToIthParty(this->lambda_clear(), owner_id_);
    this->fake_party().WriteSharesToAllParites(this->lambda_shr());
    
    if constexpr (UseMac) {
        this->fake_party().WriteMacSharesToAllParties(this->lambda_shr_mac());
    }
}


} // namespace md_ml

#endif //MD_ML_FAKEINPUTGATEFP_H