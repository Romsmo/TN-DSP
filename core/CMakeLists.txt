# bdsp_core — platform-independent logic. No ESP-IDF / Arduino / SDL includes.
# Compiles on the host (tests, simulator) and inside the ESP-IDF component (see
# firmware/components/*/CMakeLists.txt which add these same sources).

set(BDSP_CORE_SOURCES
  model/version.cpp
  catalog/category_registry.cpp
)

add_library(bdsp_core STATIC ${BDSP_CORE_SOURCES})
target_include_directories(bdsp_core PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(bdsp_core PUBLIC cxx_std_17)

if(TARGET bdsp_warnings)
  target_link_libraries(bdsp_core PRIVATE bdsp_warnings)
endif()

# Expose the source list so the ESP-IDF build can reuse it verbatim.
set(BDSP_CORE_SOURCES "${BDSP_CORE_SOURCES}" PARENT_SCOPE)
