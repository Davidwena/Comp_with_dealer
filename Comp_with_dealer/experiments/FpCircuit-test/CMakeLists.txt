add_executable(FpCircuit_fake_offline FpCircuit_fake_offline.cpp)
target_link_libraries(FpCircuit_fake_offline ${FAKE_OFFLINE_LIB})

add_executable(FpCircuit_party_0 FpCircuit_party_0.cpp)
target_link_libraries(FpCircuit_party_0 ${ONLINE_LIB})

add_executable(FpCircuit_party_1 FpCircuit_party_1.cpp)
target_link_libraries(FpCircuit_party_1 ${ONLINE_LIB})