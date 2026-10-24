// ble_transport.cpp — NimBLE GATT peripheral.
//
// One primary service with five characteristics:
//   State     write-no-rsp   app -> device   CBOR DisplayState
//   Event     notify         device -> app   CBOR DeviceEvent
//   DeviceInfo read           app <- device   CBOR {proto, fw, caps, w, h}
//   Settings  read/write/ntf  bidirectional   CBOR Settings
//   Time      write           app -> device   CBOR {epochS, tzOffsetMin}
//
// The NimBLE host runs its own task; GATT callbacks fire there. poll()/emit()
// run on the app loop. The small shared state is guarded by g_mtx.
#include "firmware/components/transport_ble/ble_transport.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "mbedtls/sha256.h"
#include "nvs_flash.h"

#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_hs_mbuf.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "os/os_mbuf.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "core/codec/protocol.h"
#include "firmware/components/transport_ble/ble_uuids.h"
#include "hal/storage.h"

namespace transport::ble {

namespace {
const char* TAG = "ble";

// ── shared state (guarded) ────────────────────────────────────────────────────
SemaphoreHandle_t  g_mtx = nullptr;
core::DisplayState g_pending{};
bool               g_state_new = false;
uint32_t           g_last_seq = 0;
int64_t            g_last_rx_us = 0;
core::Settings     g_settings{};
bool               g_settings_new = false;
core::UiConfig     g_ui_config{};
bool               g_ui_config_new = false;

// ── config ───────────────────────────────────────────────────────────────────
uint32_t    g_caps = 0;
const char* g_fw = "0.0.0";
std::function<void(uint32_t, int16_t)> g_time_cb;

// ── link ─────────────────────────────────────────────────────────────────────
volatile transport::LinkStatus g_link = transport::LinkStatus::Disconnected;
uint16_t g_conn = BLE_HS_CONN_HANDLE_NONE;
uint16_t g_event_h = 0;
uint16_t g_settings_h = 0;
uint16_t g_ui_config_h = 0;
uint16_t g_ota_h = 0;
bool     g_event_sub = false;
bool     g_settings_sub = false;
bool     g_ui_config_sub = false;
bool     g_ota_sub = false;

// ── OTA receive state (touched only from the BLE host task) ──────────────────
enum : uint8_t {
    OTA_OP_BEGIN = 0x01, OTA_OP_DATA = 0x02, OTA_OP_END = 0x03, OTA_OP_ABORT = 0x04,
    OTA_NTF_PROGRESS = 0x10, OTA_NTF_OK = 0x11, OTA_NTF_ERR = 0x12,
};
bool                   g_ota_active = false;
esp_ota_handle_t       g_ota_handle = 0;
const esp_partition_t* g_ota_part = nullptr;
uint32_t               g_ota_total = 0;
uint32_t               g_ota_recv = 0;
uint32_t               g_ota_last_ntf = 0;
uint8_t                g_ota_percent = 0xFF;   // 0xFF == idle, else 0..100
mbedtls_sha256_context g_ota_sha;
uint8_t                g_ota_expect[32];
uint8_t*               g_ota_buf = nullptr;   // 4 KB, alloc'd for the transfer only
uint32_t               g_ota_buf_len = 0;
constexpr uint32_t     kOtaBufSize = 4096;

// ── asset (SD file) upload ──────────────────────────────────────────────────
enum : uint8_t { AST_OP_BEGIN = 0x01, AST_OP_DATA = 0x02, AST_OP_END = 0x03, AST_OP_ABORT = 0x04,
                 AST_NTF_OK = 0x11, AST_NTF_ERR = 0x12 };
hal::Storage*          g_storage = nullptr;
hal::Storage::Writer*  g_asset_w = nullptr;
uint32_t               g_asset_recv = 0;
uint16_t g_asset_h = 0;
bool     g_asset_sub = false;

uint8_t  g_own_addr_type = 0;
char     g_addr_str[18] = {0};

struct Lock {
    Lock() { if (g_mtx) xSemaphoreTake(g_mtx, portMAX_DELAY); }
    ~Lock() { if (g_mtx) xSemaphoreGive(g_mtx); }
};

// ── UUIDs ────────────────────────────────────────────────────────────────────
const ble_uuid128_t k_svc      = {{BLE_UUID_TYPE_128}, BDSP_BLE_UUID128(BDSP_BLE_SLOT_SERVICE)};
const ble_uuid128_t k_state    = {{BLE_UUID_TYPE_128}, BDSP_BLE_UUID128(BDSP_BLE_SLOT_STATE)};
const ble_uuid128_t k_event    = {{BLE_UUID_TYPE_128}, BDSP_BLE_UUID128(BDSP_BLE_SLOT_EVENT)};
const ble_uuid128_t k_info     = {{BLE_UUID_TYPE_128}, BDSP_BLE_UUID128(BDSP_BLE_SLOT_DEVICE_INFO)};
const ble_uuid128_t k_settings = {{BLE_UUID_TYPE_128}, BDSP_BLE_UUID128(BDSP_BLE_SLOT_SETTINGS)};
const ble_uuid128_t k_time     = {{BLE_UUID_TYPE_128}, BDSP_BLE_UUID128(BDSP_BLE_SLOT_TIME)};
const ble_uuid128_t k_ui_config = {{BLE_UUID_TYPE_128}, BDSP_BLE_UUID128(BDSP_BLE_SLOT_UI_CONFIG)};
const ble_uuid128_t k_ota = {{BLE_UUID_TYPE_128}, BDSP_BLE_UUID128(BDSP_BLE_SLOT_OTA)};
const ble_uuid128_t k_asset = {{BLE_UUID_TYPE_128}, BDSP_BLE_UUID128(BDSP_BLE_SLOT_ASSET)};

void start_advertising();

// ── OTA ──────────────────────────────────────────────────────────────────────
void ota_notify(uint8_t op, uint32_t arg) {
    if (g_conn == BLE_HS_CONN_HANDLE_NONE || !g_ota_sub) return;
    uint8_t p[5] = {op, uint8_t(arg), uint8_t(arg >> 8), uint8_t(arg >> 16), uint8_t(arg >> 24)};
    // progress carries the received byte count, an error carries its code (1 byte),
    // ok carries nothing — see docs/ble-protocol.md.
    const uint16_t len = (op == OTA_NTF_PROGRESS) ? 5 : (op == OTA_NTF_ERR ? 2 : 1);
    struct os_mbuf* om = ble_hs_mbuf_from_flat(p, len);
    if (om) ble_gatts_notify_custom(g_conn, g_ota_h, om);
}

void ota_reset(bool abort_flash) {
    if (g_ota_active && abort_flash && g_ota_handle) esp_ota_abort(g_ota_handle);
    if (g_ota_active) mbedtls_sha256_free(&g_ota_sha);
    if (g_ota_buf) { free(g_ota_buf); g_ota_buf = nullptr; }
    g_ota_active = false;
    g_ota_handle = 0;
    g_ota_part = nullptr;
    g_ota_total = g_ota_recv = g_ota_last_ntf = g_ota_buf_len = 0;
    g_ota_percent = 0xFF;
}

esp_err_t ota_flush() {   // push the RAM buffer to flash
    if (g_ota_buf_len == 0) return ESP_OK;
    const esp_err_t r = esp_ota_write(g_ota_handle, g_ota_buf, g_ota_buf_len);
    if (r == ESP_OK) {
        mbedtls_sha256_update(&g_ota_sha, g_ota_buf, g_ota_buf_len);
        g_ota_buf_len = 0;
    }
    return r;
}

int ota_handle_write(const uint8_t* d, uint16_t n) {
    if (n < 1) return BLE_ATT_ERR_INVALID_PDU;
    const uint8_t op = d[0];

    if (op == OTA_OP_BEGIN) {
        if (n < 1 + 4 + 32) return BLE_ATT_ERR_INVALID_PDU;
        ota_reset(true);
        g_ota_part = esp_ota_get_next_update_partition(nullptr);
        if (!g_ota_part) { ota_notify(OTA_NTF_ERR, 1); return 0; }
        g_ota_total = d[1] | (d[2] << 8) | (d[3] << 16) | (uint32_t(d[4]) << 24);
        std::memcpy(g_ota_expect, d + 5, 32);
        g_ota_buf = static_cast<uint8_t*>(malloc(kOtaBufSize));
        if (!g_ota_buf) { ota_notify(OTA_NTF_ERR, 2); return 0; }
        if (esp_ota_begin(g_ota_part, g_ota_total, &g_ota_handle) != ESP_OK) {
            free(g_ota_buf);
            g_ota_buf = nullptr;
            ota_notify(OTA_NTF_ERR, 2);
            return 0;
        }
        mbedtls_sha256_init(&g_ota_sha);
        mbedtls_sha256_starts(&g_ota_sha, 0);
        g_ota_active = true;
        g_ota_recv = g_ota_last_ntf = g_ota_buf_len = 0;
        g_ota_percent = 0;
        ESP_LOGW(TAG, "OTA begin: %lu bytes -> %s", (unsigned long)g_ota_total, g_ota_part->label);
        ota_notify(OTA_NTF_PROGRESS, 0);
        return 0;
    }

    if (!g_ota_active) return 0;   // stray frame, ignore

    if (op == OTA_OP_DATA) {
        const uint8_t* chunk = d + 1;
        uint16_t left = n - 1;
        while (left > 0) {
            const uint32_t space = kOtaBufSize - g_ota_buf_len;
            const uint32_t take = left < space ? left : space;
            std::memcpy(g_ota_buf + g_ota_buf_len, chunk, take);
            g_ota_buf_len += take;
            chunk += take;
            left -= take;
            if (g_ota_buf_len == kOtaBufSize && ota_flush() != ESP_OK) {
                ESP_LOGE(TAG, "OTA write failed");
                ota_reset(true);
                ota_notify(OTA_NTF_ERR, 3);
                return 0;
            }
        }
        g_ota_recv += n - 1;
        if (g_ota_total) g_ota_percent = uint8_t(uint64_t(g_ota_recv) * 100 / g_ota_total);
        if (g_ota_recv - g_ota_last_ntf >= 32768) {
            g_ota_last_ntf = g_ota_recv;
            ota_notify(OTA_NTF_PROGRESS, g_ota_recv);
        }
        return 0;
    }

    if (op == OTA_OP_END) {
        if (ota_flush() != ESP_OK) { ota_reset(true); ota_notify(OTA_NTF_ERR, 3); return 0; }
        uint8_t got[32];
        mbedtls_sha256_finish(&g_ota_sha, got);
        if (std::memcmp(got, g_ota_expect, 32) != 0) {
            ESP_LOGE(TAG, "OTA sha mismatch");
            ota_reset(true);
            ota_notify(OTA_NTF_ERR, 4);
            return 0;
        }
        if (esp_ota_end(g_ota_handle) != ESP_OK ||
            esp_ota_set_boot_partition(g_ota_part) != ESP_OK) {
            ota_reset(false);
            ota_notify(OTA_NTF_ERR, 5);
            return 0;
        }
        mbedtls_sha256_free(&g_ota_sha);
        g_ota_active = false;
        g_ota_percent = 100;
        ESP_LOGW(TAG, "OTA complete, rebooting into %s", g_ota_part->label);
        ota_notify(OTA_NTF_OK, 0);
        static esp_timer_handle_t t = nullptr;
        const esp_timer_create_args_t a = {
            .callback = [](void*) { esp_restart(); }, .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK, .name = "ota_reboot", .skip_unhandled_events = false};
        esp_timer_create(&a, &t);
        esp_timer_start_once(t, 600 * 1000);
        return 0;
    }

    if (op == OTA_OP_ABORT) {
        ESP_LOGW(TAG, "OTA aborted by app");
        ota_reset(true);
        return 0;
    }
    return 0;
}

// ── asset upload: write a file straight to the SD card ───────────────────────
void asset_notify(uint8_t op) {
    if (g_conn == BLE_HS_CONN_HANDLE_NONE || !g_asset_sub) return;
    struct os_mbuf* om = ble_hs_mbuf_from_flat(&op, 1);
    if (om) ble_gatts_notify_custom(g_conn, g_asset_h, om);
}

void asset_reset() {
    if (g_asset_w) { g_asset_w->close(); g_asset_w = nullptr; }
    g_asset_recv = 0;
}

int asset_handle_write(const uint8_t* d, uint16_t n) {
    if (n < 1) return BLE_ATT_ERR_INVALID_PDU;
    const uint8_t op = d[0];

    if (op == AST_OP_BEGIN) {
        asset_reset();
        if (!g_storage || !g_storage->mounted()) { asset_notify(AST_NTF_ERR); return 0; }
        if (n < 2) return BLE_ATT_ERR_INVALID_PDU;
        const uint8_t plen = d[1];
        if (n < 2u + plen) return BLE_ATT_ERR_INVALID_PDU;
        std::string path(reinterpret_cast<const char*>(d + 2), plen);
        g_asset_w = g_storage->open_write(path);
        if (!g_asset_w) { ESP_LOGW(TAG, "asset: open %s failed", path.c_str()); asset_notify(AST_NTF_ERR); return 0; }
        ESP_LOGI(TAG, "asset: receiving %s", path.c_str());
        return 0;
    }

    if (op == AST_OP_DATA) {
        if (!g_asset_w) return 0;
        if (!g_asset_w->write(d + 1, n - 1)) {
            ESP_LOGE(TAG, "asset: write failed");
            asset_reset();
            asset_notify(AST_NTF_ERR);
            return 0;
        }
        g_asset_recv += n - 1;
        return 0;
    }

    if (op == AST_OP_END) {
        if (!g_asset_w) return 0;
        const bool ok = g_asset_w->close();
        g_asset_w = nullptr;
        ESP_LOGI(TAG, "asset: %lu bytes %s", (unsigned long)g_asset_recv, ok ? "ok" : "FLUSH FAIL");
        g_asset_recv = 0;
        asset_notify(ok ? AST_NTF_OK : AST_NTF_ERR);
        return 0;
    }

    if (op == AST_OP_ABORT) { asset_reset(); return 0; }
    return 0;
}

// ── GATT access ──────────────────────────────────────────────────────────────
int gatt_access(uint16_t /*conn*/, uint16_t /*attr*/, ble_gatt_access_ctxt* ctxt, void* arg) {
    const int slot = static_cast<int>(reinterpret_cast<intptr_t>(arg));
    uint8_t buf[core::proto::kMaxPayload];

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_WRITE_CHR: {
            uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
            if (len > sizeof(buf)) len = sizeof(buf);
            uint16_t got = 0;
            if (os_mbuf_copydata(ctxt->om, 0, len, buf) != 0) return BLE_ATT_ERR_UNLIKELY;
            got = len;

            if (slot == BDSP_BLE_SLOT_STATE) {
                core::DisplayState s;
                if (!core::proto::decode_state(buf, got, s)) return BLE_ATT_ERR_INVALID_PDU;
                Lock lk;
                if (s.stateSeq != 0 && g_last_seq != 0 && s.stateSeq <= g_last_seq) return 0;
                g_last_seq = s.stateSeq;
                g_pending = s;
                g_state_new = true;
                g_last_rx_us = esp_timer_get_time();
                return 0;
            }
            if (slot == BDSP_BLE_SLOT_SETTINGS) {
                core::Settings s;
                if (!core::proto::decode_settings(buf, got, s)) return BLE_ATT_ERR_INVALID_PDU;
                Lock lk;
                g_settings = s;
                g_settings_new = true;
                return 0;
            }
            if (slot == BDSP_BLE_SLOT_UI_CONFIG) {
                core::UiConfig c;
                if (!core::proto::decode_ui_config(buf, got, c)) return BLE_ATT_ERR_INVALID_PDU;
                Lock lk;
                g_ui_config = c;
                g_ui_config_new = true;
                return 0;
            }
            if (slot == BDSP_BLE_SLOT_TIME) {
                core::proto::TimeSync t;
                if (!core::proto::decode_time(buf, got, t)) return BLE_ATT_ERR_INVALID_PDU;
                if (g_time_cb) g_time_cb(t.epochS, t.tzOffsetMin);
                return 0;
            }
            if (slot == BDSP_BLE_SLOT_OTA) {
                return ota_handle_write(buf, got);   // raw framing, not CBOR
            }
            if (slot == BDSP_BLE_SLOT_ASSET) {
                return asset_handle_write(buf, got);
            }
            return BLE_ATT_ERR_WRITE_NOT_PERMITTED;
        }

        case BLE_GATT_ACCESS_OP_READ_CHR: {
            size_t n = 0;
            if (slot == BDSP_BLE_SLOT_DEVICE_INFO) {
                core::proto::DeviceInfo d;
                d.fwVersion = g_fw;
                d.caps = g_caps;
                d.width = 466;
                d.height = 466;
                n = core::proto::encode_device_info(d, buf, sizeof(buf));
            } else if (slot == BDSP_BLE_SLOT_SETTINGS) {
                Lock lk;
                n = core::proto::encode_settings(g_settings, buf, sizeof(buf));
            } else if (slot == BDSP_BLE_SLOT_UI_CONFIG) {
                Lock lk;
                n = core::proto::encode_ui_config(g_ui_config, buf, sizeof(buf));
            } else {
                return BLE_ATT_ERR_READ_NOT_PERMITTED;
            }
            if (n == 0) return BLE_ATT_ERR_UNLIKELY;
            return os_mbuf_append(ctxt->om, buf, n) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }

        default:
            return BLE_ATT_ERR_UNLIKELY;
    }
}

const struct ble_gatt_chr_def k_chars[] = {
    {
        .uuid = &k_state.u,
        .access_cb = gatt_access,
        .arg = reinterpret_cast<void*>(BDSP_BLE_SLOT_STATE),
        .descriptors = nullptr,
        .flags = BLE_GATT_CHR_F_WRITE_NO_RSP,
        .min_key_size = 0,
        .val_handle = nullptr,
        .cpfd = nullptr,
    },
    {
        .uuid = &k_event.u,
        .access_cb = gatt_access,
        .arg = reinterpret_cast<void*>(BDSP_BLE_SLOT_EVENT),
        .descriptors = nullptr,
        .flags = BLE_GATT_CHR_F_NOTIFY,
        .min_key_size = 0,
        .val_handle = &g_event_h,
        .cpfd = nullptr,
    },
    {
        .uuid = &k_info.u,
        .access_cb = gatt_access,
        .arg = reinterpret_cast<void*>(BDSP_BLE_SLOT_DEVICE_INFO),
        .descriptors = nullptr,
        .flags = BLE_GATT_CHR_F_READ,
        .min_key_size = 0,
        .val_handle = nullptr,
        .cpfd = nullptr,
    },
    {
        .uuid = &k_settings.u,
        .access_cb = gatt_access,
        .arg = reinterpret_cast<void*>(BDSP_BLE_SLOT_SETTINGS),
        .descriptors = nullptr,
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
        .min_key_size = 0,
        .val_handle = &g_settings_h,
        .cpfd = nullptr,
    },
    {
        .uuid = &k_time.u,
        .access_cb = gatt_access,
        .arg = reinterpret_cast<void*>(BDSP_BLE_SLOT_TIME),
        .descriptors = nullptr,
        .flags = BLE_GATT_CHR_F_WRITE,
        .min_key_size = 0,
        .val_handle = nullptr,
        .cpfd = nullptr,
    },
    {
        .uuid = &k_ui_config.u,
        .access_cb = gatt_access,
        .arg = reinterpret_cast<void*>(BDSP_BLE_SLOT_UI_CONFIG),
        .descriptors = nullptr,
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
        .min_key_size = 0,
        .val_handle = &g_ui_config_h,
        .cpfd = nullptr,
    },
    {
        .uuid = &k_ota.u,
        .access_cb = gatt_access,
        .arg = reinterpret_cast<void*>(BDSP_BLE_SLOT_OTA),
        .descriptors = nullptr,
        .flags = BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_NOTIFY,
        .min_key_size = 0,
        .val_handle = &g_ota_h,
        .cpfd = nullptr,
    },
    {
        .uuid = &k_asset.u,
        .access_cb = gatt_access,
        .arg = reinterpret_cast<void*>(BDSP_BLE_SLOT_ASSET),
        .descriptors = nullptr,
        .flags = BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_NOTIFY,
        .min_key_size = 0,
        .val_handle = &g_asset_h,
        .cpfd = nullptr,
    },
    {0},
};

const struct ble_gatt_svc_def k_gatt[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &k_svc.u,
        .includes = nullptr,
        .characteristics = k_chars,
    },
    {0},
};

// ── GAP ──────────────────────────────────────────────────────────────────────
int gap_event(struct ble_gap_event* event, void* /*arg*/) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                g_conn = event->connect.conn_handle;
                g_link = transport::LinkStatus::Connected;
                {
                    Lock lk;
                    g_last_seq = 0;   // new session: accept the peer's seq from 1
                }
                ESP_LOGI(TAG, "connected");
            } else {
                start_advertising();
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "disconnected (0x%02x)", event->disconnect.reason);
            g_conn = BLE_HS_CONN_HANDLE_NONE;
            g_event_sub = g_settings_sub = g_ui_config_sub = g_ota_sub = g_asset_sub = false;
            if (g_ota_active) { ESP_LOGW(TAG, "OTA aborted (disconnect)"); ota_reset(true); }
            asset_reset();
            g_link = transport::LinkStatus::Advertising;
            {
                Lock lk;
                g_last_seq = 0;
            }
            start_advertising();
            return 0;

        case BLE_GAP_EVENT_SUBSCRIBE:
            if (event->subscribe.attr_handle == g_event_h)
                g_event_sub = event->subscribe.cur_notify;
            else if (event->subscribe.attr_handle == g_settings_h)
                g_settings_sub = event->subscribe.cur_notify;
            else if (event->subscribe.attr_handle == g_ui_config_h)
                g_ui_config_sub = event->subscribe.cur_notify;
            else if (event->subscribe.attr_handle == g_ota_h)
                g_ota_sub = event->subscribe.cur_notify;
            else if (event->subscribe.attr_handle == g_asset_h)
                g_asset_sub = event->subscribe.cur_notify;
            return 0;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            start_advertising();
            return 0;

        default:
            return 0;
    }
}

void start_advertising() {
    // ADV packet: flags + the 128-bit service UUID (21 B, fits in 31).
    struct ble_hs_adv_fields adv_fields{};
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    adv_fields.uuids128 = const_cast<ble_uuid128_t*>(&k_svc);
    adv_fields.num_uuids128 = 1;
    adv_fields.uuids128_is_complete = 1;
    int rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGW(TAG, "adv_set_fields rc=%d", rc);
        return;
    }

    // Scan response: the name.
    struct ble_hs_adv_fields rsp_fields{};
    rsp_fields.name = reinterpret_cast<const uint8_t*>("BlitzerdeDsp");
    rsp_fields.name_len = 12;
    rsp_fields.name_is_complete = 1;
    ble_gap_adv_rsp_set_fields(&rsp_fields);

    struct ble_gap_adv_params adv{};
    adv.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(g_own_addr_type, nullptr, BLE_HS_FOREVER, &adv, gap_event, nullptr);
    if (rc != 0) {
        ESP_LOGW(TAG, "adv_start rc=%d", rc);
        return;
    }
    g_link = transport::LinkStatus::Advertising;
}

void on_sync() {
    ble_hs_util_ensure_addr(0);
    if (ble_hs_id_infer_auto(0, &g_own_addr_type) != 0) {
        ESP_LOGE(TAG, "no BLE address");
        return;
    }
    uint8_t a[6];
    if (ble_hs_id_copy_addr(g_own_addr_type, a, nullptr) == 0) {
        std::snprintf(g_addr_str, sizeof(g_addr_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                      a[5], a[4], a[3], a[2], a[1], a[0]);   // NimBLE stores it LE
    }
    start_advertising();
    ESP_LOGI(TAG, "advertising as BlitzerdeDsp (%s)", g_addr_str);
}

void on_reset(int reason) { ESP_LOGW(TAG, "nimble reset %d", reason); }

void host_task(void* /*param*/) {
    nimble_port_run();          // returns on nimble_port_stop()
    nimble_port_freertos_deinit();
}

}  // namespace

// ── BleTransport ─────────────────────────────────────────────────────────────
void BleTransport::start() {
    if (!g_mtx) g_mtx = xSemaphoreCreateMutex();

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    if (nimble_port_init() != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init failed");
        return;
    }
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    if (ble_gatts_count_cfg(k_gatt) != 0 || ble_gatts_add_svcs(k_gatt) != 0) {
        ESP_LOGE(TAG, "gatt registration failed");
        return;
    }
    ble_svc_gap_device_name_set("BlitzerdeDsp");

    nimble_port_freertos_init(host_task);
    g_link = transport::LinkStatus::Advertising;
}

void BleTransport::stop() {
    nimble_port_stop();
    g_link = transport::LinkStatus::Disconnected;
    g_conn = BLE_HS_CONN_HANDLE_NONE;
}

transport::LinkStatus BleTransport::link() const { return g_link; }

bool BleTransport::poll(core::DisplayState& out) {
    Lock lk;
    if (!g_state_new) return false;
    g_state_new = false;
    out = g_pending;
    return true;
}

void BleTransport::emit(const core::DeviceEvent& e) {
    if (g_conn == BLE_HS_CONN_HANDLE_NONE || !g_event_sub) return;
    uint8_t buf[core::proto::kMaxPayload];
    const size_t n = core::proto::encode_event(e, buf, sizeof(buf));
    if (n == 0) return;
    struct os_mbuf* om = ble_hs_mbuf_from_flat(buf, n);
    if (!om) return;
    ble_gatts_notify_custom(g_conn, g_event_h, om);
}

void BleTransport::push_settings(const core::Settings& s) {
    {
        Lock lk;
        g_settings = s;
    }
    if (g_conn == BLE_HS_CONN_HANDLE_NONE || !g_settings_sub) return;
    uint8_t buf[core::proto::kMaxPayload];
    const size_t n = core::proto::encode_settings(s, buf, sizeof(buf));
    if (n == 0) return;
    struct os_mbuf* om = ble_hs_mbuf_from_flat(buf, n);
    if (!om) return;
    ble_gatts_notify_custom(g_conn, g_settings_h, om);
}

bool BleTransport::poll_settings(core::Settings& out) {
    Lock lk;
    if (!g_settings_new) return false;
    g_settings_new = false;
    out = g_settings;
    return true;
}

void BleTransport::push_ui_config(const core::UiConfig& c) {
    {
        Lock lk;
        g_ui_config = c;
    }
    if (g_conn == BLE_HS_CONN_HANDLE_NONE || !g_ui_config_sub) return;
    uint8_t buf[core::proto::kMaxPayload];
    const size_t n = core::proto::encode_ui_config(c, buf, sizeof(buf));
    if (n == 0) return;
    struct os_mbuf* om = ble_hs_mbuf_from_flat(buf, n);
    if (!om) return;
    ble_gatts_notify_custom(g_conn, g_ui_config_h, om);
}

bool BleTransport::poll_ui_config(core::UiConfig& out) {
    Lock lk;
    if (!g_ui_config_new) return false;
    g_ui_config_new = false;
    out = g_ui_config;
    return true;
}

bool BleTransport::state_stale() const {
    if (g_link != transport::LinkStatus::Connected) return false;
    const int64_t age_ms = (esp_timer_get_time() - g_last_rx_us) / 1000;
    return g_last_rx_us != 0 && age_ms > CONFIG_BDSP_STATE_WATCHDOG_S * 1000;
}

uint8_t BleTransport::ota_percent() const { return g_ota_percent; }
void BleTransport::set_storage(hal::Storage* s) { g_storage = s; }

void BleTransport::set_caps(uint32_t caps) { g_caps = caps; }
void BleTransport::set_fw_version(const char* v) { g_fw = v; }
const char* BleTransport::address_string() const { return g_addr_str; }
void BleTransport::on_time_set(std::function<void(uint32_t, int16_t)> cb) {
    g_time_cb = std::move(cb);
}

}  // namespace transport::ble
