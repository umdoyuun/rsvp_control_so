#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include "server.h"

int daemonize(void) {
    pid_t pid, sid;
    
    // 1. 부모 프로세스 종료 (첫 번째 fork)
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    
    if (pid > 0) {
        // 부모 프로세스 종료
        exit(EXIT_SUCCESS);
    }
    
    // 2. 새로운 세션 생성 (터미널에서 분리)
    sid = setsid();
    if (sid < 0) {
        perror("setsid");
        return -1;
    }
    
    // 3. 시그널 무시 설정
    signal(SIGHUP, SIG_IGN);
    
    // 4. 두 번째 fork (세션 리더 방지)
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    
    if (pid > 0) {
        // 자식 프로세스도 종료
        exit(EXIT_SUCCESS);
    }
    
    // 5. 작업 디렉토리 변경 제거 (main에서 복원하므로)
    // chdir("/")를 호출하지 않음
    
    // 6. 파일 생성 마스크 설정
    umask(0);
    
    // 7. 표준 입출력 파일 디스크립터 닫기
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // 8. /dev/null로 리다이렉트
    int fd = open("/dev/null", O_RDWR);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        
        if (fd > 2) {
            close(fd);
        }
    }
    
    return 0;
}

/**
 * PID 파일 작성
 */
int write_pid_file(const char* pidfile) {
    FILE* fp;
    pid_t pid;
    
    // 기존 PID 파일 확인
    fp = fopen(pidfile, "r");
    if (fp != NULL) {
        if (fscanf(fp, "%d", &pid) == 1) {
            // 프로세스가 실행 중인지 확인
            if (kill(pid, 0) == 0) {
                fclose(fp);
                fprintf(stderr, "Server already running (PID: %d)\n", pid);
                return -1;
            }
        }
        fclose(fp);
    }
    
    // 새로운 PID 파일 작성
    fp = fopen(pidfile, "w");
    if (fp == NULL) {
        perror("fopen");
        return -1;
    }
    
    fprintf(fp, "%d\n", getpid());
    fclose(fp);
    
    return 0;
}

int remove_pid_file(const char* pidfile) {
    if (unlink(pidfile) < 0) {
        if (errno != ENOENT) {
            perror("unlink");
            return -1;
        }
    }
    return 0;
}

void redirect_output_to_log(const char* logfile) {
    int fd;
    
    // 로그 파일 열기 (없으면 생성, 있으면 추가)
    fd = open(logfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }
    
    // stdout, stderr를 로그 파일로 리다이렉트
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    
    if (fd > 2) {
        close(fd);
    }
    
    // 버퍼링 비활성화 (즉시 로그 기록)
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);
}
