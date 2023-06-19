#include <stdio.h>
#include <inttypes.h>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "button.h"
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "esp_log.h"

#define GPIO_BOOT_BUTTON	GPIO_NUM_9
#define GPIO_INPUT_PIN_SEL	(1ULL<<GPIO_BOOT_BUTTON)
#define QUEUE_LENGTH		10
#define QUEUE_ITEM_SIZE		sizeof(uint32_t)

#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;

static ButtonPressedEventListener gbuttonPressedEventListener = NULL;

static const char* TAG = "button";

void notifyButtonPressed();

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void reactOnPress(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
        	ESP_LOGI(TAG, "button pressed");
        	notifyButtonPressed();
        }
    }
}

void init_button(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    gpio_set_intr_type(GPIO_BOOT_BUTTON, GPIO_INTR_ANYEDGE);

    gpio_evt_queue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    assert(gpio_evt_queue != NULL);
    xTaskCreate(reactOnPress, "reactOnPress", 2048, NULL, 10, NULL);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_BOOT_BUTTON, gpio_isr_handler, (void*) GPIO_BOOT_BUTTON);

    printf("Minimum free heap size: %"PRIu32" bytes\n", esp_get_minimum_free_heap_size());
}

void registerButtonPressedEventListener(ButtonPressedEventListener listener) {
	gbuttonPressedEventListener = listener;
}

void notifyButtonPressed() {
	if(gbuttonPressedEventListener != NULL) {
		ButtonPressedEvent event;
		event.systemtime = esp_timer_get_time();

		gbuttonPressedEventListener(event);
		ESP_LOGI(TAG, "sent ButtonPressedEvent to listener");
	}
}
