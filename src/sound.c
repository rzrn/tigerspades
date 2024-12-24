/*
    Copyright (c) 2017-2020 ByteBit

    This file is part of BetterSpades.

    BetterSpades is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    BetterSpades is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with BetterSpades.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <bs/common.h>
#include <bs/sound.h>
#include <bs/config.h>
#include <bs/camera.h>
#include <bs/entitysystem.h>

#include <log.h>

#ifdef USE_SOUND
    int sound_enabled = 1;
#else
    int sound_enabled = 0;
#endif

#ifdef USE_SOUND
    #if __APPLE__
        #include <OpenAL/al.h>
        #include <OpenAL/alc.h>
    #else
        #include <AL/al.h>
        #include <AL/alc.h>
    #endif
#endif

#ifdef USE_SOUND
    #include <dr_wav.h>
#endif

typedef struct {
#ifdef USE_SOUND
    ALuint openal_handle;
    char local;
    int stick_to_player;
#else
    void * dummy;
#endif
} SoundSource;

EntitySystem sound_sources;

struct _WAV {
#ifdef USE_SOUND
    ALuint openal_buffer;
    float min, max;
#else
    void * dummy;
#endif
};

#define SOUND_TOTAL (SOUND_LAST + 1)
WAV _sounds[SOUND_TOTAL];

WAV * sound(enum WAV index) { return &_sounds[index]; }

typedef struct {
    const char * filename;
    float min, max;
} Resource;

#ifdef USE_SOUND
static Resource wav_sound(enum WAV index) {
    switch (index) {
        case SOUND_CHAT:           return (Resource) {"wav/chat.wav", 0.1F, 1024.0F};
        case SOUND_FOOTSTEP1:      return (Resource) {"wav/footstep1.wav", 0.1F, 32.0F};
        case SOUND_FOOTSTEP2:      return (Resource) {"wav/footstep2.wav", 0.1F, 32.0F};
        case SOUND_FOOTSTEP3:      return (Resource) {"wav/footstep3.wav", 0.1F, 32.0F};
        case SOUND_FOOTSTEP4:      return (Resource) {"wav/footstep4.wav", 0.1F, 32.0F};
        case SOUND_WADE1:          return (Resource) {"wav/wade1.wav", 0.1F, 32.0F};
        case SOUND_WADE2:          return (Resource) {"wav/wade2.wav", 0.1F, 32.0F};
        case SOUND_WADE3:          return (Resource) {"wav/wade3.wav", 0.1F, 32.0F};
        case SOUND_WADE4:          return (Resource) {"wav/wade4.wav", 0.1F, 32.0F};
        case SOUND_JUMP:           return (Resource) {"wav/jump.wav", 0.1F, 32.0F};
        case SOUND_JUMP_WATER:     return (Resource) {"wav/waterjump.wav", 0.1F, 32.0F};
        case SOUND_LAND:           return (Resource) {"wav/land.wav", 0.1F, 32.0F};
        case SOUND_LAND_WATER:     return (Resource) {"wav/waterland.wav", 0.1F, 32.0F};
        case SOUND_HURT_FALL:      return (Resource) {"wav/fallhurt.wav", 0.1F, 32.0F};
        case SOUND_EXPLODE:        return (Resource) {"wav/explode.wav", 0.1F, 53.0F};
        case SOUND_EXPLODE_WATER:  return (Resource) {"wav/waterexplode.wav", 0.1F, 53.0F};
        case SOUND_GRENADE_BOUNCE: return (Resource) {"wav/grenadebounce.wav", 0.1F, 48.0F};
        case SOUND_GRENADE_PIN:    return (Resource) {"wav/pin.wav", 0.1F, 48.0F};
        case SOUND_RIFLE_SHOOT:    return (Resource) {"wav/semishoot.wav", 0.1F, 96.0F};
        case SOUND_RIFLE_RELOAD:   return (Resource) {"wav/semireload.wav", 0.1F, 16.0F};
        case SOUND_SMG_SHOOT:      return (Resource) {"wav/smgshoot.wav", 0.1F, 96.0F};
        case SOUND_SMG_RELOAD:     return (Resource) {"wav/smgreload.wav", 0.1F, 16.0F};
        case SOUND_SHOTGUN_SHOOT:  return (Resource) {"wav/shotgunshoot.wav", 0.1F, 96.0F};
        case SOUND_SHOTGUN_RELOAD: return (Resource) {"wav/shotgunreload.wav", 0.1F, 16.0F};
        case SOUND_SHOTGUN_COCK:   return (Resource) {"wav/cock.wav", 0.1F, 16.0F};
        case SOUND_HITGROUND:      return (Resource) {"wav/hitground.wav", 0.1F, 32.0F};
        case SOUND_HITPLAYER:      return (Resource) {"wav/hitplayer.wav", 0.1F, 32.0F};
        case SOUND_BUILD:          return (Resource) {"wav/build.wav", 0.1F, 32.0F};
        case SOUND_SPADE_WOOSH:    return (Resource) {"wav/woosh.wav", 0.1F, 32.0F};
        case SOUND_SPADE_WHACK:    return (Resource) {"wav/whack.wav", 0.1F, 32.0F};
        case SOUND_DEATH:          return (Resource) {"wav/death.wav", 0.1F, 24.0F};
        case SOUND_BEEP1:          return (Resource) {"wav/beep1.wav", 0.1F, 1024.0F};
        case SOUND_BEEP2:          return (Resource) {"wav/beep2.wav", 0.1F, 1024.0F};
        case SOUND_SWITCH:         return (Resource) {"wav/switch.wav", 0.1F, 1024.0F};
        case SOUND_EMPTY:          return (Resource) {"wav/empty.wav", 0.1F, 1024.0F};
        case SOUND_INTRO:          return (Resource) {"wav/intro.wav", 0.1F, 1024.0F};
        case SOUND_DEBRIS:         return (Resource) {"wav/debris.wav", 0.1F, 53.0F};
        case SOUND_BOUNCE:         return (Resource) {"wav/bounce.wav", 0.1F, 32.0F};
        case SOUND_IMPACT:         return (Resource) {"wav/impact.wav", 0.1F, 53.0F};
        case SOUND_PICKUP:         return (Resource) {"wav/pickup.wav", 0.1F, 1024.0F};
        case SOUND_HORN:           return (Resource) {"wav/horn.wav", 0.1F, 1024.0F};
    }

    return (Resource) {NULL, 0.0F, 0.0F};
}
#endif

void sound_volume(float vol) {
#ifdef USE_SOUND
    if (sound_enabled)
        alListenerf(AL_GAIN, vol);
#endif
}

static void sound_createEx(SoundSpace option, WAV * w, float x, float y, float z, float vx, float vy,
                           float vz, int player) {
#ifdef USE_SOUND
    if (!sound_enabled)
        return;

    SoundSource s = (SoundSource) {
        .local = option == SOUND_LOCAL,
        .stick_to_player = player,
    };

    alGetError();
    alGenSources(1, &s.openal_handle);

    if (alGetError() == AL_NO_ERROR) {
        alSourcef(s.openal_handle, AL_PITCH, 1.0F);
        alSourcef(s.openal_handle, AL_GAIN, 1.0F);
        alSourcef(s.openal_handle, AL_REFERENCE_DISTANCE, s.local ? 0.0F : w->min * SOUND_SCALE);
        alSourcef(s.openal_handle, AL_MAX_DISTANCE, s.local ? 2048.0F : w->max * SOUND_SCALE);
        alSource3f(s.openal_handle, AL_POSITION, s.local ? 0.0F : x * SOUND_SCALE, s.local ? 0.0F : y * SOUND_SCALE,
                   s.local ? 0.0F : z * SOUND_SCALE);
        alSource3f(s.openal_handle, AL_VELOCITY, s.local ? 0.0F : vx * SOUND_SCALE, s.local ? 0.0F : vy * SOUND_SCALE,
                   s.local ? 0.0F : vz * SOUND_SCALE);
        alSourcei(s.openal_handle, AL_SOURCE_RELATIVE, s.local);
        alSourcei(s.openal_handle, AL_LOOPING, AL_FALSE);
        alSourcei(s.openal_handle, AL_BUFFER, w->openal_buffer);

        alSourcePlay(s.openal_handle);

        if (alGetError() == AL_NO_ERROR) {
            entitysys_add(&sound_sources, &s);
        } else {
            alDeleteSources(1, &s.openal_handle);
        }
    }
#endif
}

void sound_create_sticky(WAV * w, Player * player, int player_id) {
    sound_createEx(SOUND_WORLD, w, player->pos.x, player->pos.y, player->pos.z, 0.0F, 0.0F, 0.0F, player_id);
}

void sound_create(SoundSpace option, WAV * w, float x, float y, float z) {
    sound_createEx(option, w, x, y, z, 0.0F, 0.0F, 0.0F, -1);
}

void sound_velocity(SoundSource * s, float vx, float vy, float vz) {
#ifdef USE_SOUND
    if (!sound_enabled || s->local)
        return;
    alSource3f(s->openal_handle, AL_VELOCITY, vx * SOUND_SCALE, vy * SOUND_SCALE, vz * SOUND_SCALE);
#endif
}

void sound_position(SoundSource * s, float x, float y, float z) {
#ifdef USE_SOUND
    if (!sound_enabled || s->local)
        return;

    alSource3f(s->openal_handle, AL_POSITION, x * SOUND_SCALE, y * SOUND_SCALE, z * SOUND_SCALE);
#endif
}

#ifdef USE_SOUND
static bool sound_update_single(void * obj, void * user) {
    SoundSource * s = (SoundSource *) obj;

    int source_state;
    alGetSourcei(s->openal_handle, AL_SOURCE_STATE, &source_state);
    if (source_state == AL_STOPPED || (s->stick_to_player >= 0 && !players[s->stick_to_player].connected)) {
        alDeleteSources(1, &s->openal_handle);

        return true;
    } else {
        if (s->stick_to_player >= 0) {
            sound_position(s, players[s->stick_to_player].pos.x, players[s->stick_to_player].pos.y,
                           players[s->stick_to_player].pos.z);
            sound_velocity(s, players[s->stick_to_player].physics.velocity.x,
                           players[s->stick_to_player].physics.velocity.y,
                           players[s->stick_to_player].physics.velocity.z);
        }

        return false;
    }
}
#endif

void sound_update() {
#ifdef USE_SOUND
    if (!sound_enabled)
        return;

    float orientation[] = {
        sin(camera.rot.x) * sin(camera.rot.y),
        cos(camera.rot.y),
        cos(camera.rot.x) * sin(camera.rot.y),
        0.0F,
        1.0F,
        0.0F,
    };

    alListener3f(AL_POSITION, camera.pos.x * SOUND_SCALE, camera.pos.y * SOUND_SCALE, camera.pos.z * SOUND_SCALE);
    alListener3f(AL_VELOCITY, camera.v.x * SOUND_SCALE, camera.v.y * SOUND_SCALE, camera.v.z * SOUND_SCALE);
    alListenerfv(AL_ORIENTATION, orientation);

    entitysys_iterate(&sound_sources, NULL, sound_update_single);
#endif
}

void sound_load(WAV * wav, const char * name, float min, float max) {
#ifdef USE_SOUND
    if (!sound_enabled)
        return;
    unsigned int channels, samplerate;
    drwav_uint64 samplecount;
    short * samples = drwav_open_file_and_read_pcm_frames_s16(name, &channels, &samplerate, &samplecount, NULL);
    if (samples == NULL) {
        log_fatal("Could not load sound %s", name);
        exit(1);
    }

    short * audio;
    if (channels > 1) { // convert stereo to mono
        audio = malloc(samplecount * sizeof(short) / 2);
        CHECK_ALLOCATION_ERROR(audio)
        for (size_t k = 0; k < samplecount / 2; k++)
            audio[k] = ((int) samples[k * 2] + (int) samples[k * 2 + 1]) / 2; // prevent overflow
        free(samples);
    }

    alGenBuffers(1, &wav->openal_buffer);
    alBufferData(wav->openal_buffer, AL_FORMAT_MONO16, (channels > 1) ? audio : samples,
                 samplecount * sizeof(short) / channels, samplerate);

    wav->min = min;
    wav->max = max;
#endif
}

#ifdef USE_SOUND
static ALCdevice * device = NULL;
#endif

void sound_init() {
#ifdef USE_SOUND
    entitysys_create(&sound_sources, sizeof(SoundSource), 256);

    device = alcOpenDevice(NULL);

    if (!device) {
        sound_enabled = 0;
        log_warn("Could not open sound device!");
        return;
    }

    ALCcontext * context = alcCreateContext(device, NULL);
    if (!alcMakeContextCurrent(context)) {
        sound_enabled = 0;
        log_warn("Could not enter sound device context!");
        return;
    }

    alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

    sound_volume(settings.volume / 10.0F);

    for (enum WAV i = SOUND_FIRST; i <= SOUND_LAST; i++) {
        Resource res = wav_sound(i);
        sound_load(&_sounds[i], res.filename, res.min, res.max);
    }
#endif
}

void sound_deinit() {
#ifdef USE_SOUND
    //if (device != NULL)
    //    alcCloseDevice(device);
#endif
}
