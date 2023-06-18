#ifndef WATERFLOW_COUNTER
#define WATERFLOW_COUNTER

#include <inttypes.h>

#define WHEEL_REVOLUTION_PER_LITER	3155

typedef struct oneWheelRevolutionEvent OneWheelRevolutionEvent;
typedef struct tenMilliliterFlownEvent TenMilliliterFlownEvent;
typedef void (*OneWheelRevolutionListener)(OneWheelRevolutionEvent event);
typedef void (*TenMilliliterFlownListener)(TenMilliliterFlownEvent event);

void init_waterflowCounter(void);
void registerRevolutionEventListener(OneWheelRevolutionListener listener);
void registerTenMilliliterEventListener(TenMilliliterFlownListener listener);

struct oneWheelRevolutionEvent {
	uint64_t systemtime;
};

struct tenMilliliterFlownEvent {
	uint64_t systemtime;
};

#endif
