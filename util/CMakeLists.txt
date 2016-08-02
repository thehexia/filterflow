add_library(runtime SHARED
  action.cpp
  binding.cpp
  buffer.cpp
  context.cpp
  packet.cpp
  types.cpp
  endian.cpp
  dataplane.cpp
  application.cpp
  port.cpp
  system.cpp
  table.cpp
  flow.cpp
)

target_link_libraries(runtime farmhash pcap dl)
