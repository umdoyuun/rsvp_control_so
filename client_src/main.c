#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "client.h"

static ClientState g_client;
static volatile sig_atomic_t g_running = 1;

void signal_handler(int signum) {
    printf("\nReceived signal %d, disconnecting...\n", signum);
    g_running = 0;
    client_disconnect(&g_client);
}

int main(int argc, char* argv[]) {
    const char* server_ip = "127.0.0.1"; // 기본값: localhost

    if (argc > 1) {
        server_ip = argv[1];
    }

    // 시그널 핸들러 등록
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("=== IoT Device Control Client ===\n");

    // 서버 연결
    if (client_connect(&g_client, server_ip) != 0) {
        fprintf(stderr, "Failed to connect to server\n");
        return EXIT_FAILURE;
    }

    // 클라이언트 실행
    client_run(&g_client);

    // 연결 종료
    client_disconnect(&g_client);

    return EXIT_SUCCESS;
}

