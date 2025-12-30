#ifndef AUDIO_HPP
#define AUDIO_HPP

#include <miniaudio.h>

enum class SoundId {
  FlashlightToggle = 0,
  RunningGrass,
  StepOnGrass,
  LandingOnGrass,
  // NightForestAmbient,
  //  MysticalForestAmbient,
  Count
};

inline constexpr int soundCount = static_cast<int>(SoundId::Count);

struct AudioPaths {
  const char* sounds[soundCount] = {
      "assets/sounds/flashlightToggle.wav",
      "assets/sounds/running.wav",
      "assets/sounds/walking.wav",
      "assets/sounds/landing_on_grass.wav",
  };
};

struct AudioSystem {
  ma_engine engine;
  ma_sound sounds[soundCount];
  AudioPaths paths;
  bool initialized = false;
  float masterVolume = 0.03f;
};

bool initAudio(AudioSystem& audio);
void shutdownAudio(AudioSystem& audio);
void setMasterVolume(AudioSystem& audio, float volume);
void playSound(AudioSystem& audio, SoundId soundId);
void startLoopingSound(AudioSystem& audio, SoundId soundId);
void stopSound(AudioSystem& audio, SoundId soundId);

#endif
