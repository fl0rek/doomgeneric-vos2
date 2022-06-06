#include "doomgeneric.h"
#include "doomkeys.h"
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

uint32_t start_ms = 0;
int fb_dev = 0;
uint32_t *fb_buff = 0;
size_t buffsize = 0;

size_t offset = 0;

int ev_dev = 0;

uint32_t get_time_ms() {
  struct timeval tv;
  gettimeofday(&tv, 0);

  return (tv.tv_sec * 1000) + (uint32_t) (tv.tv_usec /1000);

}

uint32_t DG_GetTicksMs() {
  return get_time_ms();
}

void DG_Init() {
  start_ms = get_time_ms();

  fb_dev = open("/dev/fb0", O_RDWR);
  if (fb_dev < 0) {
    perror("error opening fb");
    exit(-1);
  }

  struct fb_var_screeninfo info;

  if (ioctl(fb_dev, FBIOGET_VSCREENINFO, &info)) {
    perror("could not get info");
    exit(-1);
  }
  printf("xres: %d, yres: %d\n", info.xres, info.yres);

  //info.yres /= 2;

  buffsize = 4 * info.xres * info.yres;

  fb_buff = mmap(NULL, buffsize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_dev, 0);

  if (fb_buff == MAP_FAILED) {
    perror("Could not mmap");
    exit(-1);
  }

  ev_dev = open("/dev/input/event2", O_RDONLY);
  if (ev_dev < 0) {
    perror("error opening ev");
    exit(-1);
  }
  int flags = fcntl(ev_dev, F_GETFL, 0);
  fcntl(ev_dev, F_SETFL, flags | O_NONBLOCK);
}

void DG_SetWindowTitle(const char * title) {
  printf("<%s>\n", title);
}

void DG_SleepMs(uint32_t ms) {
  usleep(ms * 1000);
}

void DG_DrawFrame() {
  memcpy(fb_buff, DG_ScreenBuffer, buffsize);

  /*
  fb_buff[offset++] = -1;

  offset %= buffsize;
  */
}

// codes
// 2-10 - numpad
// 55 - *
// 11 - 10
// 523 - #
// 1 - red X
// 14 - yellow <
// 28 - green o

int DG_GetKey(int* pressed, unsigned char *doomKey) {
  struct input_event input;
  while (1) {
    ssize_t s = read(ev_dev, &input, sizeof input);
    if (s <= 0) {
      return 0;
    }
    if (input.type != EV_KEY) {
      continue;
    }
    unsigned char doomkey = 0;
    switch (input.code) {
      case 3:
        doomkey = KEY_UPARROW;
        break;
      case 2:
        doomkey = KEY_LEFTARROW;
        break;
      case 6:
        doomkey = KEY_DOWNARROW;
        break;
      case 4:
        doomkey = KEY_RIGHTARROW;
        break;
      case 5:
        doomkey = KEY_STRAFE_L;
        break;
      case 7:
        doomkey = KEY_STRAFE_R;
        break;
      case 28:
        doomkey = KEY_FIRE;
        break;
      case 14:
        doomkey = KEY_USE;
        break;
      case 1:
        doomkey = KEY_ESCAPE;
        break;
      case 8:
      case 9:
      case 10:
      case 523:
        printf("enter\n");
        doomkey = 13;
        break;
      case 55:
        doomkey = KEY_RSHIFT;
        break;
    }
    doomkey &= 0xFF;

    printf("event, code:% 10hd, % 10s, doomkey: % 3x, % 3d\n", input.code, input.value ? "press" : "release", doomkey, doomkey);

    *pressed = input.value;
    *doomKey = doomkey;
    return 1;
  }
  return 0;
}
