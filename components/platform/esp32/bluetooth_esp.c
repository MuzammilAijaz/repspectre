#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

/* Native ESP-IDF NimBLE Stack Host Reference Includes */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "bluetooth_esp.h"
#include "BSP_esp.h"
#include "soc/soc_caps.h" // for SOC_BT_SUPPORTED

#include "esp_bt.h" // for esp_bt_controller_get_status()

#include "qpc.h"

Q_DEFINE_THIS_MODULE("BluetoothEsp")
enum {
    BLUETOOTH_ESP_SIG = QS_USER,
    BLUETOOTH_CALLBACK_TEST_SIG,
};

static void trace_bt(const char* msg) {
    QS_BEGIN_ID(BLUETOOTH_CALLBACK_TEST_SIG, 1U)
        QS_STR(msg);
    QS_END();
}

static uint8_t own_addr_type = 0;

// ---- Async BLE state flags ----------------------------------
// Tracks BLE stack state shared across callbacks and application
// code, since many controller/host operations complete asynchronously.
static bool is_initialized = false;
static bool is_advertising_active = false;
static bool is_on_sync_called = false;
// -------------------------------------------------------------

/* Shared Characteristic Local Data Store */
static char characteristic_value[32] = "INIT";
static uint16_t dead_char_val_handle;
static bool is_notifying = false;

/* Forward Declarations of Native Event Callbacks */
static int gatt_svr_access_dead_chr(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int gatt_svr_access_dead_dsc(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int ble_gap_event_handler(struct ble_gap_event *event, void *arg);
static void trace_peer(uint16_t conn_handle, const char* prefix);

/* Static Const UUID Handles matching Official Design Specifications */
static const ble_uuid16_t dead_svc_uuid   = BLE_UUID16_INIT(0xDEAD);
static const ble_uuid16_t beef_char_uuid  = BLE_UUID16_INIT(0xBEEF);
static const ble_uuid16_t desc_2904_uuid  = BLE_UUID16_INIT(0x2904);
static const ble_uuid16_t baad_svc_uuid   = BLE_UUID16_INIT(0xBAAD);

#define UUIDS_NUM 2
// global because the setting (of adv_params) part is in
// different scopes compared to the applying part.
const ble_uuid16_t uuids[UUIDS_NUM] = { // TODO: remove hardcoded UUIDs
    BLE_UUID16_INIT(0xDEAD),
    BLE_UUID16_INIT(0xBAAD)
};

/* Advertisement parameters */
struct ble_gap_adv_params adv_params;
struct ble_hs_adv_fields adv_fields;
#if CONFIG_BT_NIMBLE_GAP_SERVICE
const char *service_name;
#endif

/* Forward declarations */
static bool Bluetooth_start_advertising(void);

/* --- Official GATT Service Definition Array Table --- */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* Service: DEAD */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &dead_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) { {
            /* Characteristic: BEEF */
            .uuid = &beef_char_uuid.u,
            .access_cb = gatt_svr_access_dead_chr,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
            .val_handle = &dead_char_val_handle,
            .descriptors = (struct ble_gatt_dsc_def[]) { {
                /* Descriptor: Presentation Format (UTF-8) */
                .uuid = &desc_2904_uuid.u,
                .att_flags = BLE_ATT_F_READ,
                .access_cb = gatt_svr_access_dead_dsc,
            }, {
                0, /* Terminate nested descriptors array */
            } }
        }, {
            0, /* Terminate nested characteristics array */
        } },
    },
    {
        /* Service: BAAD (Empty Service Configuration Layout) */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &baad_svc_uuid.u,
        .characteristics = NULL,
    },
    {
        0, /* Terminate root service definition array table */
    }
};

/**
 * Robust Flat Write Handler matching Espressif's implementation.
 * Flattens chained mbuf memory pools safely to shield against packet fragmentation issues.
 */
static int gatt_svr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len, void *dst, uint16_t *len) {
    uint16_t om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    int rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }
    return 0;
}

static int gatt_svr_access_dead_chr(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int rc;

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            {
                if (!is_notifying) {
                    char msg[80];
                    snprintf(msg, sizeof(msg),
                            "Characteristic::onRead UUID=0xbeef Value=%s",
                            characteristic_value);
                    trace_bt(msg);
                }
                rc = os_mbuf_append(ctxt->om, characteristic_value, strlen(characteristic_value));
                return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
            }

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            {
                uint16_t flattened_len = 0;
                rc = gatt_svr_write(ctxt->om, 1, sizeof(characteristic_value) - 1, characteristic_value, &flattened_len);
                if (rc == 0) {
                    characteristic_value[flattened_len] = '\0';
                    char msg[80];
                    snprintf(msg, sizeof(msg),
                            "Characteristic::onWrite UUID=0xbeef Value=%s",
                            characteristic_value);
                    trace_bt(msg);
                }
                return rc;
            }
        default:
            return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
    }
}

static int gatt_svr_access_dead_dsc(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_DSC) {
        uint8_t format_utf8 = 0x19; /* Presentation format code layout: UTF-8 standard string */
        int rc = os_mbuf_append(ctxt->om, &format_utf8, sizeof(format_utf8));
        return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
}

static void nimble_host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void on_stack_reset(int reason) {
    /* Safe hook point to capture spontaneous controller resets */
}

/* private wrapper function to actually apply the ble_hs_adv_fields for advertising */
static bool apply_adv_fields(void) {
    int rc = ble_gap_adv_set_fields(&adv_fields);

    Q_ASSERT(rc != BLE_HS_ENOTSYNCED);
    Q_ASSERT(rc == 0);

    return (rc == 0);
}

static void on_stack_sync(void) {
    int rc = ble_hs_util_ensure_addr(0);
    Q_ASSERT(rc == 0);
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    Q_ASSERT(rc == 0);

    // setup advertisement here.
    bool set = apply_adv_fields();
    Q_ASSERT(set);

    is_on_sync_called = true;

    // bool advertising_started = Bluetooth_start_advertising();
    // Q_ASSERT(advertising_started);
}

static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    /* Registration lifecycle verification callback pointer */
}

static bool Bluetooth_init_adapter(BluetoothConfig config) {
    if (is_initialized) return true;

#ifdef CONFIG_IDF_TARGET_ESP32
    /* Release Classic BT memory region to reclaim heap RAM since we only use BLE */
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
#endif

    /* Clean execution: The Bluetooth module now only cares about its own stack initialization */
    int rc = nimble_port_init();
    if (rc != ESP_OK) return false;

    // ---- Setup Callbacks ----------------------------------------
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    // ---- Configure ----------------------------------------------
#if CONFIG_BT_NIMBLE_GAP_SERVICE
    /* Set the default device name. */
    rc = ble_svc_gap_device_name_set(config.device_name);
    if (rc != 0) return false;
#endif
    rc = ble_att_set_preferred_mtu(config.mtu);
    if (rc != 0) return false;

    is_initialized = true;
    return true;
}

/**
 * @brief Registers GATT services and configures BLE advertisement data.
 *
 * Initializes the local GATT database, prepares advertisement payload fields,
 * and starts the NimBLE host task required for BLE stack execution.
 *
 * @return true if the profile was configured successfully, false otherwise.
 */
static bool Bluetooth_setup_profile(void) {
    if (!is_initialized) return false;

    int rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) return false;

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) return false;

    // ---- Set adv fields -----------------------------------------
    /* NOTE: this not actually apply the fields, which requires ble_gap_adv_set_fields()
     * @see apply_adv_fields(), which is first going to be called in on_stack_sync()
     */

    /* Build and pre-populate GAP Advertisement properties payload */
    memset(&adv_fields, 0, sizeof(adv_fields));

    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    // TODO: add explanation
#if CONFIG_BT_NIMBLE_GAP_SERVICE
    /* Fetch current active configured profile runtime device identity name */
    const char *name = ble_svc_gap_device_name();
    adv_fields.name = (uint8_t *)name;
    adv_fields.name_len = strlen(name);
    adv_fields.name_is_complete = 1;
#endif

    /* Automatically inject TX Power Configuration Field requested by the official design pattern */
    adv_fields.tx_pwr_lvl_is_present = 1;
    adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    adv_fields.uuids16 = uuids;
    adv_fields.num_uuids16 = UUIDS_NUM;
    adv_fields.uuids16_is_complete = 1;

    // -------------------------------------------------------------

    /* Initialize FreeRTOS Task */
    nimble_port_freertos_init(nimble_host_task);

    // Wait until sync with the controller.
    while (!is_on_sync_called) {
        // tick length depends on CONFIG_FREERTOS_HZ setting
        ble_npl_time_delay(ble_npl_time_ms_to_ticks32(3));
    }

    return true;
}

static bool Bluetooth_start_advertising(void) {
    if (!is_initialized) return false;

    if (!is_on_sync_called) {
        QS_BEGIN_ID(BLUETOOTH_ESP_SIG, 0U)
            QS_STR("Not Synced with controller");
        QS_END()
            return false;
    }
    Q_ASSERT(is_on_sync_called);

    bool adv_field_applied = apply_adv_fields();
    Q_ASSERT(adv_field_applied);
    if (!adv_field_applied) return false;

    // struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    int rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event_handler, NULL);
    Q_ASSERT(rc == 0);
    if (rc == 0) {
        is_advertising_active = true;
        return true;
    }
    return false;
}

static bool Bluetooth_stop_advertising(void) {
    int rc = ble_gap_adv_stop();
    if (rc == 0 || rc == BLE_HS_EALREADY) {
        is_advertising_active = false;
        return true;
    }
    return false;
}

static bool Bluetooth_is_advertising(void) {
    return is_advertising_active;
}

static bool Bluetooth_set_preferred_mtu(uint16_t mtu) {
    if (mtu > 512 || mtu < 23) return false;

    int rc = ble_att_set_preferred_mtu(mtu);
    if (rc != 0) {
        trace_bt("Count not set local mtu value");
    }
    Q_ASSERT(rc == 0);

    return rc == 0;
}

static bool Bluetooth_set_value(SensorData data) {
    snprintf(characteristic_value, sizeof(characteristic_value),
             "A:%.1f,%.1f,%.1f G:%.1f,%.1f,%.1f",
             data.accel.x, data.accel.y, data.accel.z,
             data.gyro.x, data.gyro.y, data.gyro.z);
    return true;
}

static bool Bluetooth_notify(SensorData data) {
    if (!is_initialized) return false;

    Bluetooth_set_value(data);

    /* Trigger the notification.
     *
     * ble_gatts_chr_updated() looks up the characteristic handle and notifies all
     * connected peers that have subscribed to this characteristic.
     *
     * Note: NimBLE calls the gatt_svr_access_dead_chr to get data from `characteristic_value`
     */
    is_notifying = true;
    ble_gatts_chr_updated(dead_char_val_handle);
    return true;
}

static void Bluetooth_get_address(char* out_str, size_t max_len) {
    uint8_t id_addr[6] = {0};
    int rc = ble_hs_id_copy_addr(own_addr_type, id_addr, NULL);
    if (rc == 0) {
        snprintf(out_str, max_len, "%02X:%02X:%02X:%02X:%02X:%02X",
                id_addr[5], id_addr[4], id_addr[3], id_addr[2], id_addr[1], id_addr[0]);
    } else {
        snprintf(out_str, max_len, "00:00:00:00:00:00");
    }
}

static void Bluetooth_run_callback_test(void) {
    /* Explicitly simulate characteristic state updating events via internal host driver loop */
    ble_gatts_chr_updated(dead_char_val_handle);
}

static void trace_peer(uint16_t conn_handle, const char* prefix) {
    struct ble_gap_conn_desc desc;
    int rc = ble_gap_conn_find(conn_handle, &desc);
    if (rc == 0) {
        const uint8_t *a = desc.peer_id_addr.val;
        char msg[128];
        snprintf(msg, sizeof(msg),
                "%s%02x:%02x:%02x:%02x:%02x:%02x",
                prefix,
                a[5], a[4], a[3],
                a[2], a[1], a[0]);
        trace_bt(msg);
    }
}

static void trace_subscribe(uint16_t conn_handle, uint8_t subValue) {
    trace_peer(conn_handle, "Client ID: 1 Address: ");

    if (subValue == 0) {
        trace_bt(" Unsubscribed to 0xbeef");
    } else if (subValue == 1) {
        trace_bt(" Subscribed to notifications for 0xbeef");
    } else if (subValue == 2) {
        trace_bt(" Subscribed to indications for 0xbeef");
    } else if (subValue == 3) {
        trace_bt(" Subscribed to notifications and indications for 0xbeef");
    }
}

static int ble_gap_event_handler(struct ble_gap_event *event, void *arg) {
    struct ble_gap_adv_params adv_params;

    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                is_advertising_active = false;
                trace_peer(event->connect.conn_handle,
                        "ServerCallbacks::onConnect - Client connected: ");
                ble_gap_update_params(event->connect.conn_handle,
                        &(struct ble_gap_upd_params) {
                        .itvl_min = 24,
                        .itvl_max = 48,
                        .latency = 0,
                        .supervision_timeout = 180,
                        });
            }
            if (event->connect.status != 0 && is_advertising_active) {
                /* Connection attempt aborted or failed; self-heal and auto-resume advertising */
                Bluetooth_start_advertising();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            /* CRITICAL RESILIENCE: Automated Self-Healing. Re-advertise instantly on client disconnection */
            trace_bt("ServerCallbacks::onDisconnect - Client disconnected, start advertising");
            if (!is_advertising_active) {
                Bluetooth_start_advertising();
            }
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            /* Safe hook point if advertisement completes or runs out of execution allocation scope */
            break;

        case BLE_GAP_EVENT_MTU:
            {
                char msg[80];
                snprintf(msg, sizeof(msg),
                        "ServerCallbacks::onMTUChange - MTU=%u ConnID=%u",
                        event->mtu.value,
                        event->mtu.conn_handle);
                trace_bt(msg);
                break;
            }

        case BLE_GAP_EVENT_SUBSCRIBE:
            {
                if (event->subscribe.attr_handle == dead_char_val_handle) {
                    uint8_t subValue = 0;
                    if (event->subscribe.cur_notify) subValue |= 1;
                    if (event->subscribe.cur_indicate) subValue |= 2;
                    trace_subscribe(event->subscribe.conn_handle, subValue);
                }
                break;
            }

        case BLE_GAP_EVENT_NOTIFY_TX:
            {
                if (event->notify_tx.attr_handle == dead_char_val_handle) {
                    char msg[80];
                    is_notifying = false;
                    snprintf(msg, sizeof(msg),
                            "Characteristic::onStatus code=%d (%d)",
                            event->notify_tx.status,
                            event->notify_tx.status);
                    trace_bt(msg);
                }
                break;
            }
    }
    return 0;
}

/* Immutable Pure C Mapping Structure Instance */
BluetoothInterface espBluetoothInterface = {
    .init              = Bluetooth_init_adapter,
    .setup_profile     = Bluetooth_setup_profile,
    .start_advertising = Bluetooth_start_advertising,
    .stop_advertising  = Bluetooth_stop_advertising,
    .is_advertising    = Bluetooth_is_advertising,
    .set_preferred_mtu = Bluetooth_set_preferred_mtu,
    .set_value         = Bluetooth_set_value,
    .notify            = Bluetooth_notify,
    .get_address       = Bluetooth_get_address,
    .run_callback_test = Bluetooth_run_callback_test,
};
