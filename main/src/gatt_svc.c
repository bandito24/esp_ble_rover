/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Includes */
#include "gatt_svc.h"
#include "common.h"
#include "rover_cmd.h"

/* Private function declarations */
// static int heart_rate_chr_access(uint16_t conn_handle, uint16_t attr_handle,
//                                  struct ble_gatt_access_ctxt *ctxt, void *arg);
// static int led_chr_access(uint16_t conn_handle, uint16_t attr_handle,
//                           struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Private variables */
/* Wheel Steering service */
static const ble_uuid128_t rover_cmd_svc_uuid = BLE_UUID128_INIT(
    0x78, 0x56, 0x34, 0x12, 0xef, 0xcd, 0xab, 0x90,
    0x78, 0x56, 0x34, 0x12, 0xbe, 0xba, 0xfe, 0xca);

static uint8_t rover_cmd_chr_val = 0;
static uint16_t rover_cmd_val_handle;
static const ble_uuid16_t led_chr_uuid = BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x00);

// static uint16_t wheel_chr_conn_handle = 0;
// static bool wheel_chr_conn_handle_inited = false;
// static bool wheel_ind_status = false;

/* Automation IO service */
// static const ble_uuid16_t led_conn_svc_uuid = BLE_UUID16_INIT(0x1815);
// static uint16_t led_conn_chr_val_handle;
// static const ble_uuid128_t led_conn_chr_uuid =
//     BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
//                      0x12, 0x12, 0x25, 0x15, 0x00, 0x00);

/* GATT services table */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    /* Heart rate service */
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = &rover_cmd_svc_uuid.u,
     .characteristics =
         (struct ble_gatt_chr_def[]){
             {/* Heart rate characteristic */
              .uuid = &rover_cmd_svc_uuid.u,
              .access_cb = rover_cmd_chr_access,
              .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
              .val_handle = &rover_cmd_val_handle},
             {
                 0, /* No more characteristics in this service. */
             }}},


    {
        0, /* No more services. */
    },
};

/* Private functions */
static int rover_cmd_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {
 /* Local variables */
    int rc;
    return 0;

    /* Handle access events */
    /* Note: LED characteristic is write only */
    switch (ctxt->op) {

    /* Write characteristic event */
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic write; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG,
                     "characteristic write by nimble stack; attr_handle=%d",
                     attr_handle);
        }

        /* Verify attribute handle */
        if (attr_handle == rover_cmd_val_handle) {
            /* Verify access buffer length */
            if (ctxt->om->om_len == 1) {
                if (ctxt->om->om_data[0]) {
                   interpret_rover_command(ctxt->om->om_data[0]);
                }
            } else {
                goto error;
            }
            return rc;
        }
        goto error;

    /* Unknown event */
    default:
        goto error;
    }

error:
    ESP_LOGE(TAG,
             "unexpected access operation to led characteristic, opcode: %d",
             ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}


void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    /* Local variables */
    char buf[BLE_UUID_STR_LEN];

    /* Handle GATT attributes register events */
    switch (ctxt->op) {

    /* Service register event */
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(TAG, "registered service %s with handle=%d",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                 ctxt->svc.handle);
        break;

    /* Characteristic register event */
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(TAG,
                 "registering characteristic %s with "
                 "def_handle=%d val_handle=%d",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    /* Descriptor register event */
    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGD(TAG, "registering descriptor %s with handle=%d",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                 ctxt->dsc.handle);
        break;

    /* Unknown event */
    default:
        assert(0);
        break;
    }
}


void gatt_svr_subscribe_cb(struct ble_gap_event *event) {
    /* Check connection handle */
    if (event->subscribe.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d",
                 event->subscribe.conn_handle, event->subscribe.attr_handle);
    } else {
        ESP_LOGI(TAG, "subscribe by nimble stack; attr_handle=%d",
                 event->subscribe.attr_handle);
    }

    /* Check attribute handle */
    if (event->subscribe.attr_handle == heart_rate_chr_val_handle) {
        /* Update heart rate subscription status */
        heart_rate_chr_conn_handle = event->subscribe.conn_handle;
        heart_rate_chr_conn_handle_inited = true;
        heart_rate_ind_status = event->subscribe.cur_indicate;
    }
}

/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to server
 */
int gatt_svc_init(void) {
    /* Local variables */
    int rc;

    /* 1. GATT service initialization */
    ble_svc_gatt_init();

    /* 2. Update GATT services counter */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    /* 3. Add GATT services */
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
