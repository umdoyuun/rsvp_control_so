#include <stdlib.h>
#include <string.h>
#include "server.h"

void queue_init(CommandQueue* queue) {
    queue->front = 0;
    queue->rear = 0;
    queue->count = 0;
    memset(queue->commands, 0, sizeof(queue->commands));
}

bool queue_push(CommandQueue* queue, Command* cmd) {
    if (queue->count >= MAX_QUEUE_SIZE) {
        return false;
    }
    
    Command* new_cmd = (Command*)malloc(sizeof(Command));
    if (!new_cmd) {
        return false;
    }
    
    memcpy(new_cmd, cmd, sizeof(Command));
    queue->commands[queue->rear] = new_cmd;
    queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
    queue->count++;
    
    return true;
}

Command* queue_pop(CommandQueue* queue) {
    if (queue->count == 0) {
        return NULL;
    }
    
    Command* cmd = queue->commands[queue->front];
    queue->commands[queue->front] = NULL;
    queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    queue->count--;
    
    return cmd;
}

bool queue_is_empty(CommandQueue* queue) {
    return queue->count == 0;
}

void queue_cleanup(CommandQueue* queue) {
    while (!queue_is_empty(queue)) {
        Command* cmd = queue_pop(queue);
        if (cmd) {
            free(cmd);
        }
    }
}
