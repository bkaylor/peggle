
typedef struct Sound_Struct
{
    char *path;
    Uint8 *buffer;
    Uint32 length;
} Sound;

typedef enum
{
    GAME_START = 0,
    BALL_SHOT,
    BALL_HIT,
    BALL_LOST,
    NET_HIT,
    NET_SHOT,
    GAME_LOST,
    GAME_WON,
} Sound_ID;

typedef struct Audio_Struct
{
    SDL_AudioSpec spec;
    SDL_AudioDeviceID device_id;
    Sound sounds[10];
} Audio;

void load_sound(Audio *audio, Sound *sound, char *path)
{
    // sound = (Sound *)calloc(1, sizeof(Sound));
    sound->path = path;

    SDL_LoadWAV(sound->path, &audio->spec, &sound->buffer, &sound->length);
}

void init_and_load_sounds(Audio *audio)
{
    load_sound(audio, &audio->sounds[0], "../assets/game_start.wav");
    load_sound(audio, &audio->sounds[1], "../assets/ball_shot.wav");
    load_sound(audio, &audio->sounds[2], "../assets/ball_hit.wav");
    load_sound(audio, &audio->sounds[3], "../assets/ball_lost.wav");
    load_sound(audio, &audio->sounds[4], "../assets/net_hit.wav");
    load_sound(audio, &audio->sounds[5], "../assets/net_shot.wav");
    load_sound(audio, &audio->sounds[6], "../assets/game_lost.wav");
    load_sound(audio, &audio->sounds[7], "../assets/game_won.wav");

    audio->device_id = SDL_OpenAudioDevice(NULL, 0, &audio->spec, NULL, 0);
}

void play_sound(Audio *audio, Sound_ID sound_id)
{
    Sound sound = audio->sounds[sound_id];
    SDL_QueueAudio(audio->device_id, sound.buffer, sound.length);
    SDL_PauseAudioDevice(audio->device_id, 0);
}
