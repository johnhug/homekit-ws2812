#ifndef ws2812_h
#define ws2812_h

#include "ws2812_i2s/ws2812_i2s.h"

#define MD_STATIC           0 // solid color
#define MD_STRIPES          1 // alternating color with white
#define MD_COMPLIMENT       2 // alternating color with complementary
#define MD_COMET            3 // comets of color
#define MD_COMET_STRIPES    4 // comets of color with white
#define MD_COMET_COMPLIMENT 5 // comets of color with complementary
#define MD_COMET_RAINBOW    6 // comets of W,B,G,R,Y,M,C

extern const ws2812_pixel_t WHITE;
extern const ws2812_pixel_t RED;
extern const ws2812_pixel_t GREEN;
extern const ws2812_pixel_t BLUE;
extern const ws2812_pixel_t YELLOW;
extern const ws2812_pixel_t MAGENTA;
extern const ws2812_pixel_t CYAN;

void ws2812_init(int pixel_number);

void ws2812_on(bool on);

void ws2812_setColor(ws2812_pixel_t color);

void ws2812_setBrightness(int brightness);

void ws2812_setMode(int mode_index);

void ws2812_setSpeed(int speed);

void ws2812_setInverted(bool inverted);

#endif
