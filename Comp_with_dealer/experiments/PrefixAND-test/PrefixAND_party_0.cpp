// Online phase for PrefixOR module test - Enhanced version

#include <iostream>
#include <vector>
#include <string>
#include <bitset>
#include <iomanip>

#include "share/TinyOTShare.h"
#include "protocols/PartyWithFakeOfflineTinyOT.h"
#include "protocols/CircuitTinyOT.h"
#include "protocols/BooleanPrefixAndModule.h"

using namespace md_ml;

constexpr char kJobName[] = "prefixand-test";
constexpr std::size_t n = 32;           // 输入长度（32位）
constexpr std::size_t max_branch = 4;   // 最大扇入数
constexpr std::size_t num_elements = 4; // batch size（同时处理4个数）

int main() {
    std::cout << "=== PrefixAND Module Online Phase Party 0 ===" << std::endl;
    std::cout << "Configuration: n=" << n << ", k=" << max_branch 
              << ", batch_size=" << num_elements << std::endl;

    using ShrType = TinyOTShare64;
    using IntegerType = uint32_t;
    constexpr std::size_t N = 2;  // 两方
    
    // 创建Party和Circuit
    PartyWithFakeOfflineTinyOT<ShrType> party(0, N, 5050, kJobName);
    CircuitTinyOT<ShrType> main_circuit(party);

    // 创建输入门
    std::vector<std::shared_ptr<GateTinyOT<ShrType>>> inputs;
    for (std::size_t i = 0; i < n; ++i) {
        auto input = main_circuit.input(0, num_elements, 1);  // owner=0
        inputs.push_back(input);
    }

    // 创建PrefixAND模块
    BooleanPrefixAndModule<ShrType> prefix_and(inputs, max_branch, party);
    std::cout << "PrefixAND module constructed (rounds=" << prefix_and.num_rounds() << ")" << std::endl;

    // 为每个输出创建输出门
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

    // ========== 测试数据 ==========
    std::vector<IntegerType> test_values = {
        0x00000079,
        0x00000006,
        0x0000000d,
        0x00000018
    };
    
    std::cout << "\n=== Test Input Values ===" << std::endl;
    for (std::size_t b = 0; b < num_elements; ++b) {
        std::cout << "Value[" << b << "] = " << test_values[b] 
                  << " (0b" << std::bitset<32>(test_values[b]) << ")" << std::endl;
    }

    // ========== 设置输入 ==========
    // inputs[i] 包含所有 batch 元素在第 i 位的值
    for (std::size_t i = 0; i < n; ++i) {
        std::vector<bool> bit_values(num_elements);
        for (std::size_t b = 0; b < num_elements; ++b) {
            bit_values[b] = (test_values[b] >> i) & 1;
        }
        
        auto input_gate = std::dynamic_pointer_cast<BooleanInputGate<ShrType>>(inputs[i]);
        if (input_gate) {
            input_gate->setInput(bit_values);
        }
    }
    
    std::cout << "\n=== Input Bits (first 8 bits) ===" << std::endl;
    for (std::size_t i = 0; i < std::min(size_t(8), n); ++i) {
        std::cout << "Bit[" << i << "]: ";
        for (std::size_t b = 0; b < num_elements; ++b) {
            std::cout << ((test_values[b] >> i) & 1) << " ";
        }
        std::cout << std::endl;
    }

    // ========== 运行在线计算 ==========
    std::cout << "\n=== Running Online Phase ===" << std::endl;
    main_circuit.runOnlineWithBenchmark();

    // ========== 计算期望结果（明文验证）==========
    std::vector<std::vector<bool>> expected_results(n, std::vector<bool>(num_elements));

    for (std::size_t b = 0; b < num_elements; ++b) {
        // 对每个 batch 元素
        for (std::size_t i = 0; i < n; ++i) {
            // PrefixOR[i] = bit[0] | bit[1] | ... | bit[i]
            bool and_result = true;
            for (std::size_t j = 0; j <= i; ++j) {  // ✅ 从 0 到 i（包含）
                and_result ^= ((test_values[b] >> j) & 1);
            }
            expected_results[i][b] = and_result;
        }
    }

    // ========== 验证结果 ==========
    std::cout << "\n=== PrefixOR Results & Verification ===" << std::endl;
    
    bool all_correct = true;
    for (std::size_t i = 0; i < std::min(size_t(16), n); ++i) {
        std::cout << "\nPrefixOR[" << std::setw(2) << i << "]: ";
        
        bool batch_correct = true;
        for (std::size_t b = 0; b < num_elements; ++b) {
            bool actual = output_gates[i]->getClear()[b];
            bool expected = expected_results[i][b];
            
            std::cout << "[" << b << "]=" << actual;
            
            if (actual != expected) {
                std::cout << "✗(exp:" << expected << ") ";
                batch_correct = false;
                all_correct = false;
            } else {
                std::cout << "✓ ";
            }
        }
        
        if (batch_correct) {
            std::cout << " [PASS]";
        } else {
            std::cout << " [FAIL]";
        }
        std::cout << std::endl;
    }
    
    // ========== 详细验证一个batch元素 ==========
    std::cout << "\n=== Detailed Verification (Batch[0]) ===" << std::endl;
    std::cout << "Input value: " << test_values[0] 
              << " (0b" << std::bitset<32>(test_values[0]) << ")" << std::endl;
    std::cout << std::endl;
    std::cout << "Bit | Input | Expected | Actual | Status" << std::endl;
    std::cout << "----+-------+----------+--------+--------" << std::endl;
    
    for (std::size_t i = 0; i < std::min(size_t(16), n); ++i) {
        bool input_bit = (test_values[0] >> i) & 1;
        bool expected = expected_results[i][0];
        bool actual = output_gates[i]->getClear()[0];
        
        std::cout << std::setw(3) << i << " | "
                  << std::setw(5) << input_bit << " | "
                  << std::setw(8) << expected << " | "
                  << std::setw(6) << actual << " | "
                  << (actual == expected ? "✓ PASS" : "✗ FAIL")
                  << std::endl;
    }
    
    std::cout << "\n=== Summary ===" << std::endl;
    if (all_correct) {
        std::cout << "✓ All tests PASSED!" << std::endl;
    } else {
        std::cout << "✗ Some tests FAILED!" << std::endl;
    }
    
    main_circuit.printStats();
    
    return all_correct ? 0 : 1;
}