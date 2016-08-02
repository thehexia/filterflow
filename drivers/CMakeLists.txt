add_executable(filter-driver filter.cpp)

target_link_libraries(filter-driver freeflow runtime pcap)
