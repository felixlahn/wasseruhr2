#ifndef WATERFLOW_COUNTER
#define WATERFLOW_COUNTER

#include <inttypes.h>

#define WHEEL_REVOLUTION_PER_LITER	3155

typedef struct oneWheelRevolutionEvent OneWheelRevolutionEvent;
typedef void (*OneWheelRevolutionListener)(OneWheelRevolutionEvent event);

void init_waterflowCounter(void);
void registerEventListener(OneWheelRevolutionListener listener);

struct oneWheelRevolutionEvent {
	uint64_t systemtime;
};

#endif
