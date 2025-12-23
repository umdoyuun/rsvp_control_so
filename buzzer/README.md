# Buzzer Music Player Library

Raspberry Pi용 부저 음악 재생 라이브러리

## 개요

wiringPi의 softTone을 사용하여 부저로 다양한 멜로디를 재생할 수 있는 라이브러리입니다.

## 하드웨어 연결 예시
```
Raspberry Pi                     Buzzer (Active/Passive)
============                     ======================
GPIO 1 (wiringPi) ------------> Positive (+)
GND --------------------------> Negative (-)
```

**참고:**
- GPIO 핀 번호는 wiringPi 번호 기준입니다
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
    if (wiringPiSetup() == -1) {
        fprintf(stderr, "wiringPi initialization failed\n");
        return 1;
    }

    // 2. 부저 초기화 (GPIO 핀 번호 지정)
    if (music_init(1) != 0) {  // wiringPi 핀 1번 사용
        fprintf(stderr, "Buzzer initialization failed\n");
        return 1;
    }

    // ... 음악 재생 ...

    // 3. 정리
    music_cleanup();
    return 0;
}
```

### 2. 음악 재생
```c
// 학교종 재생
play_music(MUSIC_SCHOOL_BELL);

// 반짝반짝 작은별 재생
play_music(MUSIC_TWINKLE_STAR);

// 생일 축하 노래 재생
play_music(MUSIC_HAPPY_BIRTHDAY);

// 나비야 재생
play_music(MUSIC_BUTTERFLY);
```

### 3. 완전한 예제
```c
#include "buzzer.h"
#include <wiringPi.h>
#include <stdio.h>

int main(void) {
    // wiringPi 초기화
    if (wiringPiSetup() == -1) {
        fprintf(stderr, "wiringPi setup failed\n");
        return 1;
    }

    // 부저 초기화 (GPIO 핀 1번 사용)
    if (music_init(1) != 0) {
        fprintf(stderr, "Music init failed\n");
        return 1;
    }

    printf("Playing School Bell...\n");
    play_music(MUSIC_SCHOOL_BELL);
    delay(1000);

    printf("Playing Twinkle Star...\n");
    play_music(MUSIC_TWINKLE_STAR);
    delay(1000);

    printf("Playing Happy Birthday...\n");
    play_music(MUSIC_HAPPY_BIRTHDAY);
    delay(1000);

    printf("Playing Butterfly...\n");
    play_music(MUSIC_BUTTERFLY);

    // 정리
    music_cleanup();

    return 0;
}
```

### 4. 다른 GPIO 핀 사용 예시
```c
// 예시 1: wiringPi 핀 0번 사용
music_init(0);

// 예시 2: wiringPi 핀 7번 사용
music_init(7);

// 예시 3: wiringPi 핀 11번 사용
music_init(11);
```

## API 레퍼런스

### music_init(int speaker_pin)
- **설명:** 부저 초기화 및 GPIO 핀 설정
- **파라미터:** 
  - speaker_pin: 부저가 연결된 GPIO 핀 번호 (wiringPi 번호)
- **반환값:** 성공 시 0, 실패 시 -1
- **주의:** wiringPiSetup()을 먼저 호출해야 함

### play_music(int music_number)
- **설명:** 지정된 음악 재생
- **파라미터:** 
  - music_number: 재생할 음악 번호
    - `MUSIC_SCHOOL_BELL` (1) - 학교종
    - `MUSIC_TWINKLE_STAR` (2) - 반짝반짝 작은별
    - `MUSIC_HAPPY_BIRTHDAY` (3) - 생일 축하 노래
    - `MUSIC_BUTTERFLY` (4) - 나비야
- **반환값:** 성공 시 0, 실패 시 -1
- **특징:** 음악이 끝날 때까지 블로킹됨

### music_cleanup(void)
- **설명:** 부저 정리 및 리소스 해제
- **반환값:** 없음
- **특징:** 부저 출력을 정지하고 초기화 상태를 리셋

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
- **pthread**: 라이브러리 내부 동기화

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

## 주의사항

- 음악 재생 중에는 프로그램이 블로킹됩니다
- 여러 음악을 연속으로 재생할 때는 적절한 delay를 추가하세요
- 부저의 극성을 확인하여 올바르게 연결하세요
- 패시브 부저를 사용해야 멜로디 재생이 가능합니다

## 제거
```bash
sudo make uninstall
make clean
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

// 3. play_music() 함수의 switch문에 case 추가
case MUSIC_YOUR_SONG:
    play_notes(your_song, your_song_length, 280);
    break;
```

## 라이센스

MIT License

## 문의

이슈나 개선사항이 있으시면 GitHub Issues를 통해 알려주세요.
