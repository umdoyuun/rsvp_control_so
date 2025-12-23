# 7-Segment Display Dynamic Library

7447 BCD 디코더를 사용한 7세그먼트 디스플레이 제어 라이브러리 (Raspberry Pi)

## 하드웨어 연결 예시

다음은 기본적인 연결 예시입니다. 프로젝트에 맞춰 GPIO 핀을 자유롭게 선택할 수 있습니다.
```
Raspberry Pi          7447 IC          7-Segment (Common Cathode)
============          ========         ==========================
GPIO 14 -----------> Pin 7 (A)
GPIO 15 -----------> Pin 1 (B)
GPIO 18 -----------> Pin 2 (C)
GPIO 23 -----------> Pin 6 (D)

5V ---------------> Pin 16 (VCC)
GND --------------> Pin 8 (GND)

                  Pin 13-15 (a-g) --[220Ω]-> 7-Segment
```

**참고:** 
- LT, BI/RBO, RBI 핀은 연결하지 않음 (floating)
- GPIO 핀 번호는 wiringPi 번호 기준입니다
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

### 3. 테스트 프로그램 빌드
```bash
make test
```

### 4. 예제 프로그램 빌드
```bash
make example
```

## 실행
```bash
# 테스트 프로그램
sudo ./test_7segment

# 예제 프로그램
sudo ./example
```

## API 사용법

### 1. 초기화

**중요:** wiringPi를 먼저 초기화한 후 7-segment 라이브러리를 초기화해야 합니다.
```c
#include "7segment.h"
#include <wiringPi.h>

int main(void) {
    // 1. wiringPi 초기화 (필수)
    if (wiringPiSetup() == -1) {
        fprintf(stderr, "wiringPi initialization failed\n");
        return 1;
    }

    // 2. 사용할 GPIO 핀 설정
    Seg7Pins pins = {
        .pin_a = 14,  // wiringPi 핀 번호
        .pin_b = 15,
        .pin_c = 18,
        .pin_d = 23
    };

    // 3. 7세그먼트 초기화
    if (seg7_init(&pins) != 0) {
        fprintf(stderr, "7-Segment initialization failed\n");
        return 1;
    }

    // ... 사용 ...

    // 4. 정리
    seg7_cleanup();
    return 0;
}
```

### 2. 숫자 표시
```c
// 숫자 8 표시
seg7_setnum(8);

// 0-9 순차 표시
for (int i = 0; i <= 9; i++) {
    seg7_setnum(i);
    sleep(1);
}
```

### 3. 비동기 카운트다운 (콜백 없음)
```c
// 5초부터 카운트다운 시작
seg7_counting(5, NULL);

// 카운팅 중에 다른 작업 수행
while (seg7_is_counting()) {
    printf("Doing other work...\n");
    sleep(1);
}
printf("Countdown finished!\n");
```

### 4. 비동기 카운트다운 (콜백 사용)
```c
// 콜백 함수 정의
void on_complete(void) {
    printf("Timer finished! Starting next task...\n");
    // 타이머 종료 후 수행할 작업
}

int main(void) {
    // wiringPi 초기화
    wiringPiSetup();
    
    // GPIO 핀 설정
    Seg7Pins pins = {
        .pin_a = 14,
        .pin_b = 15,
        .pin_c = 18,
        .pin_d = 23
    };
    
    seg7_init(&pins);

    // 3초 카운트다운 시작 (콜백 등록)
    seg7_counting(3, on_complete);

    // 메인 스레드는 자유롭게 다른 작업 수행
    for (int i = 0; i < 10; i++) {
        printf("Processing item %d\n", i);
        usleep(500000);  // 0.5초
    }

    // 카운트다운 완료 대기
    seg7_wait_counting();

    seg7_cleanup();
    return 0;
}
```

### 5. 카운트다운 중지
```c
// 9초부터 카운트다운 시작
seg7_counting(9, NULL);

sleep(3);

// 카운트다운 중지
seg7_stop_counting();
```

### 6. 다양한 GPIO 핀 사용 예시
```c
// 예시 1: 다른 GPIO 핀 사용
Seg7Pins custom_pins = {
    .pin_a = 0,   // GPIO 17
    .pin_b = 1,   // GPIO 18
    .pin_c = 2,   // GPIO 27
    .pin_d = 3    // GPIO 22
};
seg7_init(&custom_pins);

// 예시 2: 연속된 핀 사용
Seg7Pins sequential_pins = {
    .pin_a = 10,
    .pin_b = 11,
    .pin_c = 12,
    .pin_d = 13
};
seg7_init(&sequential_pins);
```

## API 레퍼런스

### seg7_init(const Seg7Pins* pins)
- **설명:** 7세그먼트 초기화 및 GPIO 핀 설정
- **파라미터:** 
  - pins: GPIO 핀 설정 구조체 포인터
- **반환값:** 성공 시 0, 실패 시 -1
- **주의:** wiringPiSetup()을 먼저 호출해야 함

### seg7_setnum(int num)
- **설명:** 특정 숫자 표시
- **파라미터:** num (0-9)
- **반환값:** 성공 시 0, 실패 시 -1

### seg7_counting(int start_seconds, CountdownCallback callback)
- **설명:** 비동기 카운트다운 시작
- **파라미터:**
  - start_seconds: 시작 초 (0-9)
  - callback: 완료 시 호출될 함수 포인터 (NULL 가능)
- **반환값:** 성공 시 0, 실패 시 -1
- **특징:** 별도 스레드에서 실행되어 메인 스레드를 블로킹하지 않음

### seg7_is_counting(void)
- **설명:** 카운팅 중인지 확인
- **반환값:** true(카운팅 중) / false(대기 중)

### seg7_stop_counting(void)
- **설명:** 카운트다운 중지
- **반환값:** 성공 시 0, 실패 시 -1

### seg7_wait_counting(void)
- **설명:** 카운트다운 완료까지 대기 (블로킹)
- **반환값:** 성공 시 0, 실패 시 -1
- **특징:** 카운팅이 끝날 때까지 현재 스레드를 대기시킴

### seg7_cleanup(void)
- **설명:** GPIO 정리 및 리소스 해제
- **반환값:** 없음
- **특징:** 모든 GPIO 핀을 LOW로 설정하고 초기화 상태를 리셋

## 컴파일 예제

### 라이브러리 설치 후
```bash
gcc your_program.c -l7segment -lwiringPi -lpthread -o your_program
sudo ./your_program
```

### 라이브러리 설치 안한 경우
```bash
gcc your_program.c -L. -l7segment -lwiringPi -lpthread -Wl,-rpath,. -o your_program
sudo ./your_program
```

## 의존성

- **wiringPi**: GPIO 제어
- **pthread**: 비동기 카운팅

설치:
```bash
sudo apt-get update
sudo apt-get install wiringpi
```

## 스레드 안전성

- 모든 카운팅 관련 함수는 스레드 안전합니다
- 내부적으로 mutex를 사용하여 동기화를 보장합니다
- 동시에 여러 카운트다운을 시작할 수 없습니다 (하나씩만 가능)

## 제거
```bash
sudo make uninstall
make clean
```
