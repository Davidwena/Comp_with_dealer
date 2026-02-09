// Party 1: AND-k gate online computation test

#include <iostream>
#include <vector>
#include <string>

#include "share/TinyOTShare.h"
#include "protocols/PartyWithFakeOfflineTinyOT.h"
#include "protocols/CircuitTinyOT.h"
#include "protocols/BooleanInputGate.h"
#include "protocols/BooleanAndGateK.h"

using namespace md_ml;

constexpr char kJobName[] = "andk-test";
constexpr std::size_t k = 3;  // 4-input AND
constexpr std::size_t num_elements = 4;

int main() {
    std::cout << "=== AND-" << k << " Gate Test - Party 1 ===" << std::endl;

    using ShrType = TinyOTShare64;
    constexpr std::size_t N = 2;     // 两方

    // 创建Party和Circuit (baseport=5050)
    PartyWithFakeOfflineTinyOT<ShrType> party(1, N, 5050, kJobName);
    CircuitTinyOT<ShrType> circuit(party);

    std::cout << "Party 1 initialized, connecting to Party 0..." << std::endl;

    // 创建k个输入
    std::vector<std::shared_ptr<GateTinyOT<ShrType>>> inputs;
    for (std::size_t i = 0; i < k; ++i) {
        auto input = circuit.input(0, 2, 2);  // num_elements x 1 维度
        inputs.push_back(input);
    }

    // Party 1 不设置输入值（参与计算但不提供输入）

    // 创建AND-k门
    auto andk_gate = circuit.booleanAndK(inputs);

    // 创建输出门
    auto output = circuit.output(andk_gate);

    // 添加端点
    circuit.addEndpoint(output);

    // 读取预处理数据并运行在线计算
    std::cout << "\nReading preprocessing data..." << std::endl;
    circuit.readOfflineFromFile();

    std::cout << "Running online computation..." << std::endl;
    circuit.runOnlineWithBenchmark();

    // 获取结果
    auto result_vec = output->getClear();
    // bool result = result_vec[0];

    // std::cout << "\n=== Results ===" << std::endl;
    // std::cout << "Result: " << (result ? "true" : "false") << std::endl;
    // std::cout << "✓ Computation completed!" << std::endl;

    circuit.printStats();

    return 0;
}