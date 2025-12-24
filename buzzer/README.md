# Buzzer Music Player Library

Raspberry Pi용 부저 음악 재생 라이브러리

## 개요

wiringPi의 softTone을 사용하여 부저로 다양한 멜로디를 재생할 수 있는 라이브러리입니다.
비동기 재생을 지원하여 음악이 재생되는 동안에도 다른 작업을 수행할 수 있습니다.

## 하드웨어 연결 예시
```
Raspberry Pi                     Buzzer (Active/Passive)
============                     ======================
GPIO 21 (BCM) ----------------> Positive (+)
GND --------------------------> Negative (-)
```

**참고:**
- GPIO 핀 번호는 BCM 번호 기준입니다
- 패시브 부저 사용 권장 (멜로디 재생 가능)
- 액티브 부저는 단일 톤만 재생 가능합니다
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
sudo ./test_music
```

## 실행
```bash
# 테스트 프로그램
sudo ./test_music
```

## API 사용법

### 1. 초기화

**중요:** wiringPi를 먼저 초기화한 후 buzzer 라이브러리를 초기화해야 합니다.
```c
#include "buzzer.h"
#include <wiringPi.h>

int main(void) {
    // 1. wiringPi 초기화 (필수)
    if (wiringPiSetupGpio() == -1) {  // BCM 모드
        fprintf(stderr, "wiringPi initialization failed\n");
        return 1;
    }

    // 2. 부저 초기화 (GPIO 핀 번호 지정)
    if (music_init(21) != 0) {  // BCM 핀 21번 사용
        fprintf(stderr, "Buzzer initialization failed\n");
        return 1;
    }

    // ... 음악 재생 ...

    // 3. 정리
    music_cleanup();
    return 0;
}
```

### 2. 비동기 음악 재생
```c
// 학교종 재생 (비동기)
if (play_music_async(MUSIC_SCHOOL_BELL) == 0) {
    printf("Music started!\n");
    // 음악이 재생되는 동안 다른 작업 가능
}

// 반짝반짝 작은별 재생
play_music_async(MUSIC_TWINKLE_STAR);

// 생일 축하 노래 재생
play_music_async(MUSIC_HAPPY_BIRTHDAY);

// 나비야 재생
play_music_async(MUSIC_BUTTERFLY);
```

### 3. 음악 정지
```c
// 재생 중인 음악 즉시 정지
if (stop_music() == 0) {
    printf("Music stopped\n");
} else {
    printf("No music is playing\n");
}
```

### 4. 재생 상태 확인
```c
// 현재 재생 중인지 확인
if (is_music_playing()) {
    printf("Music is playing\n");
} else {
    printf("No music playing\n");
}
```

### 5. 완전한 예제
```c
#include "buzzer.h"
#include <wiringPi.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    // wiringPi 초기화 (BCM 모드)
    if (wiringPiSetupGpio() == -1) {
        fprintf(stderr, "wiringPi setup failed\n");
        return 1;
    }

    // 부저 초기화 (GPIO 핀 21번 사용)
    if (music_init(21) != 0) {
        fprintf(stderr, "Music init failed\n");
        return 1;
    }

    // 학교종 재생 (비동기)
    printf("Playing School Bell...\n");
    play_music_async(MUSIC_SCHOOL_BELL);
    
    // 음악이 재생되는 동안 다른 작업 가능
    printf("Doing other work while music plays...\n");
    
    // 3초 후 음악 정지
    sleep(3);
    if (is_music_playing()) {
        printf("Stopping music...\n");
        stop_music();
    }
    
    sleep(1);
    
    // 다른 음악 재생
    printf("Playing Happy Birthday...\n");
    play_music_async(MUSIC_HAPPY_BIRTHDAY);
    
    // 음악이 끝날 때까지 대기
    while (is_music_playing()) {
        usleep(100000); // 100ms
    }
    
    printf("Music finished!\n");

    // 정리
    music_cleanup();

    return 0;
}
```

## API 레퍼런스

### music_init(int speaker_pin)
- **설명:** 부저 초기화 및 GPIO 핀 설정
- **파라미터:**
  - speaker_pin: 부저가 연결된 GPIO 핀 번호 (BCM 번호)
- **반환값:** 성공 시 0, 실패 시 -1
- **주의:** wiringPiSetupGpio()를 먼저 호출해야 함

### play_music_async(int music_number)
- **설명:** 지정된 음악을 비동기로 재생
- **파라미터:**
  - music_number: 재생할 음악 번호
    - `MUSIC_SCHOOL_BELL` (1) - 학교종
    - `MUSIC_TWINKLE_STAR` (2) - 반짝반짝 작은별
    - `MUSIC_HAPPY_BIRTHDAY` (3) - 생일 축하 노래
    - `MUSIC_BUTTERFLY` (4) - 나비야
- **반환값:** 성공 시 0, 실패 시 -1
- **특징:** 
  - 별도 스레드에서 재생되어 논블로킹
  - 이미 재생 중이면 실패 (-1 반환)
  - 음악 재생 중 다른 작업 수행 가능

### stop_music(void)
- **설명:** 현재 재생 중인 음악을 즉시 정지
- **반환값:** 성공 시 0, 재생 중이 아니면 -1
- **특징:** 
  - 재생 중인 음악을 즉시 중단
  - 50ms 단위로 정지 신호를 체크하여 빠른 반응

### is_music_playing(void)
- **설명:** 현재 음악이 재생 중인지 확인
- **반환값:** 재생 중이면 1, 아니면 0
- **특징:** 스레드 안전

### music_cleanup(void)
- **설명:** 부저 정리 및 리소스 해제
- **반환값:** 없음
- **특징:** 
  - 재생 중인 음악을 자동으로 정지
  - 스레드가 종료될 때까지 대기
  - 부저 출력을 정지하고 초기화 상태를 리셋

## 음악 목록

| 매크로 | 값 | 곡명 | 템포 |
|--------|-----|------|------|
| MUSIC_SCHOOL_BELL | 1 | 학교종 | 280ms |
| MUSIC_TWINKLE_STAR | 2 | 반짝반짝 작은별 | 280ms |
| MUSIC_HAPPY_BIRTHDAY | 3 | 생일 축하 노래 | 350ms |
| MUSIC_BUTTERFLY | 4 | 나비야 | 280ms |

## 컴파일 예제

### 라이브러리 설치 후
```bash
gcc your_program.c -lbuzzer -lwiringPi -lpthread -o your_program
sudo ./your_program
```

### 라이브러리 설치 안한 경우
```bash
gcc your_program.c -L. -lbuzzer -lwiringPi -lpthread -Wl,-rpath,. -o your_program
sudo ./your_program
```

## 의존성

- **wiringPi**: GPIO 제어 및 softTone 기능
- **pthread**: 비동기 재생 및 동기화

설치:
```bash
sudo apt-get update
sudo apt-get install wiringpi
```

## 음계 정보

라이브러리 내부에서 사용하는 음계:

| 음계 | 주파수 (Hz) |
|------|-------------|
| DO   | 261.63 |
| RE   | 293.66 |
| MI   | 329.63 |
| FA   | 349.23 |
| SOL  | 391.00 |
| LA   | 440.00 |
| SI   | 493.88 |
| DO_H | 523.25 |
| REST | 0 (쉼표) |

## 주요 특징

### 비동기 재생
- 음악이 백그라운드에서 재생되어 메인 스레드가 블로킹되지 않음
- 음악 재생 중에도 다른 작업 수행 가능
- 센서 모니터링, LED 제어 등과 동시 실행 가능

### 중단 가능
- `stop_music()`으로 재생 중인 음악 즉시 정지
- 50ms 단위로 정지 신호 체크하여 빠른 반응
- 카운트다운이나 타이머 기능과 조합 가능

### 스레드 안전
- pthread_mutex로 동시성 제어
- 여러 스레드에서 안전하게 호출 가능
- 재생 상태 정확하게 추적

## 주의사항

- 동시에 하나의 음악만 재생 가능 (이미 재생 중이면 새 재생 요청 거부)
- 부저의 극성을 확인하여 올바르게 연결하세요
- 패시브 부저를 사용해야 멜로디 재생이 가능합니다
- `music_cleanup()` 호출 시 재생 중인 음악이 자동으로 정지됩니다

## 제거
```bash
sudo make uninstall
make clean
```

## 사용 사례

### 1. 타이머 완료 알림
```c
// 카운트다운이 끝나면 자동으로 음악 재생
void countdown_complete_callback(void) {
    play_music_async(MUSIC_SCHOOL_BELL);
}
```

### 2. 센서 연동
```c
// 센서 감지 시 음악 재생
if (sensor_triggered()) {
    if (!is_music_playing()) {
        play_music_async(MUSIC_TWINKLE_STAR);
    }
}
```

### 3. 백그라운드 BGM
```c
// 백그라운드에서 음악 재생하면서 다른 작업
play_music_async(MUSIC_BUTTERFLY);

while (is_music_playing()) {
    // LED 깜빡이기 등 다른 작업
    led_toggle();
    delay(500);
}
```

## 사용자 정의 음악 추가

새로운 멜로디를 추가하려면 `buzzer.c` 파일을 수정하세요:
```c
// 1. buzzer.h에 매크로 추가
#define MUSIC_YOUR_SONG    5

// 2. buzzer.c에 음표 배열 추가
static int your_song[] = {
    DO, RE, MI, FA, SOL, LA, SI, DO_H, REST
};
static int your_song_length = 9;

// 3. play_music_async() 함수의 switch문에 case 추가
case MUSIC_YOUR_SONG:
    data->notes = your_song;
    data->length = your_song_length;
    data->tempo = 280;
    break;
```
