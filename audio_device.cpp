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
  sources.resize(16);
  alGenSources(sources.size(), sources.data());
  ids.resize(sources.size());
}

AudioDevice::~AudioDevice() {
  alDeleteSources(sources.size(), sources.data());
  alcMakeContextCurrent(NULL);
  alcDestroyContext((ALCcontext*)context);
  alcCloseDevice((ALCdevice*)device);
}

optional<SoundSource> AudioDevice::getFreeSource() {
  for (int i : All(sources)) {
    auto& source = sources[i];
    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
      alDeleteSources(1, &source); // recreating the source does a better job at cleaning up after streaming
      alGenSources(1, &source);    // without it the old buffer queue still existed
      return SoundSource(source, ++ids[i]);
    }
  }
  return none;
} 

void AudioDevice::play(const SoundBuffer& sound, double volume, double pitch) {
  if (auto source = getFreeSource()) {
    alSourcei(source->getSource(), AL_BUFFER, sound.getBufferId());
    alSourcef(source->getSource(), AL_PITCH, pitch);
    alSourcef(source->getSource(), AL_GAIN, volume);
    alSourcePlay(source->getSource());
  }
}

optional<unsigned int> AudioDevice::retrieveSource(const SoundSource& source) {
  if (auto index = findElement(sources, source.getSource()))
    if (ids[*index] == source.getId())
      return sources[*index];
  return none;
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
  unsigned int id;
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

unsigned int SoundBuffer::getBufferId() const {
  return *bufferId;
}

SoundSource::SoundSource() : source(1234567), id(7656321) {
}

SoundSource::SoundSource(unsigned int s, int i) : source(s), id(i) {
}

unsigned int SoundSource::getSource() const {
  return source;
}

int SoundSource::getId() const {
  return id;
}

const int streamingBufferSize = 1 * 2 * 2 * 44100;

SoundStream::SoundStream(AudioDevice& device, const char* path, double volume) : source(*device.getFreeSource()),
      audioDevice(device), startedPlaying(false),
      streamer([path, this]{init(path);}, [volume, this]{loop(volume);}) {
}

bool SoundStream::isPlaying() const {
  if (!startedPlaying)
    return true;
  if (auto sourceId = audioDevice.retrieveSource(source)) {
    ALint state;
    alGetSourcei(*sourceId, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
  } else
    return false;
}

void SoundStream::setVolume(double v) {
  if (auto sourceId = audioDevice.retrieveSource(source))
    alSourcef(*sourceId, AL_GAIN, v);
}

double SoundStream::getVolume() const {
  if (auto sourceId = audioDevice.retrieveSource(source)) {
    float ret;
    alGetSourcef(*sourceId, AL_GAIN, &ret);
    return ret;
  } else
    return 0;
}

void SoundStream::init(const char* path) {
  CHECK(ov_fopen(path, file.get()) == 0) << "Error opening audio file: " << path;
  info = ov_info(file.get(), -1);
  alGenBuffers(2, buffers);
}

void SoundStream::loop(double volume) {
  int numQueued;
  alGetSourcei(source.getSource(), AL_BUFFERS_QUEUED, &numQueued);
  if (numQueued == 0) { /*fill and queue initial buffers*/
    vector<char> data = readSoundData(*file, streamingBufferSize);
    alBufferData(buffers[0], (info->channels > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, data.data(),
        data.size(), info->rate);
    data = readSoundData(*file, streamingBufferSize);
    alBufferData(buffers[1], (info->channels > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, data.data(),
        data.size(), info->rate);
    alSourceQueueBuffers(source.getSource(), 2, buffers);
    alSourcef(source.getSource(), AL_GAIN, volume);
    alSourcePlay(source.getSource());
    startedPlaying = true;
    CHECK(isPlaying());
  } else { /*refill processed buffers*/
    int numProcessed;
    alGetSourcei(source.getSource(), AL_BUFFERS_PROCESSED, &numProcessed);
    while (numProcessed--) {
      vector<char> data = readSoundData(*file, streamingBufferSize);
      if (data.size() == 0)
        break;
      else {
        ALuint buffer;
        alSourceUnqueueBuffers(source.getSource(), 1, &buffer);
        alBufferData(buffer, (info->channels > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, data.data(),
            data.size(), info->rate);
        alSourceQueueBuffers(source.getSource(), 1, &buffer);
      }
    }
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

SoundStream::~SoundStream() {
  streamer.finish();
  ov_clear(file.get());
  alSourceStop(source.getSource());
  alDeleteBuffers(2, buffers);
}

