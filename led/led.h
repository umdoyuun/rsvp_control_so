#ifndef LED_H
#define LED_H

#include <stdbool.h>

// PWM 밝기 레벨
#define LED_BRIGHTNESS_LOW      1
#define LED_BRIGHTNESS_MEDIUM   2
#define LED_BRIGHTNESS_HIGH     3

typedef struct {
    int pin;
} LedPin;

int led_init(const LedPin* led_pin);

int led_on(void);

int led_off(void);

int led_set_brightness(int brightness);

bool led_is_on(void);

void led_cleanup(void);

#endif // LED_H
