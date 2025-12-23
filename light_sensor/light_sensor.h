#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include <stdbool.h>

typedef struct {
    int pin;
} LightSensorPin;

int light_sensor_init(const LightSensorPin* sensor_pin);

int light_sensor_read(void);

bool light_sensor_is_bright(void);

void light_sensor_cleanup(void);

#endif // LIGHT_SENSOR_H
