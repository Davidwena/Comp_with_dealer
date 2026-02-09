// Party 0: AND-k gate online computation test

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
constexpr std::size_t k = 3;  // 3-input AND
constexpr std::size_t num_elements = 4;

int main() {
    std::cout << "=== AND-" << k << " Gate Test - Party 0 ===" << std::endl;

    using ShrType = TinyOTShare64;
    constexpr std::size_t N = 2;     // 两方

    // 创建Party和Circuit (baseport=5050)
    PartyWithFakeOfflineTinyOT<ShrType> party(0, N, 5050, kJobName);
    CircuitTinyOT<ShrType> circuit(party);

    std::cout << "Party 0 initialized, waiting for Party 1..." << std::endl;

    // 创建k个输入 使用两个向量 共享指针指向同一个实体
    std::vector<std::shared_ptr<BooleanInputGate<ShrType>>> input_gates;  // 用于setInput
    std::vector<std::shared_ptr<GateTinyOT<ShrType>>> inputs;        // 用于booleanAndK

    for (std::size_t i = 0; i < k; ++i) {
        auto input = circuit.input(0, 2, 2);  // num_elements x 1 维度
        input_gates.push_back(input);       // 保存GateTinyOT指针用于AND门
        inputs.push_back(input);  // 同时保存为GateTinyOT（自动向上转型）
    }

    // Party 0 设置输入值
    // 测试用例1：全部为true
    std::vector<bool> test_input_1 = {true, true, true};

    // 测试用例2：一个为false
    std::vector<bool> test_input_2 = {false, true, false};

    // 测试用例3：全部为false
    std::vector<bool> test_input_3 = {true, false, true};

    // 测试用例3：全部为false
    std::vector<bool> test_input_4 = {false, false, false};

    // 选择测试用例
    std::vector<std::vector<bool>> test_input = {test_input_1,
                                                 test_input_2,
                                                 test_input_3,
                                                 test_input_4}; 

    // 设置输入
    for (std::size_t i = 0; i < k; ++i) {
        std::vector<bool> input_val;
        for(size_t j = 0 ; j < num_elements; j++){
            input_val.push_back(test_input[j][i]);
        }
        input_gates[i]->setInput(input_val);  // 使用BooleanInputGate的setInput
    }

    // // 打印输入
    // std::cout << "\nInput values: [";
    // for (std::size_t i = 0; i < k; ++i) {
    //     std::cout << test_input[i];
    //     if (i < k - 1) std::cout << ", ";
    // }
    // std::cout << "]" << std::endl;

    // 使用inputs创建AND-k门
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

    for(size_t i = 0 ; i < result_vec.size(); i++){
        std::cout<<"result_vec["<<i<<"] = "<<result_vec[i]<<std::endl;
    }
    // bool result = result_vec[0];

    // 验证结果
    // bool expected = true;
    // for (bool val : test_input) {
    //     expected = expected && val;
    // }

    // std::cout << "\n=== Results ===" << std::endl;
    // // std::cout << "Expected: " << (expected ? "true" : "false") << std::endl;
    // std::cout << "Result:   " << (result ? "true" : "false") << std::endl;

    // if (result == expected) {
    //     std::cout << "✓ Test PASSED!" << std::endl;
    // } else {
    //     std::cout << "✗ Test FAILED!" << std::endl;
    // }

    circuit.printStats();

    return 0;
}