#define DR_WAV_IMPLEMENTATION
#include "../thirdparty/dr_wav.h"
#include "platform/paths.h"

#include "sound_manager.h"

// Define the constant filenames here.
const std::string SoundManager::kClickSoundFilename =
    Platform::ResolvePath("assets/audio/click.wav");
const std::string SoundManager::kAmbientSoundFilename =
    Platform::ResolvePath("assets/audio/ambient.wav");
