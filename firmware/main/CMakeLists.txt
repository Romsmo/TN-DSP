# The application component: just the composition root. Shared logic is in the
# `bdsp` component; board drivers are in `hal_esp`.
idf_component_register(
  SRCS app_main.cpp
  INCLUDE_DIRS "." "${CMAKE_CURRENT_LIST_DIR}/../.."
  REQUIRES bdsp hal_esp nvs_flash esp_timer
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
