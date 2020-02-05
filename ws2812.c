#include "ws2812.h"

#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include <math.h>

// B G R W order on struct (ignoring white value)
const ws2812_pixel_t WHITE      = {{255,255,255,  0}};
const ws2812_pixel_t RED        = {{  0,  0,255,  0}};
const ws2812_pixel_t GREEN      = {{  0,255,  0,  0}};
const ws2812_pixel_t BLUE       = {{255,  0,  0,  0}};
const ws2812_pixel_t YELLOW     = {{  0,255,255,  0}};
const ws2812_pixel_t MAGENTA	= {{255,  0,255,  0}};
const ws2812_pixel_t CYAN       = {{255,255,  0,  0}};

#define RAINBOW_N 7
const ws2812_pixel_t *const RAINBOW[] = {&WHITE,&BLUE,&GREEN,&RED,&YELLOW,&MAGENTA,&CYAN};

int _led_count;

ws2812_pixel_t *pixels;
ws2812_pixel_t *black;

bool _running = true;
int _position = -1;
float _brightness = 1.0f;
ws2812_pixel_t _solid_color = {{255,255,255,  0}};
int _mode_index = MD_STATIC;
bool _inverted = false;
int _mode_delay = 0;

int constrain(int input, int min, int max) {
	if (input < min) return min;
	if (input > max) return max;
	return input;
}

void setPixel(int index, ws2812_pixel_t color, float brightnessMod) {
	if (_inverted) index = (_led_count - 1) - index;
	pixels[index].red = color.red * _brightness * brightnessMod;
	pixels[index].green = color.green * _brightness * brightnessMod;
	pixels[index].blue = color.blue * _brightness * brightnessMod;
}

void solid() {
	for (int i = 0; i < _led_count; i++) {
		setPixel(i, _solid_color, 1.0f);
	}
	ws2812_i2s_update(pixels, PIXEL_RGB);
}

const int COMET = 15;
void comets(bool rainbow) {
	_position = (_position + 1) % ((RAINBOW_N * COMET) % _led_count);
	int rainbowIndex = (_position / COMET) % RAINBOW_N;

	for (int i = 0; i < _led_count; i++) {
		int mod = (i + _position) % COMET;
		if (mod == 0 && i > 0) rainbowIndex = (rainbowIndex + 1) % RAINBOW_N;
		
		// W 2 2 4 4 8 8 16 16 32 32 64 64 128 128
		if (mod == 0) {
			setPixel(i, WHITE, 1.0f);
		}
		else {
			float fade = 1 / (float) pow(2, floor(mod / 2));
			if (rainbow)
				setPixel(i, *RAINBOW[rainbowIndex], fade);
			else
				setPixel(i, _solid_color, fade);
		}
	}
	ws2812_i2s_update(pixels, PIXEL_RGB);
}

void ws2812_service(void *_args) {
	uint32_t now = 0;
	uint32_t last_call_time = 0;

	while (true) {
		if (_running) {
			now = xTaskGetTickCount() * portTICK_PERIOD_MS;
			
			if (now - last_call_time > _mode_delay) {
				last_call_time = now;
				
				switch (_mode_index) {
					case MD_STATIC:
						solid();
						break;
					case MD_COMPLIMENT:
						solid();
						break;
					case MD_COMET:
						comets(false);
						break;
					case MD_COMET_STRIPES:
						comets(false);
						break;
					case MD_COMET_COMPLIMENT:
						comets(false);
						break;
					case MD_COMET_RAINBOW:
						comets(true);
						break;
					default:
						solid();
				}
			}
		}
		vTaskDelay(33 / portTICK_PERIOD_MS);
	}
}

void ws2812_init(int pixel_number) {
	_led_count = pixel_number;

	pixels = (ws2812_pixel_t*) malloc(_led_count * sizeof(ws2812_pixel_t));
	black = (ws2812_pixel_t*) malloc(_led_count * sizeof(ws2812_pixel_t));

	for (int i = 0; i < _led_count; i++) {
		black[i].red = 0;
		black[i].green = 0;
		black[i].blue = 0;
	}
	
	ws2812_i2s_init(_led_count, PIXEL_RGB);

	xTaskCreate(ws2812_service, "ws2812Service", 255, NULL, 2, NULL);
}

void ws2812_on(bool on) {
	_running = on;
	if (!_running) {
		ws2812_i2s_update(black, PIXEL_RGB);
	}
}

void ws2812_setColor(ws2812_pixel_t color) {
	_solid_color.red = color.red;
	_solid_color.green = color.green;
	_solid_color.blue = color.blue;
}

void ws2812_setBrightness(int brightness) {
	_brightness = brightness / 100.0f;
}

void ws2812_setMode(int mode_index) {
	_mode_index = mode_index;
}

void ws2812_setSpeed(int speed) {
	// max delay 250ms, min delay 0ms
	_mode_delay = abs((speed - 0) * (0 - 250) / (100 - 0) + 250);
}

void ws2812_setInverted(bool inverted) {
	_inverted = inverted;
}
