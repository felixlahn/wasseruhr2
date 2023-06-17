#include <stdio.h>
#include <inttypes.h>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "waterflow_counter.h"
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "esp_timer.h"

#define GPIO_FLOW_PULSE		GPIO_NUM_6
#define GPIO_INPUT_PIN_SEL	(1ULL<<GPIO_FLOW_PULSE)
#define QUEUE_LENGTH		10
#define QUEUE_ITEM_SIZE		sizeof(struct oneWheelRevolutionEvent)

#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;

static OneWheelRevolutionListener gOneWheelRevolutionListener = NULL;

void notify();

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void count_waterflow_pulse(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("waterflow_counter\n");
            notify();
        }
    }
}

void init_waterflowCounter(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    gpio_set_intr_type(GPIO_FLOW_PULSE, GPIO_INTR_ANYEDGE);

    gpio_evt_queue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    assert(gpio_evt_queue != NULL);
    xTaskCreate(count_waterflow_pulse, "count_waterflow_pulse", 2048, NULL, 10, NULL);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_FLOW_PULSE, gpio_isr_handler, (void*) GPIO_FLOW_PULSE);

    printf("Minimum free heap size: %"PRIu32" bytes\n", esp_get_minimum_free_heap_size());
}

void registerEventListener(OneWheelRevolutionListener listener) {
	gOneWheelRevolutionListener = listener;
}

void notify() {
	if (gOneWheelRevolutionListener != NULL) {
		OneWheelRevolutionEvent event;
		event.systemtime = esp_timer_get_time();

		gOneWheelRevolutionListener(event);
	}
}
