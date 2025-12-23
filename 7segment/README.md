# 7-Segment Display Dynamic Library

7447 BCD 디코더를 사용한 7세그먼트 디스플레이 제어 라이브러리 (Raspberry Pi)

## 하드웨어 연결

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

**주의:** LT, BI/RBO, RBI 핀은 연결하지 않음 (floating)

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

```c
#include "7segment.h"

int main(void) {
    // 7세그먼트 초기화
    if (seg7_init() != 0) {
        fprintf(stderr, "Initialization failed\n");
        return 1;
    }
    
    // ... 사용 ...
    
    // 정리
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
    seg7_init();
    
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
seg7_counting(9, NULL);

sleep(3);

// 카운트다운 중지
seg7_stop_counting();
```

## API 레퍼런스

### seg7_init()
- **설명:** 7세그먼트 초기화
- **반환값:** 성공 시 0, 실패 시 -1

### seg7_setnum(int num)
- **설명:** 특정 숫자 표시
- **파라미터:** num (0-9)
- **반환값:** 성공 시 0, 실패 시 -1

### seg7_counting(int start_seconds, CountdownCallback callback)
- **설명:** 비동기 카운트다운 시작
- **파라미터:**
  - start_seconds: 시작 초 (0-9)
  - callback: 완료 시 호출될 함수 (NULL 가능)
- **반환값:** 성공 시 0, 실패 시 -1

### seg7_is_counting()
- **설명:** 카운팅 중인지 확인
- **반환값:** true(카운팅 중) / false(대기 중)

### seg7_stop_counting()
- **설명:** 카운트다운 중지
- **반환값:** 성공 시 0, 실패 시 -1

### seg7_wait_counting()
- **설명:** 카운트다운 완료까지 대기 (블로킹)
- **반환값:** 성공 시 0, 실패 시 -1

### seg7_cleanup()
- **설명:** GPIO 정리 및 리소스 해제
- **반환값:** 없음

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

## 제거

```bash
sudo make uninstall
make clean
```

