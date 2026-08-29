// esp_lvgl.cpp — the LVGL runtime: tick timer, port mutex, handler task.
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"

namespace hal::esp {

namespace {
SemaphoreHandle_t g_mux = nullptr;
constexpr uint32_t kTickMs = 2;

void tick_cb(void*) { lv_tick_inc(kTickMs); }

void lvgl_task(void*) {
    for (;;) {
        uint32_t delay = 5;
        if (xSemaphoreTakeRecursive(g_mux, portMAX_DELAY) == pdTRUE) {
            delay = lv_timer_handler();
            xSemaphoreGiveRecursive(g_mux);
        }
        if (delay < 2) delay = 2;
        if (delay > 100) delay = 100;
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}
}  // namespace

void lvgl_runtime_start() {
    g_mux = xSemaphoreCreateRecursiveMutex();

    const esp_timer_create_args_t args = {
        .callback = tick_cb, .arg = nullptr, .dispatch_method = ESP_TIMER_TASK,
        .name = "lv_tick", .skip_unhandled_events = true};
    esp_timer_handle_t t;
    esp_timer_create(&args, &t);
    esp_timer_start_periodic(t, kTickMs * 1000);

    xTaskCreate(lvgl_task, "lvgl", 8192, nullptr, 5, nullptr);
}

void lvgl_lock() { xSemaphoreTakeRecursive(g_mux, portMAX_DELAY); }
void lvgl_unlock() { xSemaphoreGiveRecursive(g_mux); }

}  // namespace hal::esp
