#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include "server.h"

static void send_menu(int client_socket) {
    const char* menu = 
        "\n[ Device Control Menu ]\n"
        "1. LED ON\n"
        "2. LED OFF\n"
        "3. Set Brightness\n"
        "4. BUZZER ON (play melody)\n"
        "5. BUZZER OFF (stop)\n"
        "6. SENSOR ON (감시 시작)\n"
        "7. SENSOR OFF (감시 종료)\n"
        "8. SEGMENT DISPLAY (숫자 표시 후 카운트다운)\n"
        "9. SEGMENT STOP (카운트다운 중단)\n"
        "0. Exit\n"
        "Select: ";
    
    send(client_socket, menu, strlen(menu), 0);
}

static bool parse_command(const char* buffer, Command* cmd) {
    int type, param1 = 0, param2 = 0;
    
    if (sscanf(buffer, "%d %d %d", &type, &param1, &param2) < 1) {
        return false;
    }
    
    cmd->type = (CommandType)type;
    cmd->param1 = param1;
    cmd->param2 = param2;
    
    return true;
}

static bool send_response(int client_socket, const CommandResponse* response) {
    char buffer[512];
    
    if (response->status == 0) {
        snprintf(buffer, sizeof(buffer), "[SUCCESS] %s\n", response->message);
    } else {
        snprintf(buffer, sizeof(buffer), "[ERROR] %s\n", response->message);
    }
    
    ssize_t sent = send(client_socket, buffer, strlen(buffer), 0);
    return sent > 0;
}

void* communication_thread(void* arg) {
    ServerState* state = (ServerState*)arg;
    int client_socket = state->client_socket;
    
    printf("[Comm Thread] Started for client socket %d\n", client_socket);
    
    // 환영 메시지
    const char* welcome = "Connected to IoT Device Control Server\n";
    send(client_socket, welcome, strlen(welcome), 0);
    
    char buffer[BUFFER_SIZE];
    
    while (state->server_running) {
        send_menu(client_socket);
        
        memset(buffer, 0, sizeof(buffer));
        ssize_t received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (received <= 0) {
            if (received == 0) {
                printf("[Comm Thread] Client disconnected\n");
            } else {
                printf("[Comm Thread] Receive error: %s\n", strerror(errno));
            }
            break;
        }
        
        buffer[received] = '\0';
        
        // 개행 문자 제거
        char* newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        newline = strchr(buffer, '\r');
        if (newline) *newline = '\0';
        
        printf("[Comm Thread] Received: %s\n", buffer);
        
        Command cmd;
        if (!parse_command(buffer, &cmd)) {
            const char* error_msg = "[ERROR] Invalid command format\n";
            send(client_socket, error_msg, strlen(error_msg), 0);
            continue;
        }
        
        // EXIT 명령 처리
        if (cmd.type == CMD_EXIT) {
            const char* bye_msg = "Disconnecting...\n";
            send(client_socket, bye_msg, strlen(bye_msg), 0);
            printf("[Comm Thread] Client requested exit\n");
            break;
        }
        
        // 추가 파라미터 요청 (brightness, music number, countdown seconds)
        if (cmd.type == CMD_SET_BRIGHTNESS) {
            if (cmd.param1 == 0) {
                const char* prompt = "Enter brightness level (1-3): ";
                send(client_socket, prompt, strlen(prompt), 0);
                
                memset(buffer, 0, sizeof(buffer));
                received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
                if (received <= 0) break;
                
                cmd.param1 = atoi(buffer);
            }
        } else if (cmd.type == CMD_BUZZER_ON) {
            if (cmd.param1 == 0) {
                const char* prompt = "Enter music number (1:School Bell, 2:Twinkle Star, 3:Happy Birthday, 4:Butterfly): ";
                send(client_socket, prompt, strlen(prompt), 0);
                
                memset(buffer, 0, sizeof(buffer));
                received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
                if (received <= 0) break;
                
                cmd.param1 = atoi(buffer);
            }
        } else if (cmd.type == CMD_SEGMENT_DISPLAY) {
            if (cmd.param1 == 0) {
                const char* prompt = "Enter countdown seconds (1-9): ";
                send(client_socket, prompt, strlen(prompt), 0);
                
                memset(buffer, 0, sizeof(buffer));
                received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
                if (received <= 0) break;
                
                cmd.param1 = atoi(buffer);
            }
        }
        
        // Command Queue에 추가
        pthread_mutex_lock(&state->queue_mutex);
        
        if (!queue_push(&state->cmd_queue, &cmd)) {
            pthread_mutex_unlock(&state->queue_mutex);
            const char* error_msg = "[ERROR] Command queue full\n";
            send(client_socket, error_msg, strlen(error_msg), 0);
            continue;
        }
        
        // Device Thread 깨우기
        pthread_cond_signal(&state->queue_not_empty);
        
        // 응답 대기
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5; // 5초 timeout
        
        int wait_result = pthread_cond_timedwait(&state->response_ready, 
                                                  &state->queue_mutex, &ts);
        
        if (wait_result == 0) {
            CommandResponse response = state->response;
            pthread_mutex_unlock(&state->queue_mutex);
            
            if (!send_response(client_socket, &response)) {
                printf("[Comm Thread] Failed to send response\n");
                break;
            }
        } else {
            pthread_mutex_unlock(&state->queue_mutex);
            const char* timeout_msg = "[ERROR] Command timeout\n";
            send(client_socket, timeout_msg, strlen(timeout_msg), 0);
        }
    }
    
    // 연결 종료 처리
    pthread_mutex_lock(&state->state_mutex);
    state->client_connected = false;
    close(client_socket);
    state->client_socket = -1;
    pthread_mutex_unlock(&state->state_mutex);
    
    printf("[Comm Thread] Stopped\n");
    return NULL;
}
