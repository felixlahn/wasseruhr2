#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "esp_log.h"
//#include "bluetooth.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "waterflow_counter.h"

// should be in own bluetooth.h-file
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

void init_bluetooth();
typedef struct resetEvent ResetEvent;
typedef void (*ResetListener)(ResetEvent event);
void registerResetListener(ResetListener listener);
void setGATTReadValue(int value);

struct resetEvent {
	uint64_t systemtime;
};

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

static const char* TAG = "main";

int milliliterCounter = 0;

void resetListener(ResetEvent event) {
	ESP_LOGI(TAG, "received ResetEvent with systemtime %lld", event.systemtime);
	milliliterCounter = 0;
	setGATTReadValue(milliliterCounter);
}

void oneWheelRevolutionEventListener(OneWheelRevolutionEvent event) {
	ESP_LOGI(TAG, "received OneWheelRevolutionEvent with systemtime %lld", event.systemtime);
}

void tenMilliliterFlownListener(TenMilliliterFlownEvent event) {
	ESP_LOGI(TAG, "received TenMilliliterFlownEvent with systemtime %lld", event.systemtime);
	milliliterCounter += 10;
	setGATTReadValue(milliliterCounter);
}

void app_main(void)
{
	setGATTReadValue(milliliterCounter);
	init_bluetooth();
	init_waterflowCounter();
	registerRevolutionEventListener(oneWheelRevolutionEventListener);
	registerTenMilliliterEventListener(tenMilliliterFlownListener);
	registerResetListener(resetListener);

    while (true) {
//        printf("Hello from app_main!\n");
        sleep(1);
    }
}

// should be in own bluetooth.c-file
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

static ResetListener gResetListener = NULL;

int GATTReadValue = 0;

void setGATTReadValue(int value) {
	GATTReadValue = value;
}

uint8_t ble_addr_type;
void ble_app_advertise(void);

void notifyResetListener() {
	if (gResetListener != NULL) {
		ResetEvent event;
		event.systemtime = esp_timer_get_time();

		resetListener(event);
		ESP_LOGI(TAG, "sent ResetEvent to listener");
	}
}

// Write data to ESP32 defined as server
static int device_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    printf("Data from the client: %.*s\n", ctxt->om->om_len, ctxt->om->om_data);
    notifyResetListener();
    return 0;
}

// Read data from ESP32 defined as server
static int device_read(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	char str[12];
	sprintf(str, "%d", GATTReadValue);
    os_mbuf_append(ctxt->om, str, strlen(str));
    return 0;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(0x180),                 // Define UUID for device type
     .characteristics = (struct ble_gatt_chr_def[]){
         {.uuid = BLE_UUID16_DECLARE(0xFEF4),           // Define UUID for reading
          .flags = BLE_GATT_CHR_F_READ,
          .access_cb = device_read},
         {.uuid = BLE_UUID16_DECLARE(0xDEAD),           // Define UUID for writing
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = device_write},
         {0}}},
    {0}};

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    // Advertise if connected
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT CONNECT %s", event->connect.status == 0 ? "OK!" : "FAILED!");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        break;
    // Advertise again after completion of the event
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("GAP", "BLE GAP EVENT");
        ble_app_advertise();
        break;
    default:
        break;
    }
    return 0;
}

// Define the BLE connection
void ble_app_advertise(void)
{
    // GAP - device name definition
    struct ble_hs_adv_fields fields;
    const char *device_name;
    memset(&fields, 0, sizeof(fields));
    device_name = "wasseruhr";
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    // GAP - device connectivity definition
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // connectable or non-connectable
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // discoverable or non-discoverable
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ble_app_advertise();
}

void host_task(void *param)
{
    nimble_port_run();
}

void init_bluetooth()
{
    nvs_flash_init();
    nimble_port_init();
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    nimble_port_freertos_init(host_task);
}

void registerResetListener(ResetListener listener) {
	gResetListener = listener;
	ESP_LOGI(TAG, "registered ResetListener");
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
