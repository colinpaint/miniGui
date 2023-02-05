#pragma once
#include "miniFB.h"
#include "windowData.h"

//{{{
#if defined(__cplusplus)
  extern "C" {
#endif
//}}}

#define kCall(func, ...)  \
  if (windowData && windowData->func) \
    windowData->func ((struct mfb_window*)windowData, __VA_ARGS__);

//{{{
struct mfb_timer {
  int64_t start_time;
  int64_t delta_counter;
  uint64_t time;
  };
//}}}


extern short int g_keycodes[512];
void keyboard_default (struct mfb_window* window, mfb_key key, mfb_key_mod mod, bool isPressed);

void calc_dst_factor (sWindowData* windowData, uint32_t width, uint32_t height);
void resize_dst (sWindowData* windowData, uint32_t width, uint32_t height);

void set_target_fps_aux();

//{{{
#if defined(__cplusplus)
}
#endif
//}}}