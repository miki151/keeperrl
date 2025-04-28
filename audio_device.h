#pragma once

#include "util.h"

struct OggVorbis_File;
struct vorbis_info;
class FilePath;

typedef unsigned int OpenalId;

class SoundBuffer {
  public:
  SoundBuffer(const FilePath&);
  ~SoundBuffer();
  SoundBuffer(SoundBuffer&&);

  OpenalId getBufferId() const;

  private:
  optional<OpenalId> bufferId;
};

class SoundSource {
  public:
  SoundSource();
  ~SoundSource();
  SoundSource& operator = (SoundSource&&);
  SoundSource(SoundSource&&);
  OpenalId getId() const;
  void destroy();

  private:
  optional<OpenalId> id;
};

class AudioDevice;

class SoundStream {
  public:
  SoundStream(const FilePath&, double volume);
  bool isPlaying() const;
  ~SoundStream();
  void setVolume(double);
  double getVolume() const;

  private:
  void loop();
  void init(const FilePath&);
  SoundSource source;
  OpenalId buffers[2];
  HeapAllocated<OggVorbis_File> file;
  vorbis_info *info;
  atomic<bool> startedPlaying;
  AsyncLoop streamer;
  atomic<float> volume;
};

class AudioDevice {
  public:
  AudioDevice();
  optional<string> initialize();
  ~AudioDevice();
  void play(const SoundBuffer&, double volume, double pitch = 1);
  milliseconds getDuration(const SoundBuffer&);

  private:
  friend SoundStream;
  optional<OpenalId> getFreeSource();
  void *device;
  void *context;
  vector<SoundSource> sources;
  recursive_mutex mutex;
};

