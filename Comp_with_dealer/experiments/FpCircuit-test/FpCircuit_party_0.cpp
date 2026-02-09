// By Boshi Yuan

#include <ranges>
#include <iostream>
#include "share/FpShare.h"
#include "protocols/CircuitFp.h"
#include "utils/print_vector.h"

using namespace std;
using namespace md_ml;

int main() {
    using ShrType = FpShare61;
    using ClearType = ShrType::ClearType;

    PartyWithFakeOfflineFp<ShrType> party(0, 2, 5050, "test_fp");
    CircuitFp<ShrType> circuit(party);

    // Test for InputGateFp and OutputGateFp correctness
    // Circuit: input(10x5) -> output
    auto a = circuit.input(0, 10, 5);  // Party 0 provides 10x5 matrix
    auto b = circuit.output(a);        // Output the same matrix
    circuit.addEndpoint(b);

    // Set input data (10x5 = 50 elements)
    vector<ClearType> input_data;
    for (size_t i = 0; i < 50; ++i) {
        input_data.push_back(ClearType(i + 1));  // Values: 1, 2, 3, ..., 50
    }
    a->setInput(input_data);

    cout << "Party 0: Input data set" << endl;
    cout << "Input values: ";
    for (size_t i = 0; i < 10 && i < input_data.size(); ++i) {
        cout << input_data[i] << " ";
    }

    // Run online phase
    circuit.readOfflineFromFile();
    circuit.runOnlineWithBenchmark();
    circuit.printStats();

    // Get output and verify
    auto output = b->getClear();
    
    cout << "\nParty 0: Output data received" << endl;
    cout << "Output values: ";
    for (size_t i = 0; i < 10 && i < output.size(); ++i) {
        cout << output[i] << " ";
    }

    // Verify correctness
    bool correct = true;
    if (input_data.size() != output.size()) {
        cout << "\nERROR: Size mismatch! Input: " << input_data.size() 
             << ", Output: " << output.size() << endl;
        correct = false;
    } else {
        for (size_t i = 0; i < input_data.size(); ++i) {
            if (input_data[i] != output[i]) {
                cout << "\nERROR: Mismatch at index " << i 
                     << "! Input: " << input_data[i] 
                     << ", Output: " << output[i] << endl;
                correct = false;
                break;
            }
        }
    }

    if (correct) {
        cout << "\n✓ TEST PASSED: Input and output match perfectly!" << endl;
    } else {
        cout << "\n✗ TEST FAILED: Input and output do not match!" << endl;
    }

    return 0;
}