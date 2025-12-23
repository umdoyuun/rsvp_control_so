# LED Control Library

Raspberry Pi용 LED 제어 라이브러리

## 개요

wiringPi의 softPWM을 사용하여 LED를 제어하고 3단계 밝기 조절을 지원하는 라이브러리입니다.

## 하드웨어 연결 예시
```
Raspberry Pi                     LED
============                     ===
GPIO 0 (wiringPi) ---[220Ω]---> Anode (+)
GND ---------------------------> Cathode (-)
```

**참고:**
- GPIO 핀 번호는 wiringPi 번호 기준입니다
- 저항(220Ω~1kΩ)을 반드시 직렬로 연결하세요
- LED의 극성을 확인하여 올바르게 연결하세요
- 실제 사용 시 원하는 GPIO 핀을 선택하여 초기화할 수 있습니다

## 빌드 및 설치

### 1. 라이브러리 빌드
```bash
make
```

### 2. 시스템에 설치 (선택사항)
```bash
sudo make install
```

### 3. 테스트 프로그램 실행
```bash
sudo ./test_led
```

## 실행
```bash
# 테스트 프로그램
sudo ./test_led
```

## API 사용법

### 1. 초기화

**중요:** wiringPi를 먼저 초기화한 후 LED 라이브러리를 초기화해야 합니다.
```c
#include "led.h"
#include <wiringPi.h>

int main(void) {
    // 1. wiringPi 초기화 (필수)
    if (wiringPiSetup() == -1) {
        fprintf(stderr, "wiringPi initialization failed\n");
        return 1;
    }

    // 2. LED 핀 설정
    LedPin led_pin = {
        .pin = 0  // wiringPi 핀 번호
    };

    // 3. LED 초기화
    if (led_init(&led_pin) != 0) {
        fprintf(stderr, "LED initialization failed\n");
        return 1;
    }

    // ... LED 제어 ...

    // 4. 정리
    led_cleanup();
    return 0;
}
```

### 2. LED 켜기/끄기
```c
// LED 켜기
led_on();

// 2초 대기
sleep(2);

// LED 끄기
led_off();

// LED 상태 확인
if (led_is_on()) {
    printf("LED is ON\n");
} else {
    printf("LED is OFF\n");
}
```

### 3. 밝기 조절
```c
// 낮은 밝기 (33%)
led_set_brightness(LED_BRIGHTNESS_LOW);

// 중간 밝기 (66%)
led_set_brightness(LED_BRIGHTNESS_MEDIUM);

// 높은 밝기 (100%)
led_set_brightness(LED_BRIGHTNESS_HIGH);
```

### 4. 완전한 예제
```c
#include "led.h"
#include <wiringPi.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    // wiringPi 초기화
    if (wiringPiSetup() == -1) {
        fprintf(stderr, "wiringPi setup failed\n");
        return 1;
    }

    // LED 초기화 (GPIO 핀 0번 사용)
    LedPin led_pin = {.pin = 0};
    if (led_init(&led_pin) != 0) {
        fprintf(stderr, "LED init failed\n");
        return 1;
    }

    // LED 깜빡임
    for (int i = 0; i < 5; i++) {
        led_on();
        sleep(1);
        led_off();
        sleep(1);
    }

    // 밝기 조절
    led_set_brightness(LED_BRIGHTNESS_LOW);
    sleep(2);
    led_set_brightness(LED_BRIGHTNESS_MEDIUM);
    sleep(2);
    led_set_brightness(LED_BRIGHTNESS_HIGH);
    sleep(2);

    // 정리
    led_off();
    led_cleanup();

    return 0;
}
```

### 5. 다양한 GPIO 핀 사용 예시
```c
// 예시 1: wiringPi 핀 1번 사용
LedPin led1 = {.pin = 1};
led_init(&led1);

// 예시 2: wiringPi 핀 7번 사용
LedPin led2 = {.pin = 7};
led_init(&led2);

// 예시 3: wiringPi 핀 11번 사용
LedPin led3 = {.pin = 11};
led_init(&led3);
```

## API 레퍼런스

### led_init(const LedPin* led_pin)
- **설명:** LED 초기화 및 GPIO 핀 설정
- **파라미터:** 
  - led_pin: LED가 연결된 GPIO 핀 설정 구조체 포인터
- **반환값:** 성공 시 0, 실패 시 -1
- **주의:** wiringPiSetup()을 먼저 호출해야 함

### led_on(void)
- **설명:** LED를 최대 밝기로 켜기
- **반환값:** 성공 시 0, 실패 시 -1

### led_off(void)
- **설명:** LED 끄기
- **반환값:** 성공 시 0, 실패 시 -1

### led_set_brightness(int brightness)
- **설명:** LED 밝기를 3단계로 조절
- **파라미터:**
  - brightness: 밝기 레벨
    - `LED_BRIGHTNESS_LOW` (1) - 33%
    - `LED_BRIGHTNESS_MEDIUM` (2) - 66%
    - `LED_BRIGHTNESS_HIGH` (3) - 100%
- **반환값:** 성공 시 0, 실패 시 -1

### led_is_on(void)
- **설명:** LED 상태 확인
- **반환값:** true(켜짐) / false(꺼짐)

### led_cleanup(void)
- **설명:** LED 정리 및 리소스 해제
- **반환값:** 없음
- **특징:** LED를 끄고 초기화 상태를 리셋

## 밝기 레벨

| 매크로 | 값 | PWM 듀티 사이클 | 밝기 |
|--------|-----|-----------------|------|
| LED_BRIGHTNESS_LOW | 1 | 33% | 어두움 |
| LED_BRIGHTNESS_MEDIUM | 2 | 66% | 중간 |
| LED_BRIGHTNESS_HIGH | 3 | 100% | 밝음 |

## 컴파일 예제

### 라이브러리 설치 후
```bash
gcc your_program.c -lled -lwiringPi -lpthread -o your_program
sudo ./your_program
```

### 라이브러리 설치 안한 경우
```bash
gcc your_program.c -L. -lled -lwiringPi -lpthread -Wl,-rpath,. -o your_program
sudo ./your_program
```

## 의존성

- **wiringPi**: GPIO 제어 및 softPWM 기능
- **pthread**: 라이브러리 내부 동기화

설치:
```bash
sudo apt-get update
sudo apt-get install wiringpi
```

## 주의사항

- LED와 직렬로 저항(220Ω~1kΩ)을 반드시 연결하세요
- LED의 극성을 확인하세요 (긴 다리가 Anode +)
- GPIO의 최대 전류(16mA)를 초과하지 않도록 주의하세요
- 여러 LED를 동시에 제어하려면 각각 초기화가 필요합니다

## PWM 구현

이 라이브러리는 소프트웨어 PWM(softPWM)을 사용합니다:
- PWM 주파수: 약 100Hz
- PWM 범위: 0-100
- 정밀한 하드웨어 PWM이 필요한 경우 하드웨어 PWM 핀 사용을 권장합니다

## 제거
```bash
sudo make uninstall
make clean
```
