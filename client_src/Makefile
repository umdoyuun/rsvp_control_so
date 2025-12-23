CC = gcc
CFLAGS = -Wall -Wextra -pthread -I. -g
LDFLAGS = -pthread

SRCS = main.c client.c
OBJS = $(SRCS:.c=.o)
TARGET = client

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run

