#include "audio/audio.hpp"

#include <iostream>

static bool loadSound(AudioSystem& audio, const char* path, ma_sound& sound) {
  ma_result result = ma_sound_init_from_file(&audio.engine, path, 0, NULL, NULL,
                                             &sound);
  return result == MA_SUCCESS;
}

bool initAudio(AudioSystem& audio) {
  if (audio.initialized) {
    return true;
  }

  ma_result result = ma_engine_init(NULL, &audio.engine);
  if (result != MA_SUCCESS) {
    audio.initialized = false;
    return false;
  }

  for (int i = 0; i < soundCount; ++i) {
    if (!loadSound(audio, audio.paths.sounds[i], audio.sounds[i])) {
      std::cout << "Sound loading failed" << std::endl;
      for (int loaded = 0; loaded < i; ++loaded) {
        ma_sound_uninit(&audio.sounds[loaded]);
      }
      ma_engine_uninit(&audio.engine);
      audio.initialized = false;
      return false;
    }
  }

  ma_sound_set_looping(
      &audio.sounds[static_cast<int>(SoundId::RunningGrass)], MA_TRUE);
  ma_sound_set_looping(
      &audio.sounds[static_cast<int>(SoundId::StepOnGrass)], MA_TRUE);

  audio.initialized = true;
  ma_engine_set_volume(&audio.engine, audio.masterVolume);
  return true;
}

void shutdownAudio(AudioSystem& audio) {
  if (!audio.initialized) {
    return;
  }

  for (int i = 0; i < soundCount; ++i) {
    ma_sound_uninit(&audio.sounds[i]);
  }
  ma_engine_uninit(&audio.engine);
  audio.initialized = false;
}

void setMasterVolume(AudioSystem& audio, float volume) {
  if (volume < 0.0f) {
    volume = 0.0f;
  }
  if (volume > 1.0f) {
    volume = 1.0f;
  }

  audio.masterVolume = volume;
  if (!audio.initialized) {
    return;
  }

  ma_engine_set_volume(&audio.engine, audio.masterVolume);
}

void playSound(AudioSystem& audio, SoundId soundId) {
  if (!audio.initialized) {
    return;
  }

  ma_sound& sound = audio.sounds[static_cast<int>(soundId)];
  if (ma_sound_is_playing(&sound)) {
    ma_sound_stop(&sound);
  }

  ma_sound_seek_to_pcm_frame(&sound, 0);
  ma_sound_start(&sound);
}

void startLoopingSound(AudioSystem& audio, SoundId soundId) {
  if (!audio.initialized) {
    return;
  }

  ma_sound& sound = audio.sounds[static_cast<int>(soundId)];
  if (!ma_sound_is_playing(&sound)) {
    ma_sound_seek_to_pcm_frame(&sound, 0);
    ma_sound_start(&sound);
  }
}

void stopSound(AudioSystem& audio, SoundId soundId) {
  if (!audio.initialized) {
    return;
  }

  ma_sound& sound = audio.sounds[static_cast<int>(soundId)];
  if (ma_sound_is_playing(&sound)) {
    ma_sound_stop(&sound);
  }
}
