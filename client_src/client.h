#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
    int socket_fd;
    bool connected;
    char server_ip[64];
} ClientState;

int client_connect(ClientState* client, const char* server_ip);
void client_disconnect(ClientState* client);
int client_send_command(ClientState* client, int cmd_type, int param1, int param2);
void client_run(ClientState* client);

#endif // CLIENT_H
