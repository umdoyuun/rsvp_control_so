#include "buzzer.h"
#include <wiringPi.h>
#include <softTone.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

// 음계 정의
#define DO      261.63
#define RE      293.66
#define MI      329.63
#define FA      349.23
#define SOL     391.00
#define LA      440.00
#define SI      493.88
#define DO_H    523.25
#define REST    0

static int school_bell[] = {
    SOL, SOL, LA, LA, SOL, SOL, MI, MI,
    SOL, SOL, MI, MI, RE, RE, RE, REST,
    SOL, SOL, LA, LA, SOL, SOL, MI, MI,
    SOL, MI, RE, MI, DO, DO, DO, REST
};
static int school_bell_length = 32;

static int twinkle_star[] = {
    DO, DO, SOL, SOL, LA, LA, SOL, REST,
    FA, FA, MI, MI, RE, RE, DO, REST,
    SOL, SOL, FA, FA, MI, MI, RE, REST,
    SOL, SOL, FA, FA, MI, MI, RE, REST,
    DO, DO, SOL, SOL, LA, LA, SOL, REST,
    FA, FA, MI, MI, RE, RE, DO, REST
};
static int twinkle_star_length = 48;

static int happy_birthday[] = {
    SOL, SOL, LA, SOL, DO_H, SI, REST,
    SOL, SOL, LA, SOL, RE, DO_H, REST,
    SOL, SOL, SOL, MI, DO_H, SI, LA, REST,
    FA, FA, MI, DO_H, RE, DO_H, REST
};
static int happy_birthday_length = 28;

static int butterfly[] = {
    DO, RE, MI, FA, MI, RE, DO, REST,
    MI, FA, SOL, SOL, MI, FA, SOL, SOL,
    DO_H, SOL, MI, SOL, FA, MI, RE, DO,
    DO, RE, MI, FA, MI, RE, DO, REST
};
static int butterfly_length = 32;

static int initialized = 0;
static int SPKR = -1;

static pthread_t music_thread;
static pthread_mutex_t music_mutex = PTHREAD_MUTEX_INITIALIZER;
static int is_playing = 0;
static int should_stop = 0;

typedef struct {
    int* notes;
    int length;
    int tempo;
} MusicData;


static int play_notes_interruptible(int* notes, int length, int tempo)
{
    for (int i = 0; i < length; i++) {
        pthread_mutex_lock(&music_mutex);
        if (should_stop) {
            softToneWrite(SPKR, 0);
            pthread_mutex_unlock(&music_mutex);
            return -1; // 중단됨
        }
        pthread_mutex_unlock(&music_mutex);

        softToneWrite(SPKR, notes[i]);
        
        int delay_step = 50; // 50ms 단위로 체크
        int remaining = tempo;
        
        while (remaining > 0) {
            int sleep_time = (remaining > delay_step) ? delay_step : remaining;
            delay(sleep_time);
            remaining -= sleep_time;
            
            pthread_mutex_lock(&music_mutex);
            if (should_stop) {
                softToneWrite(SPKR, 0);
                pthread_mutex_unlock(&music_mutex);
                return -1; // 중단됨
            }
            pthread_mutex_unlock(&music_mutex);
        }
    }
    
    softToneWrite(SPKR, 0);
    return 0; // 정상 완료
}

static void* music_playback_thread(void* arg)
{
    MusicData* data = (MusicData*)arg;

    printf("[Buzzer] Music playback started\n");
    int result = play_notes_interruptible(data->notes, data->length, data->tempo);

    pthread_mutex_lock(&music_mutex);
    is_playing = 0;
    should_stop = 0;
    pthread_mutex_unlock(&music_mutex);

    if (result == 0) {
        printf("[Buzzer] Music playback completed normally\n");
    } else {
        printf("[Buzzer] Music playback stopped\n");
    }

    free(data);
    return NULL;
}

int music_init(int speaker_pin)
{
    if (initialized) {
        return 0;
    }

    if (speaker_pin < 0) {
        fprintf(stderr, "Invalid speaker pin: %d\n", speaker_pin);
        return -1;
    }

    SPKR = speaker_pin;

    if (softToneCreate(SPKR) != 0) {
        fprintf(stderr, "softTone 초기화 실패\n");
        return -1;
    }

    initialized = 1;
    printf("Buzzer initialized (GPIO pin: %d)\n", SPKR);
    return 0;
}

void music_cleanup(void)
{
    if (initialized) {
        stop_music(); 
        
        pthread_mutex_lock(&music_mutex);
        int playing = is_playing;
        pthread_mutex_unlock(&music_mutex);
        
        if (playing) {
            pthread_join(music_thread, NULL);
        }
        
        softToneWrite(SPKR, 0);
        initialized = 0;
        printf("Buzzer cleaned up\n");
    }
}

int play_music_async(int music_number)
{
    if (!initialized) {
        fprintf(stderr, "Buzzer not initialized. Call music_init() first.\n");
        return -1;
    }

    pthread_mutex_lock(&music_mutex);
    
    // 이미 재생 중이면 거부
    if (is_playing) {
        pthread_mutex_unlock(&music_mutex);
        fprintf(stderr, "Music already playing. Stop it first.\n");
        return -1;
    }
    
    MusicData* data = (MusicData*)malloc(sizeof(MusicData));
    if (!data) {
        pthread_mutex_unlock(&music_mutex);
        return -1;
    }
    
    // 음악 데이터 설정
    switch(music_number) {
        case MUSIC_SCHOOL_BELL:
            data->notes = school_bell;
            data->length = school_bell_length;
            data->tempo = 280;
            break;
        case MUSIC_TWINKLE_STAR:
            data->notes = twinkle_star;
            data->length = twinkle_star_length;
            data->tempo = 280;
            break;
        case MUSIC_HAPPY_BIRTHDAY:
            data->notes = happy_birthday;
            data->length = happy_birthday_length;
            data->tempo = 350;
            break;
        case MUSIC_BUTTERFLY:
            data->notes = butterfly;
            data->length = butterfly_length;
            data->tempo = 280;
            break;
        default:
            free(data);
            pthread_mutex_unlock(&music_mutex);
            fprintf(stderr, "잘못된 음악 번호: %d\n", music_number);
            return -1;
    }
    
    is_playing = 1;
    should_stop = 0;
    
    // 스레드 생성
    if (pthread_create(&music_thread, NULL, music_playback_thread, data) != 0) {
        is_playing = 0;
        free(data);
        pthread_mutex_unlock(&music_mutex);
        fprintf(stderr, "Failed to create music thread\n");
        return -1;
    }
    
    pthread_detach(music_thread); // 자동 정리
    pthread_mutex_unlock(&music_mutex);
    
    return 0;
}

int stop_music(void)
{
    pthread_mutex_lock(&music_mutex);
    
    if (!is_playing) {
        pthread_mutex_unlock(&music_mutex);
        return -1;
    }
    
    should_stop = 1;
    pthread_mutex_unlock(&music_mutex);
    
    return 0;
}

int is_music_playing(void)
{
    pthread_mutex_lock(&music_mutex);
    int playing = is_playing;
    pthread_mutex_unlock(&music_mutex);
    
    return playing;
}
