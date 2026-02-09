// Online phase for PrefixAND module test

#include <iostream>
#include <vector>
#include <string>
#include <bitset>

#include "share/TinyOTShare.h"
#include "protocols/PartyWithFakeOfflineTinyOT.h"
#include "protocols/CircuitTinyOT.h"
#include "protocols/BooleanPrefixAndModule.h"

using namespace md_ml;

constexpr char kJobName[] = "prefixand-test";
constexpr std::size_t n = 32;           // 输入长度
constexpr std::size_t max_branch = 4;   // 最大扇入数
constexpr std::size_t num_elements = 4; // 每个输入的宽度

int main() {
    std::cout << "=== PrefixAND Module Online Phase Party 1 ===" << std::endl;
    std::cout << "Configuration: n=" << n << ", k=" << max_branch << std::endl;

    using ShrType = TinyOTShare64;
    constexpr std::size_t N = 2;  // 两方
    // 创建Party和Circuit (baseport=5050)
    PartyWithFakeOfflineTinyOT<ShrType> party(1, N, 5050, kJobName);
    // 创建主电路
    CircuitTinyOT<ShrType> main_circuit(party);

    // 创建输入门（与预处理阶段一致）
    std::vector<std::shared_ptr<GateTinyOT<ShrType>>> inputs;
    for (std::size_t i = 0; i < n; ++i) {
        auto input = main_circuit.input(0, num_elements, 1);  // owner=0
        inputs.push_back(input);
    }

    // 创建PrefixAND模块
    BooleanPrefixAndModule<ShrType> prefix_and(inputs, max_branch, party);
    std::cout << "PrefixAND module constructed (rounds=" << prefix_and.num_rounds() << ")" << std::endl;

    // 为每个输出创建输出门（与预处理阶段一致）
    std::vector<std::shared_ptr<BooleanOutputGate<ShrType>>> output_gates;
    for (std::size_t i = 0; i < n; ++i) {
        auto output = main_circuit.output(prefix_and.getOutput(i));
        output_gates.push_back(output);
        main_circuit.addEndpoint(output);
    }
    std::cout << "Created " << output_gates.size() << " output gates" << std::endl;
    // 读取预处理数据
    std::cout << "\nReading preprocessing data..." << std::endl;

    main_circuit.readOfflineFromFile();

    // 运行在线计算
    main_circuit.runOnlineWithBenchmark();
    main_circuit.printStats();
    
}