#include "led.h"
#include <stdio.h>
#include <wiringPi.h>

#define PWM_LOW     33
#define PWM_MEDIUM  66
#define PWM_HIGH    100
#define PWM_RANGE   100

static int LED_PIN = -1;
static bool is_initialized = false;
static bool led_state = false;

int led_init(const LedPin* led_pin) {
    if (is_initialized) {
        fprintf(stderr, "LED already initialized\n");
        return 0;
    }

    if (led_pin == NULL) {
        fprintf(stderr, "LED pin configuration is NULL\n");
        return -1;
    }

    if (led_pin->pin < 0) {
        fprintf(stderr, "Invalid LED pin: %d\n", led_pin->pin);
        return -1;
    }

    LED_PIN = led_pin->pin;

	pinMode(LED_PIN, PWM_OUTPUT);
	pwmSetMode(PWM_MODE_MS);
	pwmSetRange(PWM_RANGE);
	pwmSetClock(192);
	pwmWrite(LED_PIN, PWM_HIGH);

    led_state = false;
    is_initialized = true;

    printf("LED initialized (GPIO pin: %d)\n", LED_PIN);

    return 0;
}

int led_on(void) {
    if (!is_initialized) {
        fprintf(stderr, "LED not initialized. Call led_init() first.\n");
        return -1;
    }

    pwmWrite(LED_PIN, 0);
    led_state = true;

    return 0;
}

int led_off(void) {
    if (!is_initialized) {
        fprintf(stderr, "LED not initialized. Call led_init() first.\n");
        return -1;
    }

    pwmWrite(LED_PIN, PWM_HIGH);
    led_state = false;

    return 0;
}

int led_set_brightness(int brightness) {
    if (!is_initialized) {
        fprintf(stderr, "LED not initialized. Call led_init() first.\n");
        return -1;
    }

    int pwm_value;

    switch(brightness) {
        case LED_BRIGHTNESS_LOW:
            pwm_value = PWM_HIGH - PWM_LOW;  // 67
            break;
        case LED_BRIGHTNESS_MEDIUM:
            pwm_value = PWM_HIGH - PWM_MEDIUM;  // 34
            break;
        case LED_BRIGHTNESS_HIGH:
            pwm_value = 0;
            break;
        default:
            fprintf(stderr, "Invalid brightness level: %d (use 1, 2, or 3)\n", brightness);
            return -1;
    }

    pwmWrite(LED_PIN, pwm_value);
    led_state = true;

    return 0;
}

bool led_is_on(void) {
    return led_state;
}

void led_cleanup(void) {
    if (!is_initialized) {
        return;
    }

    pwmWrite(LED_PIN, PWM_HIGH);
    led_state = false;
    is_initialized = false;

    printf("LED cleaned up\n");
}
