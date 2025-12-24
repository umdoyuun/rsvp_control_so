#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.h"

static ServerState g_server_state;
static volatile sig_atomic_t g_running = 1;

void signal_handler(int signum) {
    if (signum == SIGINT) {
        printf("\n[Signal] Received SIGINT (Ctrl+C), shutting down...\n");
        g_running = 0;
        g_server_state.server_running = false;
    }
}

void sigchld_handler(int signum) {
    (void)signum;
    // 좀비 프로세스 방지 (웹 서버가 종료되었을 때)
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void setup_signal_handlers(void) {
    struct sigaction sa_int, sa_chld, sa_ignore;
    
    // SIGINT (Ctrl+C) 핸들러 설정
    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = signal_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }
    
    // SIGCHLD 핸들러 설정 (자식 프로세스 종료 처리)
    memset(&sa_chld, 0, sizeof(sa_chld));
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        perror("sigaction SIGCHLD");
        exit(EXIT_FAILURE);
    }
    
    // 다른 신호들은 무시
    memset(&sa_ignore, 0, sizeof(sa_ignore));
    sa_ignore.sa_handler = SIG_IGN;
    sigemptyset(&sa_ignore.sa_mask);
    sa_ignore.sa_flags = 0;
    
    // SIGTERM 무시 (kill 명령으로 종료 방지)
    sigaction(SIGTERM, &sa_ignore, NULL);
    
    // SIGHUP 무시 (터미널 종료 시에도 계속 실행)
    sigaction(SIGHUP, &sa_ignore, NULL);
    
    // SIGQUIT 무시 (Ctrl+\ 종료 방지)
    sigaction(SIGQUIT, &sa_ignore, NULL);
    
    // SIGPIPE 무시 (소켓 끊김 시 종료 방지)
    sigaction(SIGPIPE, &sa_ignore, NULL);
    
    printf("[Signal] Signal handlers configured:\n");
    printf("  - SIGINT (Ctrl+C): Graceful shutdown\n");
    printf("  - SIGTERM: Ignored\n");
    printf("  - SIGHUP: Ignored\n");
    printf("  - SIGQUIT: Ignored\n");
    printf("  - SIGPIPE: Ignored\n");
    printf("  - SIGCHLD: Zombie prevention\n");
    printf("\n");
}

int main(void) {
    printf("=== IoT Device Control Server ===\n\n");
    
    // 신호 핸들러 설정
    setup_signal_handlers();
    
    // 서버 초기화
    if (server_init(&g_server_state) != 0) {
        fprintf(stderr, "Failed to initialize server\n");
        return EXIT_FAILURE;
    }
    
    // 웹 서버 시작
    printf("Starting web camera server...\n");
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
    
    printf("═══════════════════════════════════════════════\n");
    printf("  Server is running. Press Ctrl+C to stop.\n");
    printf("  Other signals (TERM, HUP, QUIT) are ignored.\n");
    printf("═══════════════════════════════════════════════\n\n");
    
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
            } else if (errno == EINTR) {
                // 시그널 인터럽트 - 정상, 계속 진행
                printf("[Info] accept() interrupted by signal, continuing...\n");
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
    printf("\n[Cleanup] Starting server cleanup...\n");
    server_cleanup(&g_server_state);
    
    printf("[Cleanup] Server stopped successfully\n");
    return EXIT_SUCCESS;
}
