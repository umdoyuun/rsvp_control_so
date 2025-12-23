#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "server.h"

// 카운트다운 완료 콜백
static ServerState* g_state = NULL;

void countdown_complete_callback(void) {
    if (g_state) {
        pthread_mutex_lock(&g_state->state_mutex);
        g_state->segment_counting = false;
        printf("[Device] Countdown completed - Playing school bell music\n");
        pthread_mutex_unlock(&g_state->state_mutex);
        
        // 학교종 음악 재생
        play_music(MUSIC_SCHOOL_BELL);
    }
}

static void process_led_on(ServerState* state, CommandResponse* response) {
    if (led_on() == 0) {
        pthread_mutex_lock(&state->state_mutex);
        state->led_on = true;
        pthread_mutex_unlock(&state->state_mutex);
        
        response->status = 0;
        strcpy(response->message, "LED turned ON");
    } else {
        response->status = -1;
        strcpy(response->message, "Failed to turn ON LED");
    }
}

static void process_led_off(ServerState* state, CommandResponse* response) {
    if (led_off() == 0) {
        pthread_mutex_lock(&state->state_mutex);
        state->led_on = false;
        pthread_mutex_unlock(&state->state_mutex);
        
        response->status = 0;
        strcpy(response->message, "LED turned OFF");
    } else {
        response->status = -1;
        strcpy(response->message, "Failed to turn OFF LED");
    }
}

static void process_set_brightness(ServerState* state, Command* cmd, CommandResponse* response) {
    if (cmd->param1 < LED_BRIGHTNESS_LOW || cmd->param1 > LED_BRIGHTNESS_HIGH) {
        response->status = -1;
        sprintf(response->message, "Invalid brightness level: %d (use 1-3)", cmd->param1);
        return;
    }
    
    if (led_set_brightness(cmd->param1) == 0) {
        pthread_mutex_lock(&state->state_mutex);
        state->led_brightness = cmd->param1;
        pthread_mutex_unlock(&state->state_mutex);
        
        response->status = 0;
        sprintf(response->message, "Brightness set to %d", cmd->param1);
    } else {
        response->status = -1;
        strcpy(response->message, "Failed to set brightness");
    }
}

static void process_buzzer_on(ServerState* state, Command* cmd, CommandResponse* response) {
    int music_num = cmd->param1;
    if (music_num < MUSIC_SCHOOL_BELL || music_num > MUSIC_BUTTERFLY) {
        music_num = MUSIC_SCHOOL_BELL;
    }
    
    pthread_mutex_lock(&state->state_mutex);
    state->buzzer_playing = true;
    pthread_mutex_unlock(&state->state_mutex);
    
    // 먼저 응답 보내기
    response->status = 0;
    sprintf(response->message, "Playing music %d", music_num);
    
    // 응답 전달
    pthread_mutex_lock(&state->queue_mutex);
    memcpy(&state->response, response, sizeof(CommandResponse));
    pthread_cond_signal(&state->response_ready);
    pthread_mutex_unlock(&state->queue_mutex);
    
    // 음악 재생 (블로킹)
    printf("[Device] Playing music %d...\n", music_num);
    play_music(music_num);
    
    pthread_mutex_lock(&state->state_mutex);
    state->buzzer_playing = false;
    pthread_mutex_unlock(&state->state_mutex);
    
    printf("[Device] Music playback completed\n");
    
    // 이미 응답을 보냈으므로 여기서는 아무것도 하지 않음
    response->status = -2; // 특수 플래그: 이미 응답 보냄
}

static void process_buzzer_off(ServerState* state, CommandResponse* response) {
    pthread_mutex_lock(&state->state_mutex);
    state->buzzer_playing = false;
    pthread_mutex_unlock(&state->state_mutex);
    
    response->status = 0;
    strcpy(response->message, "Buzzer stopped");
}

static void process_sensor_on(ServerState* state, CommandResponse* response) {
    pthread_mutex_lock(&state->state_mutex);
    state->sensor_monitoring = true;
    pthread_mutex_unlock(&state->state_mutex);
    
    response->status = 0;
    strcpy(response->message, "Sensor monitoring started");
    printf("[Device] Sensor monitoring started\n");
}

static void process_sensor_off(ServerState* state, CommandResponse* response) {
    pthread_mutex_lock(&state->state_mutex);
    state->sensor_monitoring = false;
    pthread_mutex_unlock(&state->state_mutex);
    
    response->status = 0;
    strcpy(response->message, "Sensor monitoring stopped");
    printf("[Device] Sensor monitoring stopped\n");
}

static void process_segment_display(ServerState* state, Command* cmd, CommandResponse* response) {
    // 1-9 범위 체크
    if (cmd->param1 < 1 || cmd->param1 > 9) {
        response->status = -1;
        sprintf(response->message, "Invalid countdown seconds: %d (use 1-9)", cmd->param1);
        return;
    }
    
    pthread_mutex_lock(&state->state_mutex);
    if (state->segment_counting) {
        pthread_mutex_unlock(&state->state_mutex);
        response->status = -1;
        strcpy(response->message, "Countdown already in progress");
        return;
    }
    state->segment_counting = true;
    pthread_mutex_unlock(&state->state_mutex);
    
    if (seg7_counting(cmd->param1, countdown_complete_callback) == 0) {
        response->status = 0;
        sprintf(response->message, "Countdown started from %d (will play music at 0)", cmd->param1);
        printf("[Device] Countdown started: %d seconds\n", cmd->param1);
    } else {
        pthread_mutex_lock(&state->state_mutex);
        state->segment_counting = false;
        pthread_mutex_unlock(&state->state_mutex);
        
        response->status = -1;
        strcpy(response->message, "Failed to start countdown");
    }
}

static void process_segment_stop(ServerState* state, CommandResponse* response) {
    pthread_mutex_lock(&state->state_mutex);
    bool was_counting = state->segment_counting;
    pthread_mutex_unlock(&state->state_mutex);
    
    if (!was_counting) {
        response->status = -1;
        strcpy(response->message, "No countdown in progress");
        return;
    }
    
    if (seg7_stop_counting() == 0) {
        pthread_mutex_lock(&state->state_mutex);
        state->segment_counting = false;
        pthread_mutex_unlock(&state->state_mutex);
        
        response->status = 0;
        strcpy(response->message, "Countdown stopped");
        printf("[Device] Countdown stopped\n");
    } else {
        response->status = -1;
        strcpy(response->message, "Failed to stop countdown");
    }
}

static void handle_sensor_monitoring(ServerState* state) {
    static bool last_bright_state = false;
    bool is_bright = light_sensor_is_bright();
    
    if (is_bright != last_bright_state) {
        pthread_mutex_lock(&state->state_mutex);
        
        if (is_bright) {
            // 밝으면 LED OFF
            if (state->led_on) {
                led_off();
                state->led_on = false;
                printf("[Device] Light detected - LED OFF\n");
            }
        } else {
            // 어두우면 LED ON
            if (!state->led_on) {
                led_on();
                state->led_on = true;
                printf("[Device] Dark detected - LED ON\n");
            }
        }
        
        pthread_mutex_unlock(&state->state_mutex);
        last_bright_state = is_bright;
    }
}

void* device_control_thread(void* arg) {
    ServerState* state = (ServerState*)arg;
    g_state = state;
    
    printf("[Device Thread] Started\n");
    
    while (state->server_running) {
        // Command 처리
        pthread_mutex_lock(&state->queue_mutex);
        
        while (queue_is_empty(&state->cmd_queue) && state->server_running) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1; // 1초 timeout
            
            pthread_cond_timedwait(&state->queue_not_empty, &state->queue_mutex, &ts);
            
            // Timeout 발생 시 sensor monitoring 체크
            pthread_mutex_lock(&state->state_mutex);
            bool monitoring = state->sensor_monitoring;
            pthread_mutex_unlock(&state->state_mutex);
            
            if (monitoring) {
                pthread_mutex_unlock(&state->queue_mutex);
                handle_sensor_monitoring(state);
                pthread_mutex_lock(&state->queue_mutex);
            }
        }
        
        if (!state->server_running) {
            pthread_mutex_unlock(&state->queue_mutex);
            break;
        }
        
        Command* cmd = queue_pop(&state->cmd_queue);
        pthread_mutex_unlock(&state->queue_mutex);
        
        if (!cmd) {
            continue;
        }
        
        // 명령 처리
        CommandResponse response = {0};
        
        switch (cmd->type) {
            case CMD_LED_ON:
                process_led_on(state, &response);
                break;
                
            case CMD_LED_OFF:
                process_led_off(state, &response);
                break;
                
            case CMD_SET_BRIGHTNESS:
                process_set_brightness(state, cmd, &response);
                break;
                
            case CMD_BUZZER_ON:
                process_buzzer_on(state, cmd, &response);
                // 음악 재생은 내부에서 응답을 이미 보냈음
                if (response.status == -2) {
                    free(cmd);
                    continue; // 응답 전달 생략
                }
                break;
                
            case CMD_BUZZER_OFF:
                process_buzzer_off(state, &response);
                break;
                
            case CMD_SENSOR_ON:
                process_sensor_on(state, &response);
                break;
                
            case CMD_SENSOR_OFF:
                process_sensor_off(state, &response);
                break;
                
            case CMD_SEGMENT_DISPLAY:
                process_segment_display(state, cmd, &response);
                break;
                
            case CMD_SEGMENT_STOP:
                process_segment_stop(state, &response);
                break;
                
            default:
                response.status = -1;
                strcpy(response.message, "Unknown command");
                break;
        }
        
        // 응답 전달 (BUZZER_ON의 경우 이미 전달됨)
        if (response.status != -2) {
            pthread_mutex_lock(&state->queue_mutex);
            memcpy(&state->response, &response, sizeof(CommandResponse));
            pthread_cond_signal(&state->response_ready);
            pthread_mutex_unlock(&state->queue_mutex);
        }
        
        free(cmd);
        
        // Sensor monitoring 체크 (명령 처리 후)
        pthread_mutex_lock(&state->state_mutex);
        bool monitoring = state->sensor_monitoring;
        pthread_mutex_unlock(&state->state_mutex);
        
        if (monitoring) {
            handle_sensor_monitoring(state);
        }
    }
    
    printf("[Device Thread] Stopped\n");
    return NULL;
}
