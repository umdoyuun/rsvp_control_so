#include "buzzer.h"
#include <wiringPi.h>
#include <softTone.h>
#include <stdio.h>

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

static void play_notes(int* notes, int length, int tempo)
{
    for (int i = 0; i < length; i++) {
        softToneWrite(SPKR, notes[i]);
        delay(tempo);
    }
    softToneWrite(SPKR, 0);
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
        softToneWrite(SPKR, 0);
        initialized = 0;
        printf("Buzzer cleaned up\n");
    }
}

int play_music(int music_number)
{
    if (!initialized) {
        fprintf(stderr, "Buzzer not initialized. Call music_init() first.\n");
        return -1;
    }

    switch(music_number) {
        case MUSIC_SCHOOL_BELL:
            play_notes(school_bell, school_bell_length, 280);
            break;
        case MUSIC_TWINKLE_STAR:
            play_notes(twinkle_star, twinkle_star_length, 280);
            break;
        case MUSIC_HAPPY_BIRTHDAY:
            play_notes(happy_birthday, happy_birthday_length, 350);
            break;
        case MUSIC_BUTTERFLY:
            play_notes(butterfly, butterfly_length, 280);
            break;
        default:
            fprintf(stderr, "잘못된 음악 번호: %d\n", music_number);
            return -1;
    }

    return 0;
}
