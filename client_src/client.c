#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "client.h"

int client_connect(ClientState* client, const char* server_ip) {
    memset(client, 0, sizeof(ClientState));
    strncpy(client->server_ip, server_ip, sizeof(client->server_ip) - 1);

    // 소켓 생성
    client->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->socket_fd < 0) {
        perror("socket");
        return -1;
    }

    // 서버 주소 설정
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server IP address: %s\n", server_ip);
        close(client->socket_fd);
        return -1;
    }

    // 서버 연결
    printf("Connecting to %s:%d...\n", server_ip, SERVER_PORT);
    if (connect(client->socket_fd, (struct sockaddr*)&server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect");
        close(client->socket_fd);
        return -1;
    }

    client->connected = true;
    printf("Connected to server!\n\n");

    // 환영 메시지 수신
    char buffer[BUFFER_SIZE];
    ssize_t received = recv(client->socket_fd, buffer, sizeof(buffer) - 1, 0);
    if (received > 0) {
        buffer[received] = '\0';
        printf("%s", buffer);
    }

    return 0;
}

void client_disconnect(ClientState* client) {
    if (client->connected) {
        close(client->socket_fd);
        client->connected = false;
        printf("\nDisconnected from server\n");
    }
}

int client_send_command(ClientState* client, int cmd_type, int param1, int param2) {
    if (!client->connected) {
        fprintf(stderr, "Not connected to server\n");
        return -1;
    }

    char buffer[BUFFER_SIZE];

    // 명령 전송
    snprintf(buffer, sizeof(buffer), "%d %d %d\n", cmd_type, param1, param2);
    ssize_t sent = send(client->socket_fd, buffer, strlen(buffer), 0);
    if (sent <= 0) {
        perror("send");
        return -1;
    }

    return 0;
}

static void print_menu(void) {
    printf("\n[ Device Control Menu ]\n");
    printf("1. LED ON\n");
    printf("2. LED OFF\n");
    printf("3. Set Brightness\n");
    printf("4. BUZZER ON (play melody)\n");
    printf("5. BUZZER OFF (stop)\n");
    printf("6. SENSOR ON (감시 시작)\n");
    printf("7. SENSOR OFF (감시 종료)\n");
    printf("8. SEGMENT DISPLAY (숫자 표시 후 카운트다운)\n");
    printf("9. SEGMENT STOP (카운트다운 중단)\n");
    printf("0. Exit\n");
    printf("Select: ");
    fflush(stdout);
}

void client_run(ClientState* client) {
    char buffer[BUFFER_SIZE];
    int choice;

    while (client->connected) {
        // 서버로부터 메뉴 수신 (있는 경우)
        fd_set read_fds;
        struct timeval tv;
        FD_ZERO(&read_fds);
        FD_SET(client->socket_fd, &read_fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms

        int ready = select(client->socket_fd + 1, &read_fds, NULL, NULL, &tv);
        if (ready > 0) {
            ssize_t received = recv(client->socket_fd, buffer, sizeof(buffer) - 1, 0);
            if (received > 0) {
                buffer[received] = '\0';
                printf("%s", buffer);

                // "Select:" 프롬프트가 오면 사용자 입력 대기
                if (strstr(buffer, "Select:") != NULL) {
                    // 사용자 입력
                    if (scanf("%d", &choice) != 1) {
                        // 입력 버퍼 클리어
                        while (getchar() != '\n');
                        printf("Invalid input\n");
                        continue;
                    }

                    // 명령 전송
                    int param1 = 0, param2 = 0;

                    if (choice == 0) {
                        // Exit
                        client_send_command(client, choice, 0, 0);
                        break;
                    } else if (choice == 3) {
                        // Set Brightness - 파라미터 필요 없음 (서버에서 요청)
                        client_send_command(client, choice, 0, 0);
                    } else if (choice == 4) {
                        // Buzzer ON - 파라미터 필요 없음 (서버에서 요청)
                        client_send_command(client, choice, 0, 0);
                    } else if (choice == 8) {
                        // Segment Display - 파라미터 필요 없음 (서버에서 요청)
                        client_send_command(client, choice, 0, 0);
                    } else if (choice >= 1 && choice <= 9) {
                        // 다른 명령들
                        client_send_command(client, choice, param1, param2);
                    } else {
                        printf("Invalid choice\n");
                    }

                } else if (strstr(buffer, "Enter") != NULL) {
                    // 추가 입력 요청 (brightness, music number, countdown seconds)
                    int param;
                    if (scanf("%d", &param) != 1) {
                        while (getchar() != '\n');
                        printf("Invalid input\n");
                        continue;
                    }

                    snprintf(buffer, sizeof(buffer), "%d\n", param);
                    send(client->socket_fd, buffer, strlen(buffer), 0);
                }

                fflush(stdout);
            } else if (received == 0) {
                printf("Server disconnected\n");
                break;
            } else {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("recv");
                    break;
                }
            }
        }
    }
}

