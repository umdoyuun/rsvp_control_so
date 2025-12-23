#include "light_sensor.h"
#include <stdio.h>
#include <wiringPi.h>

static int SENSOR_PIN = -1;
static bool is_initialized = false;

int light_sensor_init(const LightSensorPin* sensor_pin) {
    if (is_initialized) {
        fprintf(stderr, "Light sensor already initialized\n");
        return 0;
    }

    if (sensor_pin == NULL) {
        fprintf(stderr, "Sensor pin configuration is NULL\n");
        return -1;
    }

    if (sensor_pin->pin < 0) {
        fprintf(stderr, "Invalid sensor pin: %d\n", sensor_pin->pin);
        return -1;
    }

    SENSOR_PIN = sensor_pin->pin;

    pinMode(SENSOR_PIN, INPUT);

    is_initialized = true;

    printf("Light sensor initialized (GPIO pin: %d)\n", SENSOR_PIN);

    return 0;
}

int light_sensor_read(void) {
    if (!is_initialized) {
        fprintf(stderr, "Light sensor not initialized. Call light_sensor_init() first.\n");
        return -1;
    }

    return digitalRead(SENSOR_PIN);
}

bool light_sensor_is_bright(void) {
    int value = light_sensor_read();
    
    if (value == -1) {
        return false;
    }

    return (value == 0);
}

void light_sensor_cleanup(void) {
    if (!is_initialized) {
        return;
    }

    is_initialized = false;

    printf("Light sensor cleaned up\n");
}
