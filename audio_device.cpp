#include "stdafx.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <vorbis/vorbisfile.h>

#include "audio_device.h"
#include "file_path.h"

AudioDevice::AudioDevice() {
}

static bool initialized = false;

optional<string> AudioDevice::initialize() {
  if ((device = alcOpenDevice(nullptr))) {
    if ((context = alcCreateContext((ALCdevice*)device, nullptr))) {
      alcMakeContextCurrent((ALCcontext*)context);
      alDistanceModel(AL_NONE);
      initialized = true;
      return none;
    }
  }
  return "Error code: " + toString(alGetError());
}

AudioDevice::~AudioDevice() {
  if (device) {
    RecursiveLock lock(mutex);
    sources.clear();
    alcMakeContextCurrent(nullptr);
    alcDestroyContext((ALCcontext*)context);
    alcCloseDevice((ALCdevice*)device);
  }
}

const int maxSources = 12;

#define AL(X) { if (initialized) {X; checkError(__FILE__, __LINE__, #X); }}

static void checkError(const char* file, int line, const char* functionName) {
  ALenum error = alGetError();
  CHECK(!error) << file << ":" << line << " " << " " << functionName << ": " << alGetString(error);
}

optional<OpenalId> AudioDevice::getFreeSource() {
  RecursiveLock lock(mutex);
  for (int i : All(sources)) {
    auto& source = sources[i];
    ALint state = 0;
    AL(alGetSourcei(source.getId(), AL_SOURCE_STATE, &state));
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

milliseconds AudioDevice::getDuration(const SoundBuffer& sound) {
  auto id = sound.getBufferId();
  ALint sizeInBytes;
  ALint channels;
  ALint bits;
  alGetBufferi(id, AL_SIZE, &sizeInBytes);
  alGetBufferi(id, AL_CHANNELS, &channels);
  alGetBufferi(id, AL_BITS, &bits);

  auto lengthInSamples = sizeInBytes * 8 / (channels * bits);
  ALint frequency;

  alGetBufferi(id, AL_FREQUENCY, &frequency);

  return milliseconds{lengthInSamples / frequency * 1000};
}

void AudioDevice::play(const SoundBuffer& sound, double volume, double pitch) {
  RecursiveLock lock(mutex);
  if (auto source = getFreeSource()) {
    ALint state = 0;
    AL(alGetSourcei(*source, AL_SOURCE_STATE, &state));
    CHECK(state == AL_INITIAL) << state;
    AL(alSourcei(*source, AL_BUFFER, sound.getBufferId()));
    AL(alSourcef(*source, AL_PITCH, pitch));
    AL(alSourcef(*source, AL_GAIN, volume));
    AL(alSourcePlay(*source));
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
    ret.append(tmp, tmp + result);
  }
  return ret;
}

SoundBuffer::SoundBuffer(const FilePath& path) {
  OggVorbis_File file;
  CHECK(ov_fopen(path.getPath(), &file) == 0) << "Error opening audio file: " << path;
  vorbis_info* info = ov_info(&file, -1);
  ov_raw_seek(&file, 0);
  vector<char> buffer = readSoundData(file);
  OpenalId id;
  AL(alGenBuffers(1, &id));
  AL(alBufferData(id, (info->channels > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, buffer.data(), buffer.size(),
      info->rate));
  bufferId = id;
  ov_clear(&file);
}

SoundBuffer::~SoundBuffer() {
  if (bufferId)
    AL(alDeleteBuffers(1, &(*bufferId)));
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
  AL(alGenSources(1, &*id));
}

SoundSource::SoundSource(SoundSource&& o) {
  id = o.id;
  o.id = none;
}

SoundSource& SoundSource::operator = (SoundSource&& o) {
  if (id)
    destroy();
  id = o.id;
  o.id = none;
  return *this;
}

SoundSource::~SoundSource() {
  if (id)
    destroy();
}

OpenalId SoundSource::getId() const {
  return *id;
}

void SoundSource::destroy() {
  AL(alDeleteSources(1, &*id));
  id.reset();
}

const int streamingBufferSize = 1 * 2 * 2 * 441000;

SoundStream::SoundStream(const FilePath& path, double volume) : startedPlaying(false),
      streamer([path, this]{ init(path);}, [this]{loop();}), volume((float) volume) {
}

bool SoundStream::isPlaying() const {
  if (!startedPlaying)
    return true;
  ALint state = 0;
  AL(alGetSourcei(source.getId(), AL_SOURCE_STATE, &state));
  return state == AL_PLAYING;
}

void SoundStream::setVolume(double v) {
  alSourcef(source.getId(), AL_GAIN, min<float>(1, max<float>(0, (float) v)));
  volume = v;
}

double SoundStream::getVolume() const {
  return volume;
}

void SoundStream::init(const FilePath& path) {
  USER_CHECK(ov_fopen(path.getPath(), file.get()) == 0) << "Error opening audio file: " << path
      << ". Please make sure all game data is in place.";
  info = ov_info(file.get(), -1);
  AL(alGenBuffers(2, buffers));
}

void SoundStream::loop() {
  int numQueued = 0;
  AL(alGetSourcei(source.getId(), AL_BUFFERS_QUEUED, &numQueued));
  if (numQueued == 0) { /*fill and queue initial buffers*/
    vector<char> data = readSoundData(*file, streamingBufferSize);
    AL(alBufferData(buffers[0], (info->channels > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, data.data(),
        data.size(), info->rate));
    data = readSoundData(*file, streamingBufferSize);
    AL(alBufferData(buffers[1], (info->channels > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, data.data(),
        data.size(), info->rate));
    AL(alSourceQueueBuffers(source.getId(), 2, buffers));
    alSourcef(source.getId(), AL_GAIN, min<float>(1, max<float>(0, volume)));
    ALenum error = alGetError();
    CHECK(!error) << "volume error " << volume;
    AL(alSourcePlay(source.getId()));
    startedPlaying = true;
    //CHECK(isPlaying()); fails if I unplug/plug the speaker cable...?
  } else { /*refill processed buffers*/
    int numProcessed = 0;
    AL(alGetSourcei(source.getId(), AL_BUFFERS_PROCESSED, &numProcessed));
    while (numProcessed--) {
      vector<char> data = readSoundData(*file, streamingBufferSize);
      if (data.size() == 0)
        break;
      else {
        ALuint buffer = 0;
        AL(alSourceUnqueueBuffers(source.getId(), 1, &buffer));
        AL(alBufferData(buffer, (info->channels > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, data.data(),
            data.size(), info->rate));
        AL(alSourceQueueBuffers(source.getId(), 1, &buffer));
      }
    }
  }
  sleep_for(milliseconds(10));
}

SoundStream::~SoundStream() {
  streamer.finishAndWait();
  ov_clear(file.get());
  source.destroy();
  AL(alDeleteBuffers(2, buffers));
}

