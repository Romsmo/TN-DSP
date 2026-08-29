# Host unit tests. One executable, doctest-based, registered with CTest.

set(BDSP_TEST_SOURCES
  doctest_main.cpp
  model_test.cpp
  interface_compile_test.cpp
  ble_uuids_test.cpp
  category_registry_test.cpp
  core_logic_test.cpp
  state_machine_test.cpp
  audio_test.cpp
  codec_test.cpp
  codec_interop_test.cpp
  app_runner_test.cpp
)

# ui/ tests need LVGL; only add them when the ui library is in the build.
if(TARGET bdsp_ui)
  list(APPEND BDSP_TEST_SOURCES i18n_test.cpp sign_catalog_test.cpp screen_manager_test.cpp)
endif()

add_executable(bdsp_tests ${BDSP_TEST_SOURCES})
target_link_libraries(bdsp_tests PRIVATE bdsp_core bdsp_transport doctest)
if(TARGET bdsp_warnings)
  target_link_libraries(bdsp_tests PRIVATE bdsp_warnings)
endif()
if(TARGET bdsp_ui)
  target_link_libraries(bdsp_tests PRIVATE bdsp_ui)
  target_compile_definitions(bdsp_tests PRIVATE BDSP_HAVE_UI=1)
endif()

add_test(NAME bdsp_tests COMMAND bdsp_tests --no-version)
set_tests_properties(bdsp_tests PROPERTIES TIMEOUT 60)
