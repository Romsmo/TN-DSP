// esp_storage.cpp — microSD via FATFS over SPI. The card sits on the SPI2 bus
// that esp_display already brought up, so this only adds the SD *device* (its own
// CS on GPIO15). Bus access is serialised by the SPI master driver; SD reads are
// rare (boot-time asset load) so the occasional LCD stall is acceptable.
#include <cstdio>
#include <cstring>
#include <dirent.h>

#include "board_config.h"
#include "driver/sdspi_host.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "hal/storage.h"
#include "hal_esp.h"
#include "sdmmc_cmd.h"

namespace hal::esp {

namespace {
const char* TAG = "sd";
constexpr const char* kMount = BSP_SD_MOUNT_POINT;

std::string full(const std::string& path) {
    if (!path.empty() && path[0] == '/') return std::string(kMount) + path;
    return std::string(kMount) + "/" + path;
}

class EspStorage final : public hal::Storage {
public:
    bool mount() {
        esp_vfs_fat_sdmmc_mount_config_t mcfg = {};
        mcfg.format_if_mount_failed = false;
        mcfg.max_files = 5;
        mcfg.allocation_unit_size = 512;

        sdmmc_host_t host = SDSPI_HOST_DEFAULT();
        host.slot = BSP_SD_SPI_HOST;

        sdspi_device_config_t dev = SDSPI_DEVICE_CONFIG_DEFAULT();
        dev.host_id = static_cast<spi_host_device_t>(BSP_SD_SPI_HOST);
        dev.gpio_cs = BSP_SD_PIN_CS;

        const esp_err_t err =
            esp_vfs_fat_sdspi_mount(kMount, &host, &dev, &mcfg, &card_);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "no card / mount failed (%s)", esp_err_to_name(err));
            card_ = nullptr;
            return false;
        }
        mounted_ = true;
        const double gb = card_ ? double(card_->csd.capacity) / (2048.0 * 1024.0) : 0.0;
        ESP_LOGI(TAG, "mounted %s  (%.1f GB)", kMount, gb);
        return true;
    }

    bool mounted() const override { return mounted_; }

    bool read_file(const std::string& path, std::vector<uint8_t>& out) override {
        FILE* f = std::fopen(full(path).c_str(), "rb");
        if (!f) return false;
        std::fseek(f, 0, SEEK_END);
        const long n = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        out.resize(n > 0 ? static_cast<size_t>(n) : 0);
        const size_t got = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
        std::fclose(f);
        out.resize(got);
        return true;
    }

    bool write_file(const std::string& path, const uint8_t* data, size_t len) override {
        FILE* f = std::fopen(full(path).c_str(), "wb");
        if (!f) return false;
        const size_t put = std::fwrite(data, 1, len, f);
        std::fclose(f);
        return put == len;
    }

    bool exists(const std::string& path) override {
        FILE* f = std::fopen(full(path).c_str(), "rb");
        if (!f) return false;
        std::fclose(f);
        return true;
    }

    bool list_dir(const std::string& path, std::vector<std::string>& names) override {
        DIR* d = ::opendir(full(path).c_str());
        if (!d) return false;
        while (dirent* e = ::readdir(d)) names.emplace_back(e->d_name);
        ::closedir(d);
        return true;
    }

    Reader* open_read(const std::string& path) override {
        FILE* f = std::fopen(full(path).c_str(), "rb");
        if (!f) return nullptr;
        return new FileReader(f);
    }

private:
    struct FileReader : hal::Storage::Reader {
        explicit FileReader(FILE* f) : f_(f) {}
        size_t read(uint8_t* buf, size_t n) override { return std::fread(buf, 1, n, f_); }
        void close() override {
            if (f_) std::fclose(f_);
            f_ = nullptr;
            delete this;
        }
        FILE* f_ = nullptr;
    };

    sdmmc_card_t* card_ = nullptr;
    bool          mounted_ = false;
};

EspStorage g_storage;
}  // namespace

hal::Storage* storage_instance() {
    static bool tried = false;
    if (!tried) {
        tried = true;
        g_storage.mount();   // ok to fail — mounted() stays false
    }
    return &g_storage;
}

}  // namespace hal::esp
