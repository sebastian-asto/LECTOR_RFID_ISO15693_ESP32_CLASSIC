#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/ble_hs_adv.h"
#include "host/ble_store.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "services/gap/ble_svc_gap.h"

#include "ble_advertiser.h"

#define BLE_DEVICE_NAME_IDLE "RFID SIN TAG"
#define BLE_UID_LEN          8

static const char *TAG = "BLE_ADV";

static uint8_t own_addr_type;
static bool ble_synced;
static bool has_uid;
static uint8_t current_uid[BLE_UID_LEN];
static char scan_name[sizeof("RFID ") + (BLE_UID_LEN * 2)];

void ble_store_config_init(void);

static void set_idle_name(void)
{
    snprintf(scan_name, sizeof(scan_name), "%s", BLE_DEVICE_NAME_IDLE);
}

static void uid_to_hex_name(const uint8_t *uid)
{
    int offset = snprintf(scan_name, sizeof(scan_name), "RFID ");

    for (size_t i = 0; i < BLE_UID_LEN && offset > 0; i++) {
        offset += snprintf(&scan_name[offset],
                           sizeof(scan_name) - (size_t)offset,
                           "%02X",
                           uid[i]);
    }
}

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_ADV_COMPLETE:
        if (ble_synced) {
            ble_advertiser_set_uid(has_uid ? current_uid : NULL,
                                   has_uid ? BLE_UID_LEN : 0);
        }
        break;

    default:
        break;
    }

    return 0;
}

static esp_err_t ble_advertiser_start(void)
{
    int rc;
    uint8_t mfg_data[2 + BLE_UID_LEN] = {0xFF, 0xFF};
    struct ble_hs_adv_fields adv_fields = {0};
    struct ble_hs_adv_fields rsp_fields = {0};
    struct ble_gap_adv_params adv_params = {0};

    if (!ble_synced) {
        return ESP_ERR_INVALID_STATE;
    }

    if (ble_gap_adv_active()) {
        rc = ble_gap_adv_stop();
        if (rc != 0) {
            ESP_LOGW(TAG, "No se pudo detener advertising anterior, rc=%d", rc);
        }
    }

    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    adv_fields.tx_pwr_lvl_is_present = 1;
    adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    if (has_uid) {
        memcpy(&mfg_data[2], current_uid, BLE_UID_LEN);
        adv_fields.mfg_data = mfg_data;
        adv_fields.mfg_data_len = sizeof(mfg_data);
    }

    rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error configurando advertising data, rc=%d", rc);
        return ESP_FAIL;
    }

    rsp_fields.name = (uint8_t *)scan_name;
    rsp_fields.name_len = strlen(scan_name);
    rsp_fields.name_is_complete = 1;
    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error configurando scan response, rc=%d", rc);
        return ESP_FAIL;
    }

    adv_params.conn_mode = BLE_GAP_CONN_MODE_NON;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = BLE_GAP_ADV_ITVL_MS(300);
    adv_params.itvl_max = BLE_GAP_ADV_ITVL_MS(320);

    rc = ble_gap_adv_start(own_addr_type,
                           NULL,
                           BLE_HS_FOREVER,
                           &adv_params,
                           ble_gap_event,
                           NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error iniciando advertising BLE, rc=%d", rc);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "BLE anunciando: %s", scan_name);
    return ESP_OK;
}

static void ble_on_reset(int reason)
{
    ble_synced = false;
    ESP_LOGW(TAG, "NimBLE reset, reason=%d", reason);
}

static void ble_on_sync(void)
{
    int rc = ble_hs_id_infer_auto(0, &own_addr_type);

    if (rc != 0) {
        ESP_LOGE(TAG, "No se pudo inferir direccion BLE, rc=%d", rc);
        return;
    }

    ble_synced = true;
    ble_advertiser_start();
}

static void nimble_host_task(void *param)
{
    ESP_LOGI(TAG, "NimBLE host iniciado");
    nimble_port_run();
    vTaskDelete(NULL);
}

esp_err_t ble_advertiser_init(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo inicializar NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    set_idle_name();

    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo inicializar NimBLE: %s", esp_err_to_name(ret));
        return ret;
    }

    ble_svc_gap_init();
    ble_svc_gap_device_name_set("RFID-ISO15693");

    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    ble_store_config_init();

    BaseType_t ok = xTaskCreate(nimble_host_task,
                                "nimble_host",
                                4096,
                                NULL,
                                5,
                                NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "No se pudo crear tarea NimBLE");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void ble_advertiser_set_uid(const uint8_t *uid, size_t uid_len)
{
    if (uid == NULL || uid_len != BLE_UID_LEN) {
        ble_advertiser_set_no_tag();
        return;
    }

    if (has_uid && memcmp(current_uid, uid, BLE_UID_LEN) == 0) {
        return;
    }

    memcpy(current_uid, uid, BLE_UID_LEN);
    has_uid = true;
    uid_to_hex_name(current_uid);
    ble_advertiser_start();
}

void ble_advertiser_set_no_tag(void)
{
    if (!has_uid && strcmp(scan_name, BLE_DEVICE_NAME_IDLE) == 0) {
        return;
    }

    has_uid = false;
    memset(current_uid, 0, sizeof(current_uid));
    set_idle_name();
    ble_advertiser_start();
}
