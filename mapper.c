#include "mapper.h"
#include <stdint.h>

static int led_count = 0;
static int led_format = 0;
static float brightness = 1.0f;

void mapper_init(int num_leds, int fmt) {
    if (num_leds > 0) {
        led_count = num_leds;
    }
    led_format = fmt;
}

void mapper_set_brightness(float b) {
    if (b >= 0.0f && b <= 1.0f) {
        brightness = b;
    }
}
