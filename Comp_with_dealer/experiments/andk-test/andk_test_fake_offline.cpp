// Fake offline phase for AND-k gate test

#include <iostream>
#include <vector>
#include <string>

#include "share/TinyOTShare.h"
#include "fake-offline/FakePartyTinyOT.h"
#include "fake-offline/FakeCircuitTinyOT.h"
// #include "fake-offline/FakeBooleanAndGateK.h"

using namespace md_ml;

constexpr char kJobName[] = "andk-test";
constexpr std::size_t k = 3;  // 3-input AND
constexpr std::size_t num_elements = 4; // 每个输入包含的元素数量

int main() {
    std::cout << "=== AND-" << k << " Gate Fake Offline ===" << std::endl;

    using ShrType = TinyOTShare64;
    constexpr std::size_t N = 2;     // 两方

    // 创建Party和Circuit
    FakePartyTinyOT<ShrType, N> party(kJobName);
    FakeCircuitTinyOT<ShrType, N> circuit(party);

    // 创建k个输入
    std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>> inputs;
    for (std::size_t i = 0; i < k; ++i) {
        auto input = circuit.input(0, 2, 2);
        inputs.push_back(input);
    }

    // 创建AND-k门
    auto andk_gate = circuit.booleanAndK(inputs);

    // 创建输出门
    auto output = circuit.output(andk_gate);

    // 添加端点
    circuit.addEndpoint(output);

    // 生成预处理数据
    std::cout << "Generating preprocessing data for AND-" << k << " gate..." << std::endl;
    std::cout << "Number of subsets to generate: " << ((1 << k) - 1) << std::endl;

    circuit.runOffline();

    std::cout << "Preprocessing data generated successfully!" << std::endl;
    std::cout << "Files created:" << std::endl;
    std::cout << "  - fake-offline-data/" << kJobName << "-party-0.txt" << std::endl;
    std::cout << "  - fake-offline-data/" << kJobName << "-party-1.txt" << std::endl;

    return 0;
}