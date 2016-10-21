#pragma once

#include "util.h"

struct OggVorbis_File;
struct vorbis_info;

typedef unsigned int OpenalId;

class SoundBuffer {
  public:
  SoundBuffer(const char* path);
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
  SoundStream(const char* path, double volume);
  bool isPlaying() const;
  ~SoundStream();
  void setVolume(double);
  double getVolume() const;

  private:
  void loop(double volume);
  void init(const char* path);
  SoundSource source;
  OpenalId buffers[2];
  HeapAllocated<OggVorbis_File> file;
  vorbis_info *info;
  atomic<bool> startedPlaying;
  AsyncLoop streamer;
};

class AudioDevice {
  public:
  AudioDevice();
  optional<string> initialize();
  ~AudioDevice();
  void play(const SoundBuffer&, double volume, double pitch = 1);

  private:
  friend SoundStream;
  optional<OpenalId> getFreeSource();
  void *device;
  void *context;
  vector<SoundSource> sources;
  recursive_mutex mutex;
};

