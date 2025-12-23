#ifndef SEVEN_SEGMENT_H
#define SEVEN_SEGMENT_H

#include <stdbool.h>

typedef void (*CountdownCallback)(void);

// GPIO 핀 설정을 위한 구조체
typedef struct {
    int pin_a;
    int pin_b;
    int pin_c;
    int pin_d;
} Seg7Pins;

int seg7_init(const Seg7Pins* pins);

int seg7_setnum(int num);

int seg7_counting(int start_seconds, CountdownCallback callback);

bool seg7_is_counting(void);

int seg7_stop_counting(void);

int seg7_wait_counting(void);

void seg7_cleanup(void);

#endif // SEVEN_SEGMENT_H
