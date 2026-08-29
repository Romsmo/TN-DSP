# bdsp_transport — ITransport implementations that need no platform code:
# MockTransport (demo / sim) and SerialTransport (M-later). BleTransport lives in
# the firmware component.
add_library(bdsp_transport STATIC
  mock/mock_transport.cpp
)
target_include_directories(bdsp_transport PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(bdsp_transport PUBLIC cxx_std_17)
target_link_libraries(bdsp_transport PUBLIC bdsp_core)
if(TARGET bdsp_warnings)
  target_link_libraries(bdsp_transport PRIVATE bdsp_warnings)
endif()
