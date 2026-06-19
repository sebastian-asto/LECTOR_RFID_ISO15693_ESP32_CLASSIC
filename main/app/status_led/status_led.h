#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void status_led_init(void);
void status_led_set_tag_detected(bool detected);

#ifdef __cplusplus
}
#endif
