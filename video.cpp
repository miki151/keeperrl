/**
 * TheoraPlay; multithreaded Ogg Theora/Ogg Vorbis decoding.
 *
 * Please see the file LICENSE_THEORAPLAY.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */
 
 #include <stdio.h>
#include <string.h>
#include <assert.h>

#include "theoraplay.h"
#include "renderer.h"
#include "steam_input.h"

using namespace SDL;

static Uint32 baseticks = 0;

typedef struct AudioQueue {
  const THEORAPLAY_AudioPacket *audio;
  int offset;
  struct AudioQueue *next;
} AudioQueue;

static volatile AudioQueue *audio_queue = NULL;
static volatile AudioQueue *audio_queue_tail = NULL;

static void SDLCALL audio_callback(void *userdata, Uint8 *stream, int len) {
  Sint16 *dst = (Sint16 *)stream;
  float volume = *((float*)userdata);
  while (audio_queue && (len > 0)) {
    volatile AudioQueue *item = audio_queue;
    AudioQueue *next = item->next;
    const int channels = item->audio->channels;

    const float *src = item->audio->samples + (item->offset * channels);
    int cpy = (item->audio->frames - item->offset) * channels;
    int i;

    if (cpy > (len / sizeof(Sint16)))
      cpy = len / sizeof(Sint16);

    for (i = 0; i < cpy; i++) {
      const float val = volume * *(src++);
      if (val < -1.0f)
        *(dst++) = -32768;
      else if (val > 1.0f)
        *(dst++) = 32767;
      else
        *(dst++) = (Sint16)(val * 32767.0f);
    }

    item->offset += (cpy / channels);
    len -= cpy * sizeof(Sint16);

    if (item->offset >= item->audio->frames) {
      THEORAPLAY_freeAudio(item->audio);
      SDL_free((void *)item);
      audio_queue = next;
    }
  }

  if (!audio_queue)
    audio_queue_tail = NULL;

  if (len > 0)
    memset(dst, '\0', len);
}


static void queue_audio(const THEORAPLAY_AudioPacket *audio) {
  AudioQueue *item = (AudioQueue *)SDL_malloc(sizeof(AudioQueue));
  if (!item) {
    THEORAPLAY_freeAudio(audio);
    return;
  }

  item->audio = audio;
  item->offset = 0;
  item->next = NULL;

  SDL_LockAudio();
  if (audio_queue_tail)
    audio_queue_tail->next = item;
  else
    audio_queue = item;
  audio_queue_tail = item;
  SDL_UnlockAudio();
}


void playfile(const char *fname, SDL_Window* screen, Renderer& renderer, float volume) {
  THEORAPLAY_Decoder *decoder = NULL;
  const THEORAPLAY_VideoFrame *video = NULL;
  const THEORAPLAY_AudioPacket *audio = NULL;
  SDL_AudioSpec spec;
  SDL_Event event;
  Uint32 framems = 0;
  int initfailed = 0;
  int quit = 0;

  decoder = THEORAPLAY_startDecodeFile(fname, 30, THEORAPLAY_VIDFMT_RGBA);
  if (!decoder)
    return;
  while (!audio || !video) {
    if (!audio) audio = THEORAPLAY_getAudio(decoder);
    if (!video) video = THEORAPLAY_getVideo(decoder);
    SDL_Delay(10);
  }

  int width = video->width;
  int height = video->height;

  framems = (video->fps == 0.0) ? 0 : ((Uint32)(1000.0 / video->fps));

  memset(&spec, '\0', sizeof(SDL_AudioSpec));
  spec.freq = audio->freq;
  spec.format = AUDIO_S16SYS;
  spec.channels = audio->channels;
  spec.samples = 2048;
  spec.callback = audio_callback;
  spec.userdata = &volume;
  initfailed = quit = (initfailed || (SDL_OpenAudio(&spec, NULL) != 0));
  while (audio) {
    queue_audio(audio);
    audio = THEORAPLAY_getAudio(decoder);
  }

  baseticks = SDL_GetTicks();

  if (!quit)
    SDL_PauseAudio(0);
  auto texture = Texture(Color::BLACK, width, height);
  while (!quit && THEORAPLAY_isDecoding(decoder)) {
    const Uint32 now = SDL_GetTicks() - baseticks;

    if (!video)
      video = THEORAPLAY_getVideo(decoder);

    if (video && (video->playms <= now)) {
      if (framems && ((now - video->playms) >= framems)) {
        const THEORAPLAY_VideoFrame *last = video;
        while ((video = THEORAPLAY_getVideo(decoder)) != NULL) {
          THEORAPLAY_freeVideo(last);
          last = video;
          if ((now - video->playms) < framems)
            break;
        }

        if (!video)
          video = last;
      }
      if (!texture.loadPixels(video->pixels))
        goto exitVideo;
      THEORAPLAY_freeVideo(video);
      video = NULL;
    }
    else {
      SDL_Delay(10);
    }

    while ((audio = THEORAPLAY_getAudio(decoder)) != NULL)
      queue_audio(audio);

    while (renderer.pollEvent(event)) {
      if (event.type == SDL::SDL_WINDOWEVENT && event.window.event == SDL::SDL_WINDOWEVENT_RESIZED)
        renderer.resize(event.window.data1, event.window.data2);
      switch (event.type) {
        case C_BUILDINGS_CONFIRM:
        case C_BUILDINGS_CANCEL:
        case SDL_QUIT:
        case SDL_KEYDOWN:
        case SDL_MOUSEBUTTONDOWN:
          quit = 1;
          break;
        }
    }
    auto screenSize = renderer.getSize();
    ::Rectangle targetRect;
    if (screenSize.x * height >= screenSize.y * width) {
      auto newWidth = width * screenSize.y / height;
      targetRect = ::Rectangle((screenSize.x - newWidth) / 2, 0, (screenSize.x + newWidth) / 2, screenSize.y);
    } else {
      auto newHeight = height * screenSize.x / width;
      targetRect = ::Rectangle(0, (screenSize.y - newHeight) / 2, screenSize.x, (screenSize.y + newHeight) / 2);
    }
    renderer.drawImage(targetRect, ::Rectangle(0, 0, width, height), texture);
    renderer.drawAndClearBuffer();
  }

  while (!quit) {
    SDL_LockAudio();
    quit = (audio_queue == NULL);
    SDL_UnlockAudio();
    if (!quit)
      SDL_Delay(100);
  }

/*  if (initfailed)
    printf("Initialization failed!\n");
  else if (THEORAPLAY_decodingError(decoder))
    printf("There was an error decoding this file!\n");
  else
    printf("done with this file!\n");*/
exitVideo:
  if (video) THEORAPLAY_freeVideo(video);
  if (audio) THEORAPLAY_freeAudio(audio);
  if (decoder) THEORAPLAY_stopDecode(decoder);
  SDL_CloseAudio();
}
