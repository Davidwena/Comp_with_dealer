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

    PartyWithFakeOfflineFp<ShrType> party(1, 2, 5050, "test_fp");
    CircuitFp<ShrType> circuit(party);

    // Test for InputGateFp and OutputGateFp correctness
    // Circuit: input(10x5) -> output
    auto a = circuit.input(0, 10, 5);  // Party 0 provides 10x5 matrix
    auto b = circuit.output(a);        // Output the same matrix
    circuit.addEndpoint(b);

    cout << "Party 1: Waiting for input from Party 0" << endl;

    // Run online phase
    circuit.readOfflineFromFile();
    circuit.runOnlineWithBenchmark();
    circuit.printStats();

    // Get output
    auto output = b->getClear();
    
    cout << "\nParty 1: Output data received" << endl;
    cout << "Output values: ";
    for (size_t i = 0; i < 10 && i < output.size(); ++i) {
        cout << output[i] << " ";
    }

    cout << "\nParty 1: Test completed" << endl;

    return 0;
}