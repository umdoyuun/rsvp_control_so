#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wiringPi.h>
#include "server.h"

static LedPin led_pin = {.pin = 12};
static LightSensorPin sensor_pin = {.pin = 11}; 
static Seg7Pins seg7_pins = {
    .pin_a = 14,
    .pin_b = 15,
    .pin_c = 18,
    .pin_d = 23 
};
static const int buzzer_pin = 21;

int server_init(ServerState* state) {
    memset(state, 0, sizeof(ServerState));
    
    // wiringPi 초기화 (GPIO 모드)
    if (wiringPiSetupGpio() == -1) {
        fprintf(stderr, "Failed to initialize wiringPi\n");
        return -1;
    }
    printf("✓ wiringPi initialized\n");
    
    // Queue 초기화
    queue_init(&state->cmd_queue);
    
    // Mutex 초기화
    if (pthread_mutex_init(&state->queue_mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize queue mutex\n");
        return -1;
    }
    
    if (pthread_mutex_init(&state->state_mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize state mutex\n");
        pthread_mutex_destroy(&state->queue_mutex);
        return -1;
    }
    
    // Condition Variable 초기화
    if (pthread_cond_init(&state->queue_not_empty, NULL) != 0) {
        fprintf(stderr, "Failed to initialize queue_not_empty condition\n");
        pthread_mutex_destroy(&state->queue_mutex);
        pthread_mutex_destroy(&state->state_mutex);
        return -1;
    }
    
    if (pthread_cond_init(&state->response_ready, NULL) != 0) {
        fprintf(stderr, "Failed to initialize response_ready condition\n");
        pthread_cond_destroy(&state->queue_not_empty);
        pthread_mutex_destroy(&state->queue_mutex);
        pthread_mutex_destroy(&state->state_mutex);
        return -1;
    }
    
    // 디바이스 초기화
    printf("Initializing devices...\n");
    
    // 7-Segment 초기화
    if (seg7_init(&seg7_pins) != 0) {
        fprintf(stderr, "Failed to initialize 7-segment\n");
        goto cleanup_sync;
    }
    printf("✓ 7-Segment initialized (pins: A=%d, B=%d, C=%d, D=%d)\n",
           seg7_pins.pin_a, seg7_pins.pin_b, seg7_pins.pin_c, seg7_pins.pin_d);
    
    // Buzzer 초기화
    if (music_init(buzzer_pin) != 0) {
        fprintf(stderr, "Failed to initialize buzzer\n");
        seg7_cleanup();
        goto cleanup_sync;
    }
    printf("✓ Buzzer initialized (pin: %d)\n", buzzer_pin);
    
    // LED 초기화
    if (led_init(&led_pin) != 0) {
        fprintf(stderr, "Failed to initialize LED\n");
        music_cleanup();
        seg7_cleanup();
        goto cleanup_sync;
    }
    printf("✓ LED initialized (pin: %d)\n", led_pin.pin);
    
    // Light Sensor 초기화
    if (light_sensor_init(&sensor_pin) != 0) {
        fprintf(stderr, "Failed to initialize light sensor\n");
        led_cleanup();
        music_cleanup();
        seg7_cleanup();
        goto cleanup_sync;
    }
    printf("✓ Light sensor initialized (pin: %d)\n", sensor_pin.pin);
    
    // 소켓 생성
    state->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (state->server_socket < 0) {
        perror("socket");
        goto cleanup_devices;
    }
    
    // SO_REUSEADDR 옵션 설정
    int opt = 1;
    if (setsockopt(state->server_socket, SOL_SOCKET, SO_REUSEADDR, 
                   &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(state->server_socket);
        goto cleanup_devices;
    }
    
    // 논블로킹 소켓으로 설정
    int flags = fcntl(state->server_socket, F_GETFL, 0);
    fcntl(state->server_socket, F_SETFL, flags | O_NONBLOCK);
    
    // 바인딩
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (bind(state->server_socket, (struct sockaddr*)&server_addr, 
             sizeof(server_addr)) < 0) {
        perror("bind");
        close(state->server_socket);
        goto cleanup_devices;
    }
    
    // 리슨
    if (listen(state->server_socket, 1) < 0) {
        perror("listen");
        close(state->server_socket);
        goto cleanup_devices;
    }
    
    state->server_running = true;
    state->client_socket = -1;
    state->client_connected = false;
    
    printf("\n=== Server initialized successfully ===\n");
    printf("Listening on port %d...\n", SERVER_PORT);
    printf("Press Ctrl+C to stop\n\n");
    
    return 0;
    
cleanup_devices:
    light_sensor_cleanup();
    led_cleanup();
    music_cleanup();
    seg7_cleanup();
    
cleanup_sync:
    pthread_cond_destroy(&state->response_ready);
    pthread_cond_destroy(&state->queue_not_empty);
    pthread_mutex_destroy(&state->state_mutex);
    pthread_mutex_destroy(&state->queue_mutex);
    
    return -1;
}

void server_cleanup(ServerState* state) {
    printf("\nCleaning up server...\n");
    
    state->server_running = false;
    
    // 스레드 종료 대기
    if (state->client_connected) {
        pthread_cond_signal(&state->queue_not_empty);
        pthread_cond_signal(&state->response_ready);
        
        pthread_join(state->comm_thread, NULL);
    }
    
    pthread_cond_signal(&state->queue_not_empty);
    pthread_join(state->device_thread, NULL);
    
    // 소켓 종료
    if (state->client_socket >= 0) {
        close(state->client_socket);
    }
    if (state->server_socket >= 0) {
        close(state->server_socket);
    }
    
    // Command Queue 정리
    queue_cleanup(&state->cmd_queue);
    
    // 디바이스 정리
    printf("Cleaning up devices...\n");
    light_sensor_cleanup();
    led_cleanup();
    music_cleanup();
    seg7_cleanup();
    
    // 동기화 객체 정리
    pthread_cond_destroy(&state->response_ready);
    pthread_cond_destroy(&state->queue_not_empty);
    pthread_mutex_destroy(&state->state_mutex);
    pthread_mutex_destroy(&state->queue_mutex);
    
    printf("Server cleanup completed\n");
}
