# Host unit tests. One executable, doctest-based, registered with CTest.

set(BDSP_TEST_SOURCES
  doctest_main.cpp
  model_test.cpp
  interface_compile_test.cpp
  ble_uuids_test.cpp
)

add_executable(bdsp_tests ${BDSP_TEST_SOURCES})
target_link_libraries(bdsp_tests PRIVATE bdsp_core doctest)
if(TARGET bdsp_warnings)
  target_link_libraries(bdsp_tests PRIVATE bdsp_warnings)
endif()

add_test(NAME bdsp_tests COMMAND bdsp_tests --no-version)
set_tests_properties(bdsp_tests PROPERTIES TIMEOUT 60)
