# Makefile for 7-Segment Dynamic Library

CC = gcc
CFLAGS = -Wall -Wextra -fPIC -O2
LDFLAGS = -shared -lwiringPi -lpthread

# 라이브러리 이름
LIB_NAME = lib7segment.so
LIB_VERSION = 1.0.0

# 소스 파일
SRC = 7segment.c
OBJ = $(SRC:.c=.o)
HEADER = 7segment.h

# 설치 경로
INSTALL_LIB_DIR = /usr/local/lib
INSTALL_INCLUDE_DIR = /usr/local/include

.PHONY: all clean install uninstall test example

all: $(LIB_NAME)

# 동적 라이브러리 생성
$(LIB_NAME): $(OBJ)
	@echo "Building shared library: $@"
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "Done!"

# Object 파일 생성
%.o: %.c $(HEADER)
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# 테스트 프로그램 빌드
test: $(LIB_NAME) test_7segment.c
	@echo "Building test program..."
	$(CC) -Wall -O2 test_7segment.c -L. -l7segment -lwiringPi -lpthread -Wl,-rpath,. -o test_7segment
	@echo "Run with: sudo ./test_7segment"

# 예제 프로그램 빌드
example: $(LIB_NAME) example.c
	@echo "Building example program..."
	$(CC) -Wall -O2 example.c -L. -l7segment -lwiringPi -lpthread -Wl,-rpath,. -o example
	@echo "Run with: sudo ./example"

# 시스템에 설치
install: $(LIB_NAME)
	@echo "Installing library to $(INSTALL_LIB_DIR)..."
	sudo cp $(LIB_NAME) $(INSTALL_LIB_DIR)/
	sudo cp $(HEADER) $(INSTALL_INCLUDE_DIR)/
	sudo ldconfig
	@echo "Installation complete!"
	@echo "Use: #include <7segment.h> and link with -l7segment"

# 제거
uninstall:
	@echo "Removing library..."
	sudo rm -f $(INSTALL_LIB_DIR)/$(LIB_NAME)
	sudo rm -f $(INSTALL_INCLUDE_DIR)/$(HEADER)
	sudo ldconfig
	@echo "Uninstallation complete!"

# 정리
clean:
	@echo "Cleaning up..."
	rm -f $(OBJ) $(LIB_NAME) test_7segment example
	@echo "Clean complete!"

# 도움말
help:
	@echo "7-Segment Library Makefile"
	@echo ""
	@echo "Usage:"
	@echo "  make              - Build the shared library"
	@echo "  make test         - Build test program"
	@echo "  make example      - Build example program"
	@echo "  make install      - Install library to system"
	@echo "  make uninstall    - Remove library from system"
	@echo "  make clean        - Remove build files"
	@echo ""
	@echo "After installation, compile programs with:"
	@echo "  gcc your_program.c -l7segment -lwiringPi -lpthread -o your_program"
