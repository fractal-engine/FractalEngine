#pragma once

#include <portaudio.h>
#include <string>
#include <thread>
#include "engine/core/logger.h"
#include "thirdparty/dr_wav.h"

class SoundManager {
public:
  static SoundManager& Instance() {
    static SoundManager instance;
    return instance;
  }

  // Constants for audio filenames.
  static const std::string kClickSoundFilename;
  static const std::string kAmbientSoundFilename;

  bool init() {  // Initialize PortAudio.
    PaError err = Pa_Initialize();
    if (err != paNoError) {
      Logger::getInstance().Log(
          LogLevel::Error,
          std::string("PortAudio init error: ") + Pa_GetErrorText(err));
      return false;
    }
    return true;
  }

  void terminate() {  // Terminate PortAudio.
    stopAmbient();
    Pa_Terminate();
  }

  // Start playing an ambient WAV file in a loop using dr_wav.
  bool startAmbient() {
    if (!drwav_init_file(&ambientWav, kAmbientSoundFilename.c_str(), NULL)) {
      Logger::getInstance().Log(
          LogLevel::Error, std::string("Error: Could not open ambient file: ") +
                               kAmbientSoundFilename);
      ambientInitialized = false;
      return false;
    }
    ambientInitialized = true;
    PaError err = Pa_OpenDefaultStream(&ambientStream, 0, ambientWav.channels,
                                       paFloat32, ambientWav.sampleRate,
                                       paFramesPerBufferUnspecified,
                                       ambientCallback, this);
    if (err != paNoError) {  // Error opening stream.
      Logger::getInstance().Log(
          LogLevel::Error,
          std::string("Error opening ambient stream: ") + Pa_GetErrorText(err));
      drwav_uninit(&ambientWav);
      ambientInitialized = false;
      ambientStream = NULL;
      return false;
    }
    err = Pa_StartStream(ambientStream);
    if (err != paNoError) {
      Logger::getInstance().Log(LogLevel::Error,
                                std::string("Error starting ambient stream: ") +
                                    Pa_GetErrorText(err));
      Pa_CloseStream(ambientStream);
      ambientStream = NULL;
      drwav_uninit(&ambientWav);
      ambientInitialized = false;
      return false;
    }
    return true;
  }

  void stopAmbient() {
    if (ambientStream != NULL) {
      Pa_StopStream(ambientStream);
      Pa_CloseStream(ambientStream);
      ambientStream = NULL;
    }
    if (ambientInitialized) {
      drwav_uninit(&ambientWav);
      ambientInitialized = false;
    }
  }

  // Blocking function to play a click sound effect using dr_wav.
  bool playClick() {
    drwav clickWav;
    if (!drwav_init_file(&clickWav, kClickSoundFilename.c_str(), NULL)) {
      Logger::getInstance().Log(
          LogLevel::Error, std::string("Error: Could not open click file: ") +
                               kClickSoundFilename);
      return false;
    }
    drwav_uint64 frames = clickWav.totalPCMFrameCount;
    unsigned int channels = clickWav.channels;
    float* buffer = new float[frames * channels];
    drwav_uint64 framesRead =
        drwav_read_pcm_frames_f32(&clickWav, frames, buffer);
    drwav_uninit(&clickWav);
    if (framesRead != frames) {
      Logger::getInstance().Log(LogLevel::Error,
                                "Error reading all frames from click file");
      delete[] buffer;
      return false;
    }
    PaStream* stream;
    PaError err = Pa_OpenDefaultStream(
        &stream, 0, channels, paFloat32, clickWav.sampleRate,
        paFramesPerBufferUnspecified, NULL, NULL);
    if (err != paNoError) {
      Logger::getInstance().Log(
          LogLevel::Error,
          std::string("Error opening click stream: ") + Pa_GetErrorText(err));
      delete[] buffer;
      return false;
    }
    err = Pa_StartStream(stream);
    if (err != paNoError) {
      Logger::getInstance().Log(
          LogLevel::Error,
          std::string("Error starting click stream: ") + Pa_GetErrorText(err));
      Pa_CloseStream(stream);
      delete[] buffer;
      return false;
    }
    // Write the entire buffer to the stream.
    err = Pa_WriteStream(stream, buffer, (unsigned long)frames);
    if (err != paNoError) {
      Logger::getInstance().Log(
          LogLevel::Error,
          std::string("Error writing click stream: ") + Pa_GetErrorText(err));
      Pa_StopStream(stream);
      Pa_CloseStream(stream);
      delete[] buffer;
      return false;
    }
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    delete[] buffer;
    return true;
  }

  // New asynchronous function to play a click sound without blocking.
  bool playClickAsync() {
    std::thread([]() {
      // Call the blocking playClick() in a separate thread.
      SoundManager::Instance().playClick();
    }).detach();
    return true;
  }

  // Setter to adjust ambient volume at runtime.
  void setAmbientVolume(float volume) { ambientVolume = volume; }

private:
  SoundManager()
      : ambientStream(NULL),
        ambientInitialized(false),
        ambientVolume(2.0f) {}  // Default ambient volume  (change volume in main, not here)
  ~SoundManager() { terminate(); }
  SoundManager(const SoundManager&) = delete;
  SoundManager& operator=(const SoundManager&) = delete;

  // Ambient sound state.
  PaStream* ambientStream;
  drwav ambientWav;
  bool ambientInitialized;  // Indicates if ambientWav is initialized.
  float
      ambientVolume;  // Ambient volume factor (0.0 = silent, 1.0 = full volume)

  // Callback for looping ambient sound.
  static int ambientCallback(const void* input, void* output,
                             unsigned long frameCount,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void* userData) {
    SoundManager* sm = reinterpret_cast<SoundManager*>(userData);
    float* out = reinterpret_cast<float*>(output);
    unsigned int channels = sm->ambientWav.channels;
    drwav_uint64 framesRead =
        drwav_read_pcm_frames_f32(&sm->ambientWav, frameCount, out);
    if (framesRead < frameCount) {
      size_t offset = framesRead * channels;
      for (unsigned long i = framesRead; i < frameCount; i++) {
        for (unsigned int ch = 0; ch < channels; ch++) {
          out[offset++] = 0.0f;
        }
      }
      // Rewind for looping. Be kind, rewind.
      drwav_seek_to_pcm_frame(&sm->ambientWav, 0);
    }
    // Apply ambient volume scaling.
    for (unsigned long i = 0; i < framesRead * channels; i++) {
      out[i] *= sm->ambientVolume;
    }
    return paContinue;
  }
};
