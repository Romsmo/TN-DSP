#include <cstring>

#include "hal/nvs.h"
#include "nvs.h"
#include "nvs_flash.h"

namespace hal::esp {

class EspNvs final : public hal::Nvs {
public:
    bool get_blob(const std::string& ns, const std::string& key,
                  std::vector<uint8_t>& out) override {
        nvs_handle_t h;
        if (nvs_open(ns.c_str(), NVS_READONLY, &h) != ESP_OK) return false;
        size_t len = 0;
        esp_err_t err = nvs_get_blob(h, key.c_str(), nullptr, &len);
        if (err == ESP_OK && len > 0) {
            out.resize(len);
            err = nvs_get_blob(h, key.c_str(), out.data(), &len);
        }
        nvs_close(h);
        return err == ESP_OK;
    }

    bool set_blob(const std::string& ns, const std::string& key, const uint8_t* data,
                  size_t len) override {
        nvs_handle_t h;
        if (nvs_open(ns.c_str(), NVS_READWRITE, &h) != ESP_OK) return false;
        const esp_err_t err = nvs_set_blob(h, key.c_str(), data, len);
        nvs_close(h);
        return err == ESP_OK;
    }

    bool erase(const std::string& ns, const std::string& key) override {
        nvs_handle_t h;
        if (nvs_open(ns.c_str(), NVS_READWRITE, &h) != ESP_OK) return false;
        const esp_err_t err = nvs_erase_key(h, key.c_str());
        nvs_close(h);
        return err == ESP_OK;
    }

    bool commit(const std::string& ns) override {
        nvs_handle_t h;
        if (nvs_open(ns.c_str(), NVS_READWRITE, &h) != ESP_OK) return false;
        const esp_err_t err = nvs_commit(h);
        nvs_close(h);
        return err == ESP_OK;
    }
};

hal::Nvs* nvs_instance() {
    static bool inited = false;
    if (!inited) {
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            nvs_flash_erase();
            nvs_flash_init();
        }
        inited = true;
    }
    static EspNvs n;
    return &n;
}

}  // namespace hal::esp
