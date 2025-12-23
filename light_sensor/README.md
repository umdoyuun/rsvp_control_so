# Light Sensor Control Library

Raspberry Pi용 조도 센서 제어 라이브러리

## 개요

디지털 조도 센서(Light Sensor)에서 값을 읽어오는 라이브러리입니다. 일반적인 디지털 조도 센서 모듈과 호환됩니다.

## 하드웨어 연결 예시
```
Raspberry Pi                Light Sensor Module
============                ===================
3.3V or 5V --------------> VCC
GND ---------------------> GND
GPIO 17 (BCM) -----------> DO (Digital Out)
```

**참고:**
- GPIO 핀 번호는 BCM 번호 기준입니다 (wiringPiSetupGpio() 사용 시)
- 대부분의 디지털 조도 센서 모듈은 3.3V 또는 5V 모두 지원합니다
- DO 핀이 디지털 출력 핀입니다
- 일부 센서는 감도 조절용 가변저항이 있습니다
- 실제 사용 시 원하는 GPIO 핀을 선택하여 초기화할 수 있습니다

## 지원 센서

- 일반 디지털 조도 센서 모듈
- LDR(Light Dependent Resistor) 기반 디지털 출력 모듈
- 기타 디지털 출력을 제공하는 조도 센서

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
sudo ./test_light_sensor
```

## 실행
```bash
# 테스트 프로그램
sudo ./test_light_sensor
```

## API 사용법

### 1. 초기화

**중요:** wiringPi를 먼저 초기화한 후 조도 센서 라이브러리를 초기화해야 합니다.
```c
#include "light_sensor.h"
#include <wiringPi.h>

int main(void) {
    // 1. wiringPi 초기화 (BCM 모드)
    if (wiringPiSetupGpio() == -1) {
        fprintf(stderr, "wiringPi initialization failed\n");
        return 1;
    }

    // 2. 조도 센서 핀 설정
    LightSensorPin sensor_pin = {
        .pin = 17  // BCM GPIO 번호
    };

    // 3. 조도 센서 초기화
    if (light_sensor_init(&sensor_pin) != 0) {
        fprintf(stderr, "Light sensor initialization failed\n");
        return 1;
    }

    // ... 센서 값 읽기 ...

    // 4. 정리
    light_sensor_cleanup();
    return 0;
}
```

### 2. 센서 값 읽기
```c
// 디지털 값 읽기 (0 또는 1)
int value = light_sensor_read();

if (value == 1) {
    printf("밝습니다\n");
} else if (value == 0) {
    printf("어둡습니다\n");
} else {
    printf("에러 발생\n");
}
```

### 3. 상태 확인
```c
// 밝은지 확인
if (light_sensor_is_bright()) {
    printf("현재 밝은 상태입니다\n");
} else {
    printf("현재 어두운 상태입니다\n");
}
```

### 4. 완전한 예제
```c
#include "light_sensor.h"
#include <wiringPi.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    // wiringPi 초기화 (BCM 모드)
    if (wiringPiSetupGpio() == -1) {
        fprintf(stderr, "wiringPi setup failed\n");
        return 1;
    }

    // 조도 센서 초기화 (GPIO 17번 사용)
    LightSensorPin sensor_pin = {.pin = 17};
    if (light_sensor_init(&sensor_pin) != 0) {
        fprintf(stderr, "Sensor init failed\n");
        return 1;
    }

    // 10초 동안 1초마다 센서 값 읽기
    for (int i = 0; i < 10; i++) {
        int value = light_sensor_read();
        
        printf("센서 값: %d - ", value);
        
        if (light_sensor_is_bright()) {
            printf("밝음\n");
        } else {
            printf("어두움\n");
        }
        
        sleep(1);
    }

    // 정리
    light_sensor_cleanup();

    return 0;
}
```

### 5. 이벤트 기반 예제 (밝기 변화 감지)
```c
#include "light_sensor.h"
#include <wiringPi.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    wiringPiSetupGpio();
    
    LightSensorPin sensor_pin = {.pin = 17};
    light_sensor_init(&sensor_pin);

    int prev_state = -1;
    
    printf("밝기 변화를 감지합니다...\n");
    
    while (1) {
        int current_state = light_sensor_read();
        
        // 상태가 변경되었을 때만 출력
        if (current_state != prev_state && current_state != -1) {
            if (current_state == 1) {
                printf("🔆 밝아졌습니다!\n");
            } else {
                printf("🌙 어두워졌습니다!\n");
            }
            prev_state = current_state;
        }
        
        usleep(100000);  // 100ms 대기
    }

    light_sensor_cleanup();
    return 0;
}
```

### 6. 다양한 GPIO 핀 사용 예시
```c
// 예시 1: BCM GPIO 17번 사용
LightSensorPin sensor1 = {.pin = 17};
light_sensor_init(&sensor1);

// 예시 2: BCM GPIO 27번 사용
LightSensorPin sensor2 = {.pin = 27};
light_sensor_init(&sensor2);

// 예시 3: BCM GPIO 22번 사용
LightSensorPin sensor3 = {.pin = 22};
light_sensor_init(&sensor3);
```

## API 레퍼런스

### light_sensor_init(const LightSensorPin* sensor_pin)
- **설명:** 조도 센서 초기화 및 GPIO 핀 설정
- **파라미터:** 
  - sensor_pin: 센서가 연결된 GPIO 핀 설정 구조체 포인터
- **반환값:** 성공 시 0, 실패 시 -1
- **주의:** wiringPiSetupGpio()를 먼저 호출해야 함

### light_sensor_read(void)
- **설명:** 센서의 디지털 값 읽기
- **반환값:** 
  - 0: 어두움 (LOW)
  - 1: 밝음 (HIGH)
  - -1: 에러
- **특징:** 즉시 현재 값을 반환

### light_sensor_is_bright(void)
- **설명:** 현재 밝은 상태인지 확인
- **반환값:** true(밝음) / false(어두움)
- **특징:** 내부적으로 light_sensor_read()를 호출

### light_sensor_cleanup(void)
- **설명:** 센서 정리 및 리소스 해제
- **반환값:** 없음

## 센서 값 해석

| 센서 출력 | 의미 | light_sensor_is_bright() |
|-----------|------|--------------------------|
| HIGH (1) | 밝음 | true |
| LOW (0) | 어두움 | false |

**참고:** 일부 센서는 반대로 동작할 수 있습니다. 센서 모듈의 데이터시트를 확인하세요.

## 컴파일 예제

### 라이브러리 설치 후
```bash
gcc your_program.c -llight_sensor -lwiringPi -lpthread -o your_program
sudo ./your_program
```

### 라이브러리 설치 안한 경우
```bash
gcc your_program.c -L. -llight_sensor -lwiringPi -lpthread -Wl,-rpath,. -o your_program
sudo ./your_program
```

## 의존성

- **wiringPi**: GPIO 제어
- **pthread**: 라이브러리 내부 동기화

설치:
```bash
sudo apt-get update
sudo apt-get install wiringpi
```

## 주의사항

- 센서 모듈의 VCC가 3.3V인지 5V인지 확인하세요
- 대부분의 디지털 조도 센서는 3.3V와 5V 모두 지원합니다
- 센서의 감도는 모듈의 가변저항으로 조절할 수 있습니다
- 센서 값이 예상과 반대로 나온다면 센서 데이터시트를 확인하세요
- 안정적인 읽기를 위해 센서를 단단히 고정하세요

## 센서 감도 조절

대부분의 디지털 조도 센서 모듈에는 가변저항이 있습니다:
- 시계방향 회전: 감도 증가 (더 어두운 환경에서도 HIGH 출력)
- 반시계방향 회전: 감도 감소 (더 밝아야 HIGH 출력)

## 트러블슈팅

### 센서 값이 항상 0 또는 1로 고정됨
- 가변저항을 조절하여 감도를 맞추세요
- 연결을 다시 확인하세요
- 센서에 손전등을 비춰보며 테스트하세요

### 센서 값이 예상과 반대로 나옴
- 센서 모듈에 따라 로직이 반대일 수 있습니다
- `light_sensor_is_bright()` 함수의 반환값을 반전시키세요

## 제거
```bash
sudo make uninstall
make clean
```
