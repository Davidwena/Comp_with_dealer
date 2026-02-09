// By Kaiwen Wang
/// @file Circuit for TinyOT Boolean operations

#ifndef MD_ML_CIRCUITTINYOT_H
#define MD_ML_CIRCUITTINYOT_H

#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>


#include "utils/Timer.h"
#include "share/IsTinyOTShare.h"
#include "protocols/PartyWithFakeOfflineTinyOT.h"
#include "protocols/GateTinyOT.h"
#include "protocols/BooleanInputGate.h"
#include "protocols/BooleanAndGateBatch.h"
#include "protocols/BooleanOutputGate.h"
#include "protocols/BooleanAndGateK.h"
#include "protocols/LTBitsGate.h"

namespace md_ml {

/**
 * CircuitTinyOT: 用于TinyOT布尔电路的电路类
 *
 * 功能：
 * - 管理TinyOT门的创建和执行
 * - 提供input(), booleanAndBatch(), output()等工厂方法
 * - 管理电路的离线和在线执行
 *
 * 注意：使用booleanAndBatch(x, y, 1)代替单个AND门
 */
template <IsTinyOTShare ShrType>
class CircuitTinyOT {
public:
    using ClearType = typename ShrType::ClearType;  // bool
    using IntegerType = typename ShrType::IntegerType;

    explicit CircuitTinyOT(PartyWithFakeOfflineTinyOT<ShrType>& party) : party_(party) {}

    void addEndpoint(const std::shared_ptr<GateTinyOT<ShrType>>& gate);
    void runOffline();
    void readOfflineFromFile();
    void runOnline();
    void runOnlineWithBenchmark();
    void printStats();
    void runOnlineLayered();

    std::shared_ptr<BooleanInputGate<ShrType>>
    input(std::size_t owner_id, std::size_t dim_row, std::size_t dim_col);

    std::shared_ptr<BooleanAndGateBatch<ShrType>>
    booleanAndBatch(const std::vector<std::shared_ptr<GateTinyOT<ShrType>>>& input_x_vec,
                    const std::vector<std::shared_ptr<GateTinyOT<ShrType>>>& input_y_vec);

    std::shared_ptr<BooleanAndGateK<ShrType>>
    booleanAndK(const std::vector<std::shared_ptr<GateTinyOT<ShrType>>>& inputs);

    std::shared_ptr<BooleanOutputGate<ShrType>>
    output(const std::shared_ptr<GateTinyOT<ShrType>>& input);

    //比特长度等于inputs.size()
    std::shared_ptr<LTBitsGate<ShrType>>
    ltbits(const std::vector<std::shared_ptr<GateTinyOT<ShrType>>>& inputs,
            std::vector<IntegerType> R_values,  std::size_t max_branch);

    [[nodiscard]] auto& endpoints() { return endpoints_; }

private:
    PartyWithFakeOfflineTinyOT<ShrType>& party_;
    std::vector<std::shared_ptr<GateTinyOT<ShrType>>> gates_;
    std::vector<std::shared_ptr<GateTinyOT<ShrType>>> endpoints_;
    Timer timer_;

    void computeDepths(
        const std::vector<std::shared_ptr<GateTinyOT<ShrType>>>& endpoints,
        std::unordered_map<GateTinyOT<ShrType>*, int>& depths) {
        
        for (const auto& gate : endpoints) {
            computeGateDepth(gate.get(), depths);
        }
    }
    
    int computeGateDepth(
        GateTinyOT<ShrType>* gate,
        std::unordered_map<GateTinyOT<ShrType>*, int>& depths) {
        
        // 如果已经计算过，直接返回
        auto it = depths.find(gate);
        if (it != depths.end()) {
            return it->second;
        }
        
        int max_input_depth = -1;  // 没有输入的门depth为0
        
        // 递归计算所有输入的深度
        if (gate->input_x_) {
            max_input_depth = std::max(max_input_depth, 
                computeGateDepth(gate->input_x_.get(), depths));
        }
        if (gate->input_y_) {
            max_input_depth = std::max(max_input_depth,
                computeGateDepth(gate->input_y_.get(), depths));
        }
        for (auto& input : gate->inputs_) {
            if (input) {
                max_input_depth = std::max(max_input_depth,
                    computeGateDepth(input.get(), depths));
            }
        }
        
        // 当前门的深度 = 最大输入深度 + 1
        int my_depth = max_input_depth + 1;
        depths[gate] = my_depth;
        return my_depth;
    }
    
    void collectByDepth(
        GateTinyOT<ShrType>* gate,
        const std::unordered_map<GateTinyOT<ShrType>*, int>& depths,
        std::map<int, std::vector<GateTinyOT<ShrType>*>>& layers,
        std::unordered_set<GateTinyOT<ShrType>*>& collected) {
        
        if (collected.find(gate) != collected.end()) {
            return;
        }
        
        // 先递归收集所有输入门
        if (gate->input_x_) {
            collectByDepth(gate->input_x_.get(), depths, layers, collected);
        }
        if (gate->input_y_) {
            collectByDepth(gate->input_y_.get(), depths, layers, collected);
        }
        for (auto& input : gate->inputs_) {
            if (input) {
                collectByDepth(input.get(), depths, layers, collected);
            }
        }
        
        int depth = depths.at(gate);
        layers[depth].push_back(gate);
        collected.insert(gate);
    }
};


template <IsTinyOTShare ShrType>
void CircuitTinyOT<ShrType>::addEndpoint(const std::shared_ptr<GateTinyOT<ShrType>>& gate) {
    endpoints_.push_back(gate);
}


template <IsTinyOTShare ShrType>
void CircuitTinyOT<ShrType>::runOffline() {
    for (const auto& gate : endpoints_) {
        gate->RunOffline();
    }
}


template <IsTinyOTShare ShrType>
void CircuitTinyOT<ShrType>::readOfflineFromFile() {
    for (const auto& gate : endpoints_) {
        gate->readOfflineFromFile();
    }
}


template <IsTinyOTShare ShrType>
void CircuitTinyOT<ShrType>::runOnline() {
    for (const auto& gate : endpoints_) {
        gate->RunOnline();
    }
}

template <IsTinyOTShare ShrType>
void CircuitTinyOT<ShrType>::runOnlineLayered() {
    std::unordered_map<GateTinyOT<ShrType>*, int> depths;
    computeDepths(endpoints_, depths);
    
    std::map<int, std::vector<GateTinyOT<ShrType>*>> layers;
    std::unordered_set<GateTinyOT<ShrType>*> collected;
    
    for (const auto& gate : endpoints_) {
        collectByDepth(gate.get(), depths, layers, collected);
    }
    
    // 按层执行
    for (auto& [depth, gates] : layers) {
        #pragma omp parallel for
        for (size_t i = 0; i < gates.size(); ++i) {
            if (!gates[i]->isEvaluatedOnline()) {  // 使用getter
                gates[i]->doRunOnline();
                gates[i]->setEvaluatedOnline(true);  // 使用setter
            }
        }
    }
}

template <IsTinyOTShare ShrType>
void CircuitTinyOT<ShrType>::runOnlineWithBenchmark() {
    timer_.start();
    runOnlineLayered();
    timer_.stop();
}


template <IsTinyOTShare ShrType>
void CircuitTinyOT<ShrType>::printStats() {
    std::cout
        << "Spent " << timer_.elapsed() << " ms\n"
        << "Sent " << party_.bytes_sent() << " bytes\n";
}


template <IsTinyOTShare ShrType>
std::shared_ptr<BooleanInputGate<ShrType>> CircuitTinyOT<ShrType>::
input(std::size_t owner_id, std::size_t dim_row, std::size_t dim_col) {
    auto gate = std::make_shared<BooleanInputGate<ShrType>>(party_, owner_id, dim_row, dim_col);
    gates_.push_back(gate);
    return gate;
}


template <IsTinyOTShare ShrType>
std::shared_ptr<BooleanAndGateBatch<ShrType>> CircuitTinyOT<ShrType>::
booleanAndBatch(const std::vector<std::shared_ptr<GateTinyOT<ShrType>>>& input_x_vec,
                const std::vector<std::shared_ptr<GateTinyOT<ShrType>>>& input_y_vec) {
    auto gate = std::make_shared<BooleanAndGateBatch<ShrType>>(input_x_vec, input_y_vec);
    gates_.push_back(gate);
    return gate;
}

template <IsTinyOTShare ShrType>
std::shared_ptr<BooleanAndGateK<ShrType>> CircuitTinyOT<ShrType>::
booleanAndK(const std::vector<std::shared_ptr<GateTinyOT<ShrType>>>& inputs) {
    auto gate = std::make_shared<BooleanAndGateK<ShrType>>(inputs);
    gates_.push_back(gate);
    return gate;
}


template <IsTinyOTShare ShrType>
std::shared_ptr<BooleanOutputGate<ShrType>> CircuitTinyOT<ShrType>::
output(const std::shared_ptr<GateTinyOT<ShrType>>& input) {
    auto gate = std::make_shared<BooleanOutputGate<ShrType>>(input);
    gates_.push_back(gate);
    return gate;
}

template <IsTinyOTShare ShrType>
std::shared_ptr<LTBitsGate<ShrType>> CircuitTinyOT<ShrType>::
ltbits(const std::vector<std::shared_ptr<GateTinyOT<ShrType>>>& inputs,
            std::vector<IntegerType> R_values,  std::size_t max_branch) {
    auto gate = std::make_shared<LTBitsGate<ShrType>>(inputs, R_values, max_branch);
    return gate;
}


} // namespace md_ml

#endif //MD_ML_CIRCUITTINYOT_H