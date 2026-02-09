// By Boshi Yuan

#include "fake-offline/FakeInputGateFp.h"
#include "fake-offline/FakeOutputGateFp.h"
#include "fake-offline/FakeCircuitFp.h"
#include "share/FpShare.h"
#include "share/IsFpShare.h"
#include "fake-offline/FakePartyFp.h"

using namespace std;
using namespace md_ml;

int main() {
    using ShrType = FpShare61;  // Use 61-bit prime field

    FakePartyFp<ShrType, 2> party("test_fp");
    FakeCircuitFp<ShrType, 2> circuit(party);

    // Test for InputGateFp and OutputGateFp correctness
    // Circuit: input(10x5) -> output
    auto a = circuit.input(0, 10, 5);  // Party 0 provides 10x5 matrix
    auto b = circuit.output(a);        // Output the same matrix

    circuit.addEndpoint(b);
    circuit.runOffline();

    cout << "Fake offline phase completed successfully!" << endl;

    return 0;
}