#ifndef converters_h
#define converters_h

#include "ws2812_i2s/ws2812_i2s.h"

#define LED_RGB_SCALE 255
#define WHITE_HUE_PIVOT 66

//http://blog.saikoled.com/post/44677718712/how-to-convert-from-hsi-to-rgb-white
static void hsi2rgb(float h, float s, float i, ws2812_pixel_t* rgb) {
    int r, g, b;

    while (h < 0) { h += 360.0F; };     // cycle h around to 0-360 degrees
    while (h >= 360) { h -= 360.0F; };
    h = 3.14159F*h / 180.0F;            // convert to radians.
    s /= 100.0F;                        // from percentage to ratio
    i /= 100.0F;                        // from percentage to ratio
    s = s > 0 ? (s < 1 ? s : 1) : 0;    // clamp s and i to interval [0,1]
    i = i > 0 ? (i < 1 ? i : 1) : 0;    // clamp s and i to interval [0,1]
    i = i * sqrt(i);                    // shape intensity to have finer granularity near 0

    if (h < 2.09439) {
        r = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
        g = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
        b = LED_RGB_SCALE * i / 3 * (1 - s);
    }
    else if (h < 4.188787) {
        h = h - 2.09439;
        g = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
        b = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
        r = LED_RGB_SCALE * i / 3 * (1 - s);
    }
    else {
        h = h - 4.188787;
        b = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
        r = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
        g = LED_RGB_SCALE * i / 3 * (1 - s);
    }

    rgb->red = (uint8_t) r;
    rgb->green = (uint8_t) g;
    rgb->blue = (uint8_t) b;
    rgb->white = (uint8_t) 0;           // white channel is not used
}

static void limit_255(int* c) {
    if (*c < 0) *c = 0;
    if (*c > 255) *c = 255;
}

//http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
static void mired2rgb(uint32_t m, ws2812_pixel_t* rgb) {
    int k = (1000000 / m) / 100;

    int r, g, b;

    // red
    if (k <= WHITE_HUE_PIVOT) {
        r = 255;
    }
    else {
        r = k - 60;
        r = 329.698727446 * pow(r, -0.1332047592);
        limit_255(&r);
    }
    
    // green
    if (k <= WHITE_HUE_PIVOT) {
        g = k;
        g = 99.4708025861 * log(g) - 161.1195681661;
        limit_255(&g);
    }
    else {
        g = k - 60;
        g = 288.1221695283 * pow(g, -0.0755148492);
        limit_255(&g);
    }
    
    // blue
    if (k >= WHITE_HUE_PIVOT) {
        b = 255;
    }
    else {
        if (k <= 19) {
            b = 0;
        }
        else {
            b = k - 10;
            b = 138.5177312231 * log(b) - 305.0447927307;
            limit_255(&b);
        }
    }
    
    rgb->red = (uint8_t) r;
    rgb->green = (uint8_t) g;
    rgb->blue = (uint8_t) b;
    rgb->white = (uint8_t) 0;           // white channel is not used
}

#endif
