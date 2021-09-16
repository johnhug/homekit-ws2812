#ifndef ws2812_h
#define ws2812_h

#include "ws2812_i2s/ws2812_i2s.h"

#define MD_SOLID            1 // solid color
#define MD_CHASE			2 // static colors with dimming chase
#define MD_TWINKLE			3 // static colors with twinkle dimming
#define MD_SEQUENCE         4 // sequence of colors 1 pixel each
#define MD_STRIPES          5 // stripes of colors density variable pixels each
#define MD_COMETS           6 // comets of colors
#define MD_FIREWORKS        7 // random pixels light up with one of the colors and fade

// byte order type for WS281x serial data protocol
#define OT_GRB				0
#define OT_RGB				1

void ws2812_init(int pixel_number, int order_type);

void ws2812_on(bool on);

void ws2812_setColors(int color_count, ws2812_pixel_t *colors);

void ws2812_setBrightness(int brightness);

void ws2812_setMode(int mode_index);

void ws2812_setSpeed(int speed);

void ws2812_setReverseDirection(bool reversed);

void ws2812_setDensity(int density);

void ws2812_setFade(int fade);

#endif
