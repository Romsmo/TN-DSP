#include "core/model/version.h"

#ifndef BDSP_FW_VERSION
#define BDSP_FW_VERSION "0.2.0"
#endif

namespace core {

const char* firmware_version() { return BDSP_FW_VERSION; }

}  // namespace core
