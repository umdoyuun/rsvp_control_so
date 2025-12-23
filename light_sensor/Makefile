CC = gcc
CFLAGS = -Wall -fPIC
LDFLAGS = -lwiringPi -lpthread

# 라이브러리 이름
LIB_NAME = liblight_sensor
LIB_SO = $(LIB_NAME).so
LIB_VERSION = 1.0

# 소스 파일
LIB_SRC = light_sensor.c
LIB_OBJ = $(LIB_SRC:.c=.o)

# 테스트 프로그램
TEST_PROG = test_light_sensor
TEST_SRC = test_light_sensor.c

all: $(LIB_SO) $(TEST_PROG)

# 동적 라이브러리 빌드
$(LIB_SO): $(LIB_OBJ)
	$(CC) -shared -Wl,-soname,$(LIB_SO).$(LIB_VERSION) -o $(LIB_SO) $(LIB_OBJ) $(LDFLAGS)

# 오브젝트 파일 빌드
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 테스트 프로그램 빌드
$(TEST_PROG): $(TEST_SRC) $(LIB_SO)
	$(CC) $(CFLAGS) -o $(TEST_PROG) $(TEST_SRC) -L. -llight_sensor $(LDFLAGS)

clean:
	rm -f *.o $(LIB_SO) $(TEST_PROG)

install:
	sudo cp $(LIB_SO) /usr/local/lib/
	sudo cp light_sensor.h /usr/local/include/
	sudo ldconfig

uninstall:
	sudo rm -f /usr/local/lib/$(LIB_SO)
	sudo rm -f /usr/local/include/light_sensor.h
	sudo ldconfig

.PHONY: all clean install uninstall
