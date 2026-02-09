#include <iostream>
#include <vector>
#include <string>
#include <cmath>

#include "share/TinyOTShare.h"
#include "fake-offline/FakePartyTinyOT.h"
#include "fake-offline/FakeCircuitTinyOT.h"
#include "fake-offline/FakeBooleanPrefixAndModule.h"

using namespace md_ml;

constexpr char kJobName[] = "prefixand-test";
constexpr std::size_t n = 32;           // 输入长度（PrefixAND 的长度）
constexpr std::size_t max_branch = 4;   // 最大扇入数（k值）
constexpr std::size_t num_elements = 4; // 每个输入的宽度

int main() {
    std::cout << "=== PrefixAND Module Fake Offline ===" << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "  - Input length (n): " << n << std::endl;
    std::cout << "  - Max branch (k): " << max_branch << std::endl;
    std::cout << "  - Element width: " << num_elements << std::endl;

    using ShrType = TinyOTShare64;
    constexpr std::size_t N = 2;  // 两方

    // 创建Party
    FakePartyTinyOT<ShrType, N> party(kJobName);
    
    // 创建主电路（用于管理输出门）
    FakeCircuitTinyOT<ShrType, N> main_circuit(party);

    // 创建n个输入门
    std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>> inputs;
    for (std::size_t i = 0; i < n; ++i) {
        auto input = main_circuit.input(0, num_elements, 1);  // owner=0, dim=(num_elements,1)
        inputs.push_back(input);
    }
    // 创建PrefixAND模块
    FakeBooleanPrefixAndModule<ShrType, N> prefix_and(inputs, max_branch, party);
    std::cout << "PrefixAND module constructed:" << std::endl;
    std::cout << "  - Number of outputs: " << prefix_and.size() << std::endl;
    std::cout << "  - Number of rounds: " << prefix_and.num_rounds() << std::endl;
    // 为每个PrefixAND输出创建输出门，并添加到主电路
    std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>> output_gates;
    for (std::size_t i = 0; i < prefix_and.size(); ++i) {
        auto prefix_output = prefix_and.getOutput(i);
        auto output_gate = main_circuit.output(prefix_output);
        output_gates.push_back(output_gate);
        main_circuit.addEndpoint(output_gate);  // 添加为endpoint
    }
    std::cout << "Generating preprocessing data..." << std::endl;
    main_circuit.runOffline();

    std::cout << "Files created in fake-offline-data/" << std::endl;
    return 0;
}