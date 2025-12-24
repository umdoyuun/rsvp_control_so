#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <limits.h>
#include "server.h"

static ServerState g_server_state;
static volatile sig_atomic_t g_running = 1;
static char g_working_dir[PATH_MAX];

// 로그 출력 함수 (시간 포함)
void log_message(const char* level, const char* format, ...) {
    time_t now;
    struct tm *tm_info;
    char time_buffer[26];
    
    time(&now);
    tm_info = localtime(&now);
    strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("[%s] [%s] ", time_buffer, level);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
}

void signal_handler(int signum) {
    if (signum == SIGINT) {
        log_message("SIGNAL", "Received SIGINT (Ctrl+C), shutting down...");
        g_running = 0;
        g_server_state.server_running = false;
    }
}

void sigchld_handler(int signum) {
    (void)signum;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void setup_signal_handlers(void) {
    struct sigaction sa_int, sa_chld, sa_ignore;
    
    // SIGINT 핸들러
    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = signal_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }
    
    // SIGCHLD 핸들러
    memset(&sa_chld, 0, sizeof(sa_chld));
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        perror("sigaction SIGCHLD");
        exit(EXIT_FAILURE);
    }
    
    // 다른 신호 무시
    memset(&sa_ignore, 0, sizeof(sa_ignore));
    sa_ignore.sa_handler = SIG_IGN;
    sigemptyset(&sa_ignore.sa_mask);
    sa_ignore.sa_flags = 0;
    
    sigaction(SIGTERM, &sa_ignore, NULL);
    sigaction(SIGHUP, &sa_ignore, NULL);
    sigaction(SIGQUIT, &sa_ignore, NULL);
    sigaction(SIGPIPE, &sa_ignore, NULL);
    
    log_message("INFO", "Signal handlers configured");
    log_message("INFO", "  - SIGINT (Ctrl+C): Graceful shutdown");
    log_message("INFO", "  - SIGTERM, SIGHUP, SIGQUIT, SIGPIPE: Ignored");
    log_message("INFO", "  - SIGCHLD: Zombie prevention");
}

void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\n");
    printf("Options:\n");
    printf("  -d, --daemon     Run as daemon process\n");
    printf("  -h, --help       Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s              Run in foreground\n", program_name);
    printf("  %s -d           Run as daemon\n", program_name);
    printf("\n");
    printf("Daemon control:\n");
    printf("  Start:   sudo %s -d\n", program_name);
    printf("  Stop:    sudo kill $(cat %s)\n", DAEMON_PID_FILE);
    printf("  Status:  ps aux | grep %s\n", program_name);
    printf("  Logs:    tail -f %s\n", DAEMON_LOG_FILE);
    printf("\n");
}

int main(int argc, char* argv[]) {
    bool daemon_mode = false;
    
    // 현재 작업 디렉토리 저장
    if (getcwd(g_working_dir, sizeof(g_working_dir)) == NULL) {
        perror("getcwd");
        return EXIT_FAILURE;
    }
    
    // 명령행 인자 파싱
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--daemon") == 0) {
            daemon_mode = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }
    
    // 데몬 모드로 실행
    if (daemon_mode) {
        printf("Starting IoT Server as daemon...\n");
        printf("Working directory: %s\n", g_working_dir);
        printf("Logs will be written to: %s\n", DAEMON_LOG_FILE);
        printf("PID will be written to: %s\n", DAEMON_PID_FILE);
        
        if (daemonize() < 0) {
            fprintf(stderr, "Failed to daemonize\n");
            return EXIT_FAILURE;
        }
        
        // 작업 디렉토리 복원
        if (chdir(g_working_dir) < 0) {
            // 로그가 아직 리다이렉트 안 되어서 perror 사용
            perror("chdir");
            return EXIT_FAILURE;
        }
        
        // 로그 파일로 리다이렉트
        redirect_output_to_log(DAEMON_LOG_FILE);
        
        // PID 파일 작성
        if (write_pid_file(DAEMON_PID_FILE) < 0) {
            log_message("ERROR", "Failed to write PID file (server may be already running)");
            return EXIT_FAILURE;
        }
        
        log_message("INFO", "Daemon process started (PID: %d)", getpid());
        log_message("INFO", "Working directory: %s", g_working_dir);
    } else {
        printf("=== IoT Device Control Server ===\n");
        printf("Running in foreground mode\n");
        printf("Working directory: %s\n", g_working_dir);
        printf("Use -d option to run as daemon\n\n");
    }
    
    // 신호 핸들러 설정
    setup_signal_handlers();
    
    // 서버 초기화
    if (server_init(&g_server_state) != 0) {
        log_message("ERROR", "Failed to initialize server");
        if (daemon_mode) {
            remove_pid_file(DAEMON_PID_FILE);
        }
        return EXIT_FAILURE;
    }
    
    // 웹 서버 시작
    log_message("INFO", "Starting web camera server...");
    g_server_state.web_server_pid = start_web_server(WEB_SERVER_PORT);
    
    if (g_server_state.web_server_pid < 0) {
        log_message("WARN", "Failed to start web server (continuing without camera)");
    } else {
        log_message("INFO", "Camera server running at http://%s:%d", 
                   g_server_state.server_ip, WEB_SERVER_PORT);
    }
    
    // Device Control Thread 생성
    if (pthread_create(&g_server_state.device_thread, NULL, 
                       device_control_thread, &g_server_state) != 0) {
        log_message("ERROR", "Failed to create device control thread");
        if (g_server_state.web_server_pid > 0) {
            stop_web_server(g_server_state.web_server_pid);
        }
        server_cleanup(&g_server_state);
        if (daemon_mode) {
            remove_pid_file(DAEMON_PID_FILE);
        }
        return EXIT_FAILURE;
    }
    
    log_message("INFO", "Server is running. Press Ctrl+C to stop.");
    log_message("INFO", "Listening on port %d", SERVER_PORT);
    
    // 클라이언트 연결 대기 및 처리
    while (g_running && g_server_state.server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(g_server_state.server_socket, 
                                   (struct sockaddr*)&client_addr, 
                                   &client_len);
        
        if (client_socket < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(500000);
                continue;
            } else if (errno == EINTR) {
                continue;
            } else if (g_running) {
                log_message("ERROR", "accept failed: %s", strerror(errno));
            }
            break;
        }
        
        pthread_mutex_lock(&g_server_state.state_mutex);
        
        if (g_server_state.client_connected) {
            pthread_mutex_unlock(&g_server_state.state_mutex);
            
            const char* busy_msg = "Server busy - another client is connected\n";
            send(client_socket, busy_msg, strlen(busy_msg), 0);
            close(client_socket);
            
            log_message("WARN", "Rejected connection - server busy");
            continue;
        }
        
        g_server_state.client_socket = client_socket;
        g_server_state.client_connected = true;
        
        pthread_mutex_unlock(&g_server_state.state_mutex);
        
        log_message("INFO", "Client connected from %s:%d", 
                   inet_ntoa(client_addr.sin_addr), 
                   ntohs(client_addr.sin_port));
        
        if (pthread_create(&g_server_state.comm_thread, NULL, 
                          communication_thread, &g_server_state) != 0) {
            log_message("ERROR", "Failed to create communication thread");
            
            pthread_mutex_lock(&g_server_state.state_mutex);
            close(client_socket);
            g_server_state.client_socket = -1;
            g_server_state.client_connected = false;
            pthread_mutex_unlock(&g_server_state.state_mutex);
            
            continue;
        }
        
        pthread_join(g_server_state.comm_thread, NULL);
        
        log_message("INFO", "Client disconnected");
    }
    
    // 서버 정리
    log_message("INFO", "Starting server cleanup...");
    server_cleanup(&g_server_state);
    
    // PID 파일 삭제
    if (daemon_mode) {
        remove_pid_file(DAEMON_PID_FILE);
    }
    
    log_message("INFO", "Server stopped successfully");
    return EXIT_SUCCESS;
}
