#ifndef BUTTON_H
#define BUTTON_H

#include <inttypes.h>

typedef struct buttonPressedEvent ButtonPressedEvent;
typedef void (*ButtonPressedEventListener)(ButtonPressedEvent event);

void init_button();
void registerButtonPressedEventListener(ButtonPressedEventListener listener);

struct buttonPressedEvent {
	uint64_t systemtime;
};

#endif
