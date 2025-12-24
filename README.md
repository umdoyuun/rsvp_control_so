# IoT 디바이스 제어 시스템

라즈베리파이 GPIO를 이용한 소켓 기반 원격 제어 시스템

---

## 📋 목차

- [프로젝트 개요](#프로젝트-개요)
- [시스템 아키텍처](#시스템-아키텍처)
- [디렉토리 구조](#디렉토리-구조)
- [하드웨어 구성](#하드웨어-구성)
- [설치 및 빌드](#설치-및-빌드)
- [서버 실행](#서버-실행)
- [클라이언트 실행](#클라이언트-실행)
- [사용 방법](#사용-방법)
- [통신 프로토콜](#통신-프로토콜)
- [트러블슈팅](#트러블슈팅)

---

## 프로젝트 개요

### 설명
멀티스레드 소켓 통신을 이용한 라즈베리파이 GPIO 디바이스 원격 제어 시스템입니다. 각 디바이스는 독립적인 공유 라이브러리(.so)로 구현되어 모듈화되어 있습니다.

### 주요 특징
- **멀티스레드 서버**: Communication Thread와 Device Control Thread 분리
- **단일 클라이언트**: 동시에 하나의 클라이언트만 연결 가능
- **모듈화 설계**: 각 디바이스별 독립 라이브러리
- **비동기 제어**: 음악 재생, 센서 감시 등 백그라운드 작업 지원
- **안전한 종료**: 논블로킹 소켓으로 Ctrl+C 즉시 반응

### 기술 스택
- **언어**: C
- **라이브러리**: wiringPi, pthread
- **통신**: TCP/IP Socket (Port 8080)
- **빌드 도구**: GCC, Make

---

## 시스템 아키텍처
```
┌───────────────────────────────────────────┐
│            Main Thread                    │
│  - 서버 초기화                             │
│  - 클라이언트 연결 accept (논블로킹)        │
│  - 시그널 처리 (Ctrl+C)                    │
└─────────────┬─────────────────────────────┘
              │
    ┌─────────┴─────────┐
    ▼                   ▼
┌──────────────┐  ┌────────────────┐
│ Comm Thread  │  │ Device Thread  │
│              │  │                │
│ - recv()     │  │ - LED 제어      │
│ - 명령 파싱   │  │ - Buzzer 제어   │
│ - Queue push │  │ - Sensor 감시   │
│ - send()     │  │ - Segment 제어  │
└──────┬───────┘  └────────┬───────┘
       │                   │
       └─────────┬─────────┘
                 ▼
         ┌──────────────┐
         │ Command Queue│
         │              │
         │ Mutex + Cond │
         └──────┬───────┘
                ▼
       ┌─────────────────────┐
       │ Device Libraries    │
       ├─────────────────────┤
       │ led/libled.so       │
       │ buzzer/libbuzzer.so │
       │ light_sensor/       │
       │   liblight_sensor.so│
       │ 7segment/           │
       │   lib7segment.so    │
       └─────────────────────┘
```

---

## 디렉토리 구조
```
rsvp_control_so/
│
├── led/                          # LED 제어 모듈
│   ├── led.c                     # LED 제어 구현
│   ├── led.h                     # LED 헤더
│   ├── libled.so                 # LED 공유 라이브러리
│   ├── test_led.c                # LED 테스트 프로그램
│   ├── Makefile
│   └── README.md
│
├── buzzer/                       # 부저 제어 모듈
│   ├── buzzer.c                  # 부저 제어 구현
│   ├── buzzer.h                  # 부저 헤더
│   ├── libbuzzer.so              # 부저 공유 라이브러리
│   ├── test_music.c              # 부저 테스트 프로그램
│   ├── Makefile
│   └── README.md
│
├── light_sensor/                 # 조도센서 모듈
│   ├── light_sensor.c            # 조도센서 구현
│   ├── light_sensor.h            # 조도센서 헤더
│   ├── liblight_sensor.so        # 조도센서 공유 라이브러리
│   ├── test_light_sensor.c       # 센서 테스트 프로그램
│   ├── Makefile
│   └── README.md
│
├── 7segment/                     # 7-Segment 모듈
│   ├── 7segment.c                # 7-Segment 구현
│   ├── 7segment.h                # 7-Segment 헤더
│   ├── lib7segment.so            # 7-Segment 공유 라이브러리
│   ├── test_7segment.c           # 7-Segment 테스트
│   ├── Makefile
│   └── README.md
│
├── server/                       # 소켓 서버
│   ├── main.c                    # 메인 함수, accept 루프
│   ├── server.c                  # 서버 초기화 및 cleanup
│   ├── server.h                  # 구조체 및 함수 선언
│   ├── communication.c           # 통신 스레드
│   ├── device_control.c          # 디바이스 제어 스레드
│   ├── command_queue.c           # 명령 큐 관리
│   └── Makefile
│
├── client/                       # 소켓 클라이언트
│   ├── main.c                    # 클라이언트 메인
│   ├── client.c                  # 클라이언트 구현
│   ├── client.h                  # 클라이언트 헤더
│   └── Makefile
│
└── README.md                     # 프로젝트 문서
```

---

## 하드웨어 구성

### GPIO 핀맵

| 디바이스 | GPIO 핀 | 기능 |
|---------|---------|------|
| LED | 12 | PWM 밝기 조절 (3단계) |
| Buzzer | 21 | 멜로디 재생 (4곡) |
| Light Sensor | 11 | 밝기 감지 |
| 7-Segment (A) | 14 | 카운트다운 표시 |
| 7-Segment (B) | 15 | 카운트다운 표시 |
| 7-Segment (C) | 18 | 카운트다운 표시 |
| 7-Segment (D) | 23 | 카운트다운 표시 |

### 제어 가능 디바이스

**1. LED (led/libled.so)**
- ON/OFF 제어
- 밝기 3단계 (LOW=1, MEDIUM=2, HIGH=3)
- 조도센서 연동 시 자동 제어

**2. Buzzer (buzzer/libbuzzer.so)**
- 4가지 멜로디
  - 1: 학교종 (School Bell)
  - 2: 반짝반짝 작은별 (Twinkle Star)
  - 3: 생일 축하 (Happy Birthday)
  - 4: 나비야 (Butterfly)
- **비동기 재생**: 음악 재생 중에도 다른 명령 처리 가능
- **중단 기능**: 재생 중인 음악을 즉시 정지 (BUZZER OFF)
- **상태 확인**: 현재 재생 중인지 실시간 확인

**3. Light Sensor (light_sensor/liblight_sensor.so)**
- 실시간 밝기 감지
- 자동 LED 제어: 밝으면 OFF, 어두우면 ON
- 백그라운드 감시

**4. 7-Segment Display (7segment/lib7segment.so)**
- 1-9초 카운트다운
- 0 도달 시 자동으로 학교종 음악 재생
- 진행 중 중단 가능

---

## 설치 및 빌드

### 1. 사전 요구사항
```bash
# wiringPi 설치 확인
gpio -v

# 없으면 설치
sudo apt-get update
sudo apt-get install wiringpi

# 개발 도구 설치
sudo apt-get install build-essential
```

### 2. 전체 빌드 스크립트

프로젝트 루트에 `build.sh` 파일을 생성:
```bash
#!/bin/bash

echo "=== IoT 디바이스 제어 시스템 빌드 ==="

# 각 모듈 빌드
echo "Building LED module..."
cd led && make clean && make && cd ..

echo "Building Buzzer module..."
cd buzzer && make clean && make && cd ..

echo "Building Light Sensor module..."
cd light_sensor && make clean && make && cd ..

echo "Building 7-Segment module..."
cd 7segment && make clean && make && cd ..

# 서버 디렉토리에 라이브러리 링크
echo "Linking libraries to server..."
cd server
ln -sf ../led/libled.so .
ln -sf ../buzzer/libbuzzer.so .
ln -sf ../light_sensor/liblight_sensor.so .
ln -sf ../7segment/lib7segment.so .
ln -sf ../led/led.h .
ln -sf ../buzzer/buzzer.h .
ln -sf ../light_sensor/light_sensor.h .
ln -sf ../7segment/7segment.h .

# 서버 빌드
echo "Building server..."
make clean && make
cd ..

# 클라이언트 빌드
echo "Building client..."
cd client && make clean && make && cd ..

echo "=== 빌드 완료 ==="
```

실행:
```bash
chmod +x build.sh
./build.sh
```

### 3. 개별 모듈 빌드

#### LED 모듈
```bash
cd led
make clean
make
# libled.so 생성됨

# 테스트 (선택)
make test
sudo ./test_led
```

#### Buzzer 모듈
```bash
cd buzzer
make clean
make
# libbuzzer.so 생성됨

# 테스트 (선택)
make test
sudo ./test_music
```

#### Light Sensor 모듈
```bash
cd light_sensor
make clean
make
# liblight_sensor.so 생성됨

# 테스트 (선택)
make test
sudo ./test_light_sensor
```

#### 7-Segment 모듈
```bash
cd 7segment
make clean
make
# lib7segment.so 생성됨

# 테스트 (선택)
make test
sudo ./test_7segment
```

### 4. 서버 빌드
```bash
cd server

# 라이브러리 링크 (build.sh로 이미 했다면 생략)
ln -sf ../led/libled.so .
ln -sf ../buzzer/libbuzzer.so .
ln -sf ../light_sensor/liblight_sensor.so .
ln -sf ../7segment/lib7segment.so .
ln -sf ../led/led.h .
ln -sf ../buzzer/buzzer.h .
ln -sf ../light_sensor/light_sensor.h .
ln -sf ../7segment/7segment.h .

# 빌드
make clean
make
```

### 5. 클라이언트 빌드
```bash
cd client
make clean
make
```

---

## 서버 실행

### 1. 라이브러리 경로 설정
```bash
cd server

# 현재 디렉토리를 라이브러리 경로에 추가
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

# 또는 실행 시마다
LD_LIBRARY_PATH=. sudo ./server
```

### 2. 서버 실행
```bash
sudo ./server
```

**실행 출력:**
```
=== IoT Device Control Server ===
✓ wiringPi initialized
Initializing devices...
✓ 7-Segment initialized (pins: A=14, B=15, C=18, D=23)
✓ Buzzer initialized (pin: 21)
✓ LED initialized (pin: 24)
✓ Light sensor initialized (pin: 11)

=== Server initialized successfully ===
Listening on port 8080...
Press Ctrl+C to stop

[Device Thread] Started
Waiting for client connection...
```

### 3. 클라이언트 연결 시
```
Client connected from 192.168.0.105:54321
[Comm Thread] Started for client socket 4
```

### 4. 서버 종료
```bash
# Ctrl+C 입력
^C
Received signal 2, shutting down...

Cleaning up server...
[Comm Thread] Stopped
[Device Thread] Stopped
Cleaning up devices...
Server cleanup completed
Server stopped
```

---

## 클라이언트 실행

### 1. 로컬 연결
```bash
cd client
./client
```

### 2. 원격 서버 연결
```bash
# 라즈베리파이 IP가 192.168.0.100인 경우
./client 192.168.0.100
```

### 3. 실행 화면
```
=== IoT Device Control Client ===
Connecting to 192.168.0.100:8080...
Connected to server!

Connected to IoT Device Control Server

[ Device Control Menu ]
1. LED ON
2. LED OFF
3. Set Brightness
4. BUZZER ON (play melody)
5. BUZZER OFF (stop)
6. SENSOR ON (감시 시작)
7. SENSOR OFF (감시 종료)
8. SEGMENT DISPLAY (숫자 표시 후 카운트다운)
9. SEGMENT STOP (카운트다운 중단)
0. Exit
Select: 
```

---

## 사용 방법

### 1. LED 제어

#### LED 켜기
```
Select: 1
[SUCCESS] LED turned ON
```

#### 밝기 조절
```
Select: 3
Enter brightness level (1-3): 2
[SUCCESS] Brightness set to 2
```

#### LED 끄기
```
Select: 2
[SUCCESS] LED turned OFF
```

### 2. Buzzer 제어

#### 음악 재생
```
Select: 4
Enter music number (1:School Bell, 2:Twinkle Star, 3:Happy Birthday, 4:Butterfly): 1
[SUCCESS] Playing music 1
```

**서버 로그:**
```
[Device] Music 1 started
[Buzzer] Music playback started
(음악 재생 중에도 다른 명령 처리 가능)
[Buzzer] Music playback completed normally
```

#### 음악 정지
```
Select: 5
[SUCCESS] Music stopped
```

**서버 로그:**
```
[Device] Music stopped
[Buzzer] Music playback stopped
```

**특징:**
- 음악이 재생되는 동안에도 LED 제어, 센서 감시 등 다른 작업 가능
- 음악 재생 중 BUZZER OFF 명령으로 즉시 중단 가능
- 이미 재생 중일 때는 새로운 음악 재생 불가

### 3. 조도센서 자동 제어

#### 감시 시작
```
Select: 6
[SUCCESS] Sensor monitoring started
```

**서버 로그 (밝기 변화 시):**
```
[Device] Sensor monitoring started
[Device] Dark detected - LED ON
[Device] Light detected - LED OFF
```

#### 감시 종료
```
Select: 7
[SUCCESS] Sensor monitoring stopped
```

### 4. 7-Segment 카운트다운

#### 카운트다운 시작
```
Select: 8
Enter countdown seconds (1-9): 5
[SUCCESS] Countdown started from 5 (will play music at 0)
```

**서버 로그:**
```
[Device] Countdown started: 5 seconds
[Device] Countdown completed - Playing school bell music
```

#### 카운트다운 중단
```
Select: 9
[SUCCESS] Countdown stopped
```

### 5. 종료
```
Select: 0
Disconnecting...

Disconnected from server
```

---

## 통신 프로토콜

### 명령 포맷
```
[CMD_TYPE] [PARAM1] [PARAM2]\n
```

### 응답 포맷
```
[SUCCESS] 메시지\n
또는
[ERROR] 메시지\n
```

### 명령 타입

| CMD | 명령 | PARAM1 | PARAM2 | 라이브러리 |
|-----|------|--------|--------|-----------|
| 0 | Exit | - | - | - |
| 1 | LED ON | - | - | libled.so |
| 2 | LED OFF | - | - | libled.so |
| 3 | Set Brightness | 1-3 | - | libled.so |
| 4 | Buzzer ON | 1-4 | - | libbuzzer.so |
| 5 | Buzzer OFF | - | - | libbuzzer.so |
| 6 | Sensor ON | - | - | liblight_sensor.so |
| 7 | Sensor OFF | - | - | liblight_sensor.so |
| 8 | Segment Display | 1-9 | - | lib7segment.so |
| 9 | Segment Stop | - | - | lib7segment.so |

---

## 트러블슈팅

### 빌드 문제

#### Q1. "wiringPi.h: No such file or directory"
```bash
# wiringPi 재설치
sudo apt-get install --reinstall wiringpi
```

#### Q2. "cannot find -lled"
```bash
# 서버 디렉토리에서 라이브러리 링크 확인
cd server
ls -l *.so

# 없으면 링크 생성
ln -sf ../led/libled.so .
ln -sf ../buzzer/libbuzzer.so .
ln -sf ../light_sensor/liblight_sensor.so .
ln -sf ../7segment/lib7segment.so .
```

### 서버 실행 문제

#### Q1. "error while loading shared libraries"
```bash
# 라이브러리 경로 설정
cd server
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
sudo -E ./server

# 또는
LD_LIBRARY_PATH=. sudo ./server
```

#### Q2. "bind: Address already in use"
```bash
# 포트 사용 프로세스 확인
sudo netstat -tulpn | grep 8080

# 기존 서버 종료
sudo killall server
```

#### Q3. 디바이스 초기화 실패
```bash
# GPIO 상태 확인
gpio readall

# 하드웨어 연결 확인
```

### 클라이언트 문제

#### Q1. "Connection refused"
```bash
# 서버 실행 여부 확인
ps aux | grep server

# 네트워크 연결 확인
ping 192.168.0.100

# 방화벽 확인
sudo ufw allow 8080
```

---

## 개별 모듈 테스트

각 모듈은 독립적으로 테스트 가능합니다.

### LED 테스트
```bash
cd led
make test
sudo ./test_led
```

### Buzzer 테스트
```bash
cd buzzer
make test
sudo ./test_music
```

### Light Sensor 테스트
```bash
cd light_sensor
make test
sudo ./test_light_sensor
```

### 7-Segment 테스트
```bash
cd 7segment
make test
sudo ./test_7segment
```

---

## 참고 문서

각 모듈별 상세 문서:
- `led/README.md` - LED 제어 API
- `buzzer/README.md` - Buzzer 제어 API
- `light_sensor/README.md` - 조도센서 API
- `7segment/README.md` - 7-Segment API
