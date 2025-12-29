#ifndef AUDIO_HPP
#define AUDIO_HPP

#include <miniaudio.h>

enum class SoundId {
  FlashlightToggle = 0,
  RunningGrass,
  StepOnGrass,
  LandingOnGrass,
  Count
};

inline constexpr int soundCount = static_cast<int>(SoundId::Count);

struct AudioPaths {
  const char* sounds[soundCount] = {
      "assets/sounds/flashlightToggle.mp3",
      "assets/sounds/running_grass.mp3",
      "assets/sounds/step_on_grass.mp3",
      "assets/sounds/landing_on_grass.mp3"};
};

struct AudioSystem {
  ma_engine engine;
  ma_sound sounds[soundCount];
  AudioPaths paths;
  bool initialized = false;
  float masterVolume = 0.8f;
};

bool initAudio(AudioSystem& audio);
void shutdownAudio(AudioSystem& audio);
void setMasterVolume(AudioSystem& audio, float volume);
void playSound(AudioSystem& audio, SoundId soundId);
void startLoopingSound(AudioSystem& audio, SoundId soundId);
void stopSound(AudioSystem& audio, SoundId soundId);

#endif
