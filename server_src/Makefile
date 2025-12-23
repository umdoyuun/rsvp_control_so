CC = gcc
CFLAGS = -Wall -Wextra -pthread -I. -g
LDFLAGS = -L. -lled -lbuzzer -llight_sensor -l7segment -lwiringPi -pthread

SRCS = main.c server.c communication.c device_control.c command_queue.c
OBJS = $(SRCS:.c=.o)
TARGET = server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	sudo ./$(TARGET)

.PHONY: all clean run
