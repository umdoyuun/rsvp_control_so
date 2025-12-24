#ifndef BUZZER_H
#define BUZZER_H

#define MUSIC_SCHOOL_BELL       1
#define MUSIC_TWINKLE_STAR      2
#define MUSIC_HAPPY_BIRTHDAY    3
#define MUSIC_BUTTERFLY         4

int music_init(int speaker_pin);
void music_cleanup(void);
int play_music_async(int music_number);
int stop_music(void);
int is_music_playing(void);

#endif
