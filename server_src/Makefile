CC = gcc
CFLAGS = -Wall -Wextra -pthread -I. -g
LDFLAGS = -L. -lled -lbuzzer -llight_sensor -l7segment -lwiringPi -pthread

SRCS = main.c server.c communication.c device_control.c command_queue.c daemon.c
OBJS = $(SRCS:.c=.o)
TARGET = server

# 데몬 설정
DAEMON_PID_FILE = /var/run/iot_server.pid
DAEMON_LOG_FILE = /var/log/iot_server.log

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	sudo ./$(TARGET)

# 데몬 모드로 실행
daemon: $(TARGET)
	sudo ./$(TARGET) -d

# 데몬 중지
stop:
	@if [ -f $(DAEMON_PID_FILE) ]; then \
		sudo kill $$(cat $(DAEMON_PID_FILE)); \
		echo "Server stopped"; \
	else \
		echo "Server is not running"; \
	fi

# 재시작
restart: stop daemon

# 상태 확인
status:
	@if [ -f $(DAEMON_PID_FILE) ]; then \
		PID=$$(cat $(DAEMON_PID_FILE)); \
		if ps -p $$PID > /dev/null 2>&1; then \
			echo "Server is running (PID: $$PID)"; \
			echo ""; \
			ps -p $$PID -o pid,ppid,cmd,etime; \
			echo ""; \
			echo "Web server process:"; \
			ps aux | grep "web_server.py" | grep -v grep || echo "  Not running"; \
		else \
			echo "PID file exists but process is not running"; \
			echo "Cleaning up stale PID file..."; \
			sudo rm -f $(DAEMON_PID_FILE); \
		fi \
	else \
		echo "Server is not running"; \
	fi

# 로그 확인
logs:
	sudo tail -f $(DAEMON_LOG_FILE)

# 로그 지우기
clean-logs:
	sudo rm -f $(DAEMON_LOG_FILE)

# 완전 정리
distclean: clean stop clean-logs
	@echo "All cleaned up"

.PHONY: all clean run daemon stop restart status logs clean-logs distclean
