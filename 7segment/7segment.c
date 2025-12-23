#include "7segment.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <wiringPi.h>

#define PIN_A 14
#define PIN_B 15
#define PIN_C 18
#define PIN_D 23

static const int BCD_VALUES[10][4] = {
    {0, 0, 0, 0},  // 0
    {1, 0, 0, 0},  // 1
    {0, 1, 0, 0},  // 2
    {1, 1, 0, 0},  // 3
    {0, 0, 1, 0},  // 4
    {1, 0, 1, 0},  // 5
    {0, 1, 1, 0},  // 6
    {1, 1, 1, 0},  // 7
    {0, 0, 0, 1},  // 8
    {1, 0, 0, 1},  // 9
};

static pthread_t counting_thread;
static bool is_counting = false;
static bool is_initialized = false;
static pthread_mutex_t counting_mutex = PTHREAD_MUTEX_INITIALIZER;

// 카운팅 스레드 파라미터 구조체
typedef struct {
    int start_seconds;
    CountdownCallback callback;
} CountingParams;

static void output_bcd(int num) {
    if (num < 0 || num > 9) {
        fprintf(stderr, "Invalid number: %d (must be 0-9)\n", num);
        return;
    }
    
    digitalWrite(PIN_A, BCD_VALUES[num][0]);
    digitalWrite(PIN_B, BCD_VALUES[num][1]);
    digitalWrite(PIN_C, BCD_VALUES[num][2]);
    digitalWrite(PIN_D, BCD_VALUES[num][3]);
}

static void* counting_thread_func(void* arg) {	// 카운팅 스레드 함수
    CountingParams* params = (CountingParams*)arg;
    int current = params->start_seconds;
    CountdownCallback callback = params->callback;
    
    free(params);  // 파라미터 메모리 해제
    
    // start_seconds부터 0까지 카운트다운
    for (int i = current; i >= 0; i--) {
        pthread_mutex_lock(&counting_mutex);
        if (!is_counting) {
            // 중지 요청됨
            pthread_mutex_unlock(&counting_mutex);
            return NULL;
        }
        pthread_mutex_unlock(&counting_mutex);
        
        output_bcd(i);
        
        if (i > 0) {
            sleep(1);  // 1초 대기
        }
    }
    
    // 카운팅 완료
    pthread_mutex_lock(&counting_mutex);
    is_counting = false;
    pthread_mutex_unlock(&counting_mutex);
    
    // 콜백 함수 호출
    if (callback != NULL) {
        callback();
    }
    
    return NULL;
}

int seg7_init(void) {
    if (is_initialized) {
        fprintf(stderr, "7-Segment already initialized\n");
        return 0;
    }
    
    // wiringPi 초기화 (BCM 모드)
    if (wiringPiSetupGpio() == -1) {
        fprintf(stderr, "Failed to initialize wiringPi\n");
        return -1;
    }
    
    // GPIO 핀 설정
    pinMode(PIN_A, OUTPUT);
    pinMode(PIN_B, OUTPUT);
    pinMode(PIN_C, OUTPUT);
    pinMode(PIN_D, OUTPUT);
    
    // 모든 핀 LOW로 초기화
    digitalWrite(PIN_A, LOW);
    digitalWrite(PIN_B, LOW);
    digitalWrite(PIN_C, LOW);
    digitalWrite(PIN_D, LOW);
    
    is_initialized = true;
    
    printf("7-Segment initialized (GPIO: A=%d, B=%d, C=%d, D=%d)\n",
           PIN_A, PIN_B, PIN_C, PIN_D);
    
    return 0;
}

int seg7_setnum(int num) {
    if (!is_initialized) {
        fprintf(stderr, "7-Segment not initialized. Call seg7_init() first.\n");
        return -1;
    }
    
    if (num < 0 || num > 9) {
        fprintf(stderr, "Invalid number: %d (must be 0-9)\n", num);
        return -1;
    }
    
    output_bcd(num);
    return 0;
}

int seg7_counting(int start_seconds, CountdownCallback callback) {
    if (!is_initialized) {
        fprintf(stderr, "7-Segment not initialized. Call seg7_init() first.\n");
        return -1;
    }
    
    if (start_seconds < 0 || start_seconds > 9) {
        fprintf(stderr, "Invalid start time: %d (must be 0-9)\n", start_seconds);
        return -1;
    }
    
    pthread_mutex_lock(&counting_mutex);
    
    if (is_counting) {
        fprintf(stderr, "Counting already in progress\n");
        pthread_mutex_unlock(&counting_mutex);
        return -1;
    }
    
    is_counting = true;
    pthread_mutex_unlock(&counting_mutex);
    
    // 스레드 파라미터 설정
    CountingParams* params = (CountingParams*)malloc(sizeof(CountingParams));
    if (params == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        pthread_mutex_lock(&counting_mutex);
        is_counting = false;
        pthread_mutex_unlock(&counting_mutex);
        return -1;
    }
    
    params->start_seconds = start_seconds;
    params->callback = callback;
    
    // 카운팅 스레드 시작
    if (pthread_create(&counting_thread, NULL, counting_thread_func, params) != 0) {
        fprintf(stderr, "Failed to create counting thread\n");
        free(params);
        pthread_mutex_lock(&counting_mutex);
        is_counting = false;
        pthread_mutex_unlock(&counting_mutex);
        return -1;
    }
    
    // 스레드를 detach (자동으로 리소스 정리)
    pthread_detach(counting_thread);
    
    return 0;
}

bool seg7_is_counting(void) {
    bool result;
    pthread_mutex_lock(&counting_mutex);
    result = is_counting;
    pthread_mutex_unlock(&counting_mutex);
    return result;
}

int seg7_stop_counting(void) {
    pthread_mutex_lock(&counting_mutex);
    
    if (!is_counting) {
        pthread_mutex_unlock(&counting_mutex);
        return 0;
    }
    
    is_counting = false;
    pthread_mutex_unlock(&counting_mutex);
    
    // 스레드가 종료될 때까지 짧은 대기
    usleep(100000);  // 100ms
    
    return 0;
}

int seg7_wait_counting(void) {
    while (seg7_is_counting()) {
        usleep(100000);  // 100ms 간격으로 체크
    }
    return 0;
}

void seg7_cleanup(void) {
    if (!is_initialized) {
        return;
    }
    
    // 카운팅 중지
    seg7_stop_counting();
    
    // 모든 핀 LOW로
    digitalWrite(PIN_A, LOW);
    digitalWrite(PIN_B, LOW);
    digitalWrite(PIN_C, LOW);
    digitalWrite(PIN_D, LOW);
    
    is_initialized = false;
    
    printf("7-Segment cleaned up\n");
}
