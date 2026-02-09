add_executable(andk_test_fake_offline andk_test_fake_offline.cpp)
target_link_libraries(andk_test_fake_offline ${FAKE_OFFLINE_LIB})

add_executable(andk_test_party_0 andk_test_party_0.cpp)
target_link_libraries(andk_test_party_0 ${ONLINE_LIB})

add_executable(andk_test_party_1 andk_test_party_1.cpp)
target_link_libraries(andk_test_party_1 ${ONLINE_LIB})