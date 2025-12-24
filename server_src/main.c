#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.h"

static ServerState g_server_state;
static volatile sig_atomic_t g_running = 1;

void signal_handler(int signum) {
    printf("\nReceived signal %d, shutting down...\n", signum);
    g_running = 0;
    g_server_state.server_running = false;
}

int main(void) {
    // 시그널 핸들러 등록
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("=== IoT Device Control Server ===\n");
    
    // 서버 초기화
    if (server_init(&g_server_state) != 0) {
        fprintf(stderr, "Failed to initialize server\n");
        return EXIT_FAILURE;
    }
    
    // 웹 서버 시작
    printf("\nStarting web camera server...\n");
    g_server_state.web_server_pid = start_web_server(WEB_SERVER_PORT);
    
    if (g_server_state.web_server_pid < 0) {
        fprintf(stderr, "Failed to start web server (continuing without camera)\n");
    } else {
        printf("✓ Camera server running at http://%s:%d\n\n", 
               g_server_state.server_ip, WEB_SERVER_PORT);
    }
    
    // Device Control Thread 생성
    if (pthread_create(&g_server_state.device_thread, NULL, 
                       device_control_thread, &g_server_state) != 0) {
        fprintf(stderr, "Failed to create device control thread\n");
        if (g_server_state.web_server_pid > 0) {
            stop_web_server(g_server_state.web_server_pid);
        }
        server_cleanup(&g_server_state);
        return EXIT_FAILURE;
    }
    
    // 클라이언트 연결 대기 및 처리
    while (g_running && g_server_state.server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        printf("Waiting for client connection...\n");
        if (g_server_state.web_server_pid > 0) {
            printf("Camera Monitor: http://%s:%d\n", 
                   g_server_state.server_ip, WEB_SERVER_PORT);
        }
        printf("\n");
        
        int client_socket = accept(g_server_state.server_socket, 
                                   (struct sockaddr*)&client_addr, 
                                   &client_len);
        
        if (client_socket < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 논블로킹 소켓: 연결 없음, 계속 대기
                usleep(500000); // 500ms 대기
                continue;
            } else if (g_running) {
                perror("accept");
            }
            break;
        }
        
        pthread_mutex_lock(&g_server_state.state_mutex);
        
        if (g_server_state.client_connected) {
            pthread_mutex_unlock(&g_server_state.state_mutex);
            
            const char* busy_msg = "Server busy - another client is connected\n";
            send(client_socket, busy_msg, strlen(busy_msg), 0);
            close(client_socket);
            
            printf("Rejected connection - server busy\n");
            continue;
        }
        
        g_server_state.client_socket = client_socket;
        g_server_state.client_connected = true;
        
        pthread_mutex_unlock(&g_server_state.state_mutex);
        
        printf("Client connected from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));
        
        // Communication Thread 생성
        if (pthread_create(&g_server_state.comm_thread, NULL, 
                          communication_thread, &g_server_state) != 0) {
            fprintf(stderr, "Failed to create communication thread\n");
            
            pthread_mutex_lock(&g_server_state.state_mutex);
            close(client_socket);
            g_server_state.client_socket = -1;
            g_server_state.client_connected = false;
            pthread_mutex_unlock(&g_server_state.state_mutex);
            
            continue;
        }
        
        // Communication Thread 종료 대기
        pthread_join(g_server_state.comm_thread, NULL);
        
        printf("Client disconnected\n\n");
    }
    
    // 서버 정리
    server_cleanup(&g_server_state);
    
    printf("Server stopped\n");
    return EXIT_SUCCESS;
}
