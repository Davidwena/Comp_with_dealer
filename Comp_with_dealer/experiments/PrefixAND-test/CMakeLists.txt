add_executable(PrefixAND_fake_offline PrefixAND_fake_offline.cpp)
target_link_libraries(PrefixAND_fake_offline ${FAKE_OFFLINE_LIB})

add_executable(PrefixAND_party_0 PrefixAND_party_0.cpp)
target_link_libraries(PrefixAND_party_0 ${ONLINE_LIB})

add_executable(PrefixAND_party_1 PrefixAND_party_1.cpp)
target_link_libraries(PrefixAND_party_1 ${ONLINE_LIB})