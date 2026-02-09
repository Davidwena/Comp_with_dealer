// By Kaiwen Wang

#ifndef MD_ML_CIRCUIT_FP_H
#define MD_ML_CIRCUIT_FP_H

#include <memory>
#include <vector>

#include "utils/Timer.h"
#include "share/IsFpShare.h"
#include "protocols/PartyWithFakeOfflineFp.h"
#include "protocols/GateFp.h"
#include "protocols/InputGateFp.h"
#include "protocols/OutputGateFp.h"
#include "protocols/LTBitsGateFp.h"

namespace md_ml {

template <IsFpShare ShrType, bool UseMac = true>
class CircuitFp {
public:
    using ClearType = typename ShrType::ClearType;

    static constexpr bool kUseMac = UseMac;

    explicit CircuitFp(PartyWithFakeOfflineFp<ShrType, UseMac>& party) : party_(party) {}

    void addEndpoint(const std::shared_ptr<GateFp<ShrType, UseMac>>& gate);
    void runOffline();
    void readOfflineFromFile();
    void runOnline();
    void runOnlineWithBenchmark();
    void printStats();

    std::shared_ptr<InputGateFp<ShrType, UseMac>>
    input(std::size_t owner_id, std::size_t dim_row, std::size_t dim_col);

    std::shared_ptr<OutputGateFp<ShrType, UseMac>>
    output(const std::shared_ptr<GateFp<ShrType, UseMac>>& input);

    std::shared_ptr<LTBitsGateFp<ShrType, UseMac>>
    ltBits(const std::vector<std::shared_ptr<GateFp<ShrType, UseMac>>>& p_input_x);

    [[nodiscard]] auto& endpoints() { return endpoints_; }
    [[nodiscard]] auto& gates() { return gates_; }
    [[nodiscard]] auto& party() { return party_; }

private:
    PartyWithFakeOfflineFp<ShrType, UseMac>& party_;
    std::vector<std::shared_ptr<GateFp<ShrType, UseMac>>> gates_;
    std::vector<std::shared_ptr<GateFp<ShrType, UseMac>>> endpoints_;
    Timer timer_;
};


// ==================== 实现 ====================

template <IsFpShare ShrType, bool UseMac>
void CircuitFp<ShrType, UseMac>::addEndpoint(const std::shared_ptr<GateFp<ShrType, UseMac>>& gate) {
    endpoints_.push_back(gate);
}

template <IsFpShare ShrType, bool UseMac>
void CircuitFp<ShrType, UseMac>::runOffline() {
    for (const auto& gate : endpoints_) {
        gate->runOffline();
    }
}

template <IsFpShare ShrType, bool UseMac>
void CircuitFp<ShrType, UseMac>::readOfflineFromFile() {
    for (const auto& gate : endpoints_) {
        gate->readOfflineFromFile();
    }
}

template <IsFpShare ShrType, bool UseMac>
void CircuitFp<ShrType, UseMac>::runOnline() {
    for (const auto& gate : endpoints_) {
        gate->RunOnline();
    }
}

template <IsFpShare ShrType, bool UseMac>
void CircuitFp<ShrType, UseMac>::runOnlineWithBenchmark() {
    timer_.start();
    runOnline();
    timer_.stop();
}

template <IsFpShare ShrType, bool UseMac>
void CircuitFp<ShrType, UseMac>::printStats() {
    std::cout
        << "Spent " << timer_.elapsed() << " ms\n"
        << "Sent " << party_.bytes_sent() << " bytes\n";
}

template <IsFpShare ShrType, bool UseMac>
std::shared_ptr<InputGateFp<ShrType, UseMac>> 
CircuitFp<ShrType, UseMac>::input(std::size_t owner_id, std::size_t dim_row, std::size_t dim_col) {
    auto gate = std::make_shared<InputGateFp<ShrType, UseMac>>(party_, dim_row, dim_col, owner_id);
    gates_.push_back(gate);
    return gate;
}

template <IsFpShare ShrType, bool UseMac>
std::shared_ptr<OutputGateFp<ShrType, UseMac>> 
CircuitFp<ShrType, UseMac>::output(const std::shared_ptr<GateFp<ShrType, UseMac>>& input) { 
    auto gate = std::make_shared<OutputGateFp<ShrType, UseMac>>(input);
    gates_.push_back(gate);
    return gate;
}

template <IsFpShare ShrType, bool UseMac>
std::shared_ptr<LTBitsGateFp<ShrType, UseMac>> 
CircuitFp<ShrType, UseMac>::ltBits(const std::vector<std::shared_ptr<GateFp<ShrType, UseMac>>>& p_input_x) {
    auto gate = std::make_shared<LTBitsGateFp<ShrType, UseMac>>(p_input_x);
    gates_.push_back(gate);
    return gate;
}

} // namespace md_ml

#endif //MD_ML_CIRCUIT_FP_H