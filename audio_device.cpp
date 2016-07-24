#include "stdafx.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <vorbis/vorbisfile.h>

#include "audio_device.h"

AudioDevice::AudioDevice() {
  CHECK(device = alcOpenDevice(nullptr));
  CHECK(context = alcCreateContext((ALCdevice*)device, nullptr));
  alcMakeContextCurrent((ALCcontext*)context);
  alDistanceModel(AL_NONE);
}

AudioDevice::~AudioDevice() {
  RecursiveLock lock(mutex);
  sources.clear();
  alcMakeContextCurrent(NULL);
  alcDestroyContext((ALCcontext*)context);
  alcCloseDevice((ALCdevice*)device);
}

const int maxSources = 16;

optional<OpenalId> AudioDevice::getFreeSource() {
  RecursiveLock lock(mutex);
  for (int i : All(sources)) {
    auto& source = sources[i];
    ALint state;
    alGetSourcei(source.getId(), AL_SOURCE_STATE, &state);
    if (state == AL_STOPPED) {
      // recreating the source does a better job at cleaning up after streaming
      // without it the old buffer queue still existed
      source = SoundSource();
      return source.getId();
    }
  }
  if (sources.size() < maxSources) {
    sources.emplace_back();
    return sources.back().getId();
  } else
    return none;
} 

void AudioDevice::play(const SoundBuffer& sound, double volume, double pitch) {
  RecursiveLock lock(mutex);
  if (auto source = getFreeSource()) {
    ALint state;
    alGetSourcei(*source, AL_SOURCE_STATE, &state);
    CHECK(state == AL_INITIAL);
    alSourcei(*source, AL_BUFFER, sound.getBufferId());
    alSourcef(*source, AL_PITCH, pitch);
    alSourcef(*source, AL_GAIN, volume);
    alSourcePlay(*source);
  }
}

static vector<char> readSoundData(OggVorbis_File& file, optional<int> length = none) {
  vector<char> ret;
  while (!length || ret.size() < *length) {
    char tmp[4096];
    int bit_stream = 0;
    int lengthNow = sizeof(tmp);
    if (length)
      lengthNow = min<int>(*length - ret.size(), lengthNow);
    int result = ov_read(&file, tmp, lengthNow, 0, 2, 1, &bit_stream);
    if (result <= 0)
      break;
    ret.insert(end(ret), tmp, tmp + result);
  }
  return ret;
}

SoundBuffer::SoundBuffer(const char* path) {
  OggVorbis_File file;
  CHECK(ov_fopen(path, &file) == 0) << "Error opening audio file: " << path;
  vorbis_info* info = ov_info(&file, -1);
  ov_raw_seek(&file, 0);
  vector<char> buffer = readSoundData(file);
  OpenalId id;
  alGenBuffers(1, &id);
  alBufferData(id, (info->channels > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, buffer.data(), buffer.size(),
      info->rate);
  bufferId = id;
  ov_clear(&file);
}

SoundBuffer::~SoundBuffer() {
  if (bufferId)
    alDeleteBuffers(1, &(*bufferId));
}

SoundBuffer::SoundBuffer(SoundBuffer&& o) {
  bufferId = o.bufferId;
  o.bufferId = none;
}

OpenalId SoundBuffer::getBufferId() const {
  return *bufferId;
}

SoundSource::SoundSource() {
  id.emplace();
  alGenSources(1, &*id);
}

SoundSource::SoundSource(SoundSource&& o) {
  id = o.id;
  o.id = none;
}

SoundSource& SoundSource::operator = (SoundSource&& o) {
  id = o.id;
  o.id = none;
  return *this;
}

SoundSource::~SoundSource() {
  if (id) {
    alDeleteSources(1, &*id);
  }
}

OpenalId SoundSource::getId() const {
  return *id;
}

const int streamingBufferSize = 1 * 2 * 2 * 44100;

SoundStream::SoundStream(const char* path, double volume) : startedPlaying(false),
      streamer([path, this]{init(path);}, [volume, this]{loop(volume);}) {
}

bool SoundStream::isPlaying() const {
  if (!startedPlaying)
    return true;
  ALint state;
  alGetSourcei(source.getId(), AL_SOURCE_STATE, &state);
  return state == AL_PLAYING;
}

void SoundStream::setVolume(double v) {
  alSourcef(source.getId(), AL_GAIN, v);
}

double SoundStream::getVolume() const {
  float ret;
  alGetSourcef(source.getId(), AL_GAIN, &ret);
  return ret;
}

void SoundStream::init(const char* path) {
  CHECK(ov_fopen(path, file.get()) == 0) << "Error opening audio file: " << path;
  info = ov_info(file.get(), -1);
  alGenBuffers(2, buffers);
}

void SoundStream::loop(double volume) {
  int numQueued;
  alGetSourcei(source.getId(), AL_BUFFERS_QUEUED, &numQueued);
  if (numQueued == 0) { /*fill and queue initial buffers*/
    vector<char> data = readSoundData(*file, streamingBufferSize);
    alBufferData(buffers[0], (info->channels > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, data.data(),
        data.size(), info->rate);
    data = readSoundData(*file, streamingBufferSize);
    alBufferData(buffers[1], (info->channels > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, data.data(),
        data.size(), info->rate);
    alSourceQueueBuffers(source.getId(), 2, buffers);
    alSourcef(source.getId(), AL_GAIN, volume);
    alSourcePlay(source.getId());
    startedPlaying = true;
    CHECK(isPlaying());
  } else { /*refill processed buffers*/
    int numProcessed;
    alGetSourcei(source.getId(), AL_BUFFERS_PROCESSED, &numProcessed);
    while (numProcessed--) {
      vector<char> data = readSoundData(*file, streamingBufferSize);
      if (data.size() == 0)
        break;
      else {
        ALuint buffer;
        alSourceUnqueueBuffers(source.getId(), 1, &buffer);
        alBufferData(buffer, (info->channels > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, data.data(),
            data.size(), info->rate);
        alSourceQueueBuffers(source.getId(), 1, &buffer);
      }
    }
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

SoundStream::~SoundStream() {
  streamer.finishAndWait();
  ov_clear(file.get());
  alSourceStop(source.getId());
  alDeleteBuffers(2, buffers);
}

