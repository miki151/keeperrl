#pragma once

#include "util.h"

struct OggVorbis_File;
struct vorbis_info;

class SoundBuffer {
  public:
  SoundBuffer(const char* path);
  ~SoundBuffer();
  SoundBuffer(SoundBuffer&&);

  unsigned int getBufferId() const;

  private:
  optional<unsigned int> bufferId;
};

class SoundSource {
  public:
  SoundSource();
  SoundSource(unsigned int source, int id);

  unsigned int getSource() const;
  int getId() const;

  private:
  unsigned int source;
  int id;
};

class AudioDevice;

class SoundStream {
  public:
  SoundStream(AudioDevice&, const char* path, double volume);
  bool isPlaying() const;
  ~SoundStream();
  void setVolume(double);
  double getVolume() const;

  private:
  void loop(double volume);
  void init(const char* path);
  SoundSource source;
  unsigned int buffers[2];
  HeapAllocated<OggVorbis_File> file;
  vorbis_info *info;
  AudioDevice& audioDevice;
  atomic<bool> startedPlaying;
  AsyncLoop streamer;
};

class AudioDevice {
  public:
  AudioDevice();
  ~AudioDevice();
  void play(const SoundBuffer&, double volume, double pitch = 1);

  private:
  friend SoundStream;
  optional<SoundSource> getFreeSource();
  optional<unsigned int> retrieveSource(const SoundSource&);
  void *device;
  void *context;
  vector<unsigned int> sources;
  vector<int> ids;
};

