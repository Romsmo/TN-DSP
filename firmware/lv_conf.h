// The firmware LVGL config is the same file the host build and simulator use.
// BDSP_HOST is undefined here, so the SDL blocks in ui/lv_conf.h are skipped and
// the built-in allocator / on-target memory settings apply.
#include "../ui/lv_conf.h"
