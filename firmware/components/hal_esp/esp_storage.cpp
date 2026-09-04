// esp_storage.cpp — microSD via FATFS over SPI. The card sits on the SPI2 bus
// that esp_display already brought up, so this only adds the SD *device* (its own
// CS on GPIO15). The panel flush and every card access take hal::esp::spi2_bus_*
// so the two never overlap (the SPI HAL asserts if they do). Card ops block the
// display briefly — fine, they're rare.
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>

#include "board_config.h"
#include "driver/sdspi_host.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/storage.h"
#include "hal_esp.h"
#include "sdmmc_cmd.h"

namespace hal::esp {

namespace {
const char* TAG = "sd";
constexpr const char* kMount = BSP_SD_MOUNT_POINT;

// SPI2 is shared with the panel — hold the bus lock around every card access.
struct BusGuard {
    BusGuard() { spi2_bus_lock(); }
    ~BusGuard() { spi2_bus_unlock(); }
};

std::string full(const std::string& path) {
    if (!path.empty() && path[0] == '/') return std::string(kMount) + path;
    return std::string(kMount) + "/" + path;
}

class EspStorage final : public hal::Storage {
public:
    bool mount() {
        BusGuard g;
        esp_vfs_fat_sdmmc_mount_config_t mcfg = {};
        mcfg.format_if_mount_failed = false;
        mcfg.max_files = 8;
        mcfg.allocation_unit_size = 16 * 1024;   // fewer, larger writes -> faster uploads

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
        BusGuard g;
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
        BusGuard g;
        FILE* f = std::fopen(full(path).c_str(), "wb");
        if (!f) return false;
        const size_t put = std::fwrite(data, 1, len, f);
        std::fclose(f);
        return put == len;
    }

    bool exists(const std::string& path) override {
        BusGuard g;
        FILE* f = std::fopen(full(path).c_str(), "rb");
        if (!f) return false;
        std::fclose(f);
        return true;
    }

    bool list_dir(const std::string& path, std::vector<std::string>& names) override {
        BusGuard g;
        DIR* d = ::opendir(full(path).c_str());
        if (!d) return false;
        while (dirent* e = ::readdir(d)) names.emplace_back(e->d_name);
        ::closedir(d);
        return true;
    }

    Reader* open_read(const std::string& path) override {
        BusGuard g;
        FILE* f = std::fopen(full(path).c_str(), "rb");
        if (!f) return nullptr;
        return new FileReader(f);
    }

    Writer* open_write(const std::string& path) override {
        const std::string p = full(path);
        {
            BusGuard g;
            for (size_t i = 1; (i = p.find('/', i)) != std::string::npos; ++i) {
                ::mkdir(p.substr(0, i).c_str(), 0777);   // FATFS ignores the mode
            }
        }
        for (int attempt = 0; attempt < 4; ++attempt) {
            BusGuard g;
            if (FILE* f = std::fopen(p.c_str(), "wb")) return new FileWriter(f);
            vTaskDelay(pdMS_TO_TICKS(30));   // fd table may still be settling
        }
        ESP_LOGW(TAG, "open_write %s failed", p.c_str());
        return nullptr;
    }

private:
    struct FileReader : hal::Storage::Reader {
        explicit FileReader(FILE* f) : f_(f) {}
        size_t read(uint8_t* buf, size_t n) override { BusGuard g; return std::fread(buf, 1, n, f_); }
        void close() override {
            BusGuard g;
            if (f_) std::fclose(f_);
            f_ = nullptr;
            delete this;
        }
        FILE* f_ = nullptr;
    };

    struct FileWriter : hal::Storage::Writer {
        explicit FileWriter(FILE* f) : f_(f) {}
        bool write(const uint8_t* buf, size_t n) override {
            BusGuard g;
            return f_ && std::fwrite(buf, 1, n, f_) == n;
        }
        bool close() override {
            BusGuard g;
            const bool ok = f_ && std::fclose(f_) == 0;
            f_ = nullptr;
            delete this;
            return ok;
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
