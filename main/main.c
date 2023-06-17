#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "waterflow_counter.h"

void oneWheelRevolutionEventListener(OneWheelRevolutionEvent event) {
	printf("%" PRIu64 "\n", event.systemtime);
}

void app_main(void)
{
	init_waterflowCounter();
	registerEventListener(oneWheelRevolutionEventListener);

    while (true) {
        printf("Hello from app_main!\n");
        sleep(1);
    }
}
