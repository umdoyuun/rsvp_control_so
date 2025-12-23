#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <stdbool.h>
#include "led.h"
#include "buzzer.h"
#include "light_sensor.h"
#include "7segment.h"

#define SERVER_PORT 8080
#define MAX_QUEUE_SIZE 100
#define BUFFER_SIZE 1024

// 명령 타입
typedef enum {
    CMD_LED_ON = 1,
    CMD_LED_OFF = 2,
    CMD_SET_BRIGHTNESS = 3,
    CMD_BUZZER_ON = 4,
    CMD_BUZZER_OFF = 5,
    CMD_SENSOR_ON = 6,
    CMD_SENSOR_OFF = 7,
    CMD_SEGMENT_DISPLAY = 8,
    CMD_SEGMENT_STOP = 9,
    CMD_EXIT = 0
} CommandType;

// 명령 구조체
typedef struct {
    CommandType type;
    int param1;
    int param2;
} Command;

// 응답 구조체
typedef struct {
    int status;
    char message[256];
    int value;
} CommandResponse;

// Command Queue
typedef struct {
    Command* commands[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int count;
} CommandQueue;

// 서버 상태
typedef struct {
    // Command Queue
    CommandQueue cmd_queue;
    
    // 동기화 객체
    pthread_mutex_t queue_mutex;
    pthread_mutex_t state_mutex;
    pthread_cond_t queue_not_empty;
    pthread_cond_t response_ready;
    
    // 디바이스 상태
    bool led_on;
    int led_brightness;
    bool buzzer_playing;
    bool sensor_monitoring;
    bool segment_counting;
    
    // 응답 데이터
    CommandResponse response;
    
    // 서버 상태
    bool server_running;
    int server_socket;
    int client_socket;
    bool client_connected;
    
    // 스레드
    pthread_t comm_thread;
    pthread_t device_thread;
    
} ServerState;

// 함수 선언
int server_init(ServerState* state);
void server_cleanup(ServerState* state);
void* communication_thread(void* arg);
void* device_control_thread(void* arg);

// Command Queue 함수
void queue_init(CommandQueue* queue);
bool queue_push(CommandQueue* queue, Command* cmd);
Command* queue_pop(CommandQueue* queue);
bool queue_is_empty(CommandQueue* queue);
void queue_cleanup(CommandQueue* queue);

#endif // SERVER_H
