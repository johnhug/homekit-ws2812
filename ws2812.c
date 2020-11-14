#include "ws2812.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "FreeRTOS.h"
#include "task.h"
#include <math.h>

ws2812_pixel_t WHITE = {{255,255,255,0}};

int _led_count;

ws2812_pixel_t *pixels;
ws2812_pixel_t *black;

bool _running = true;
int _position = -1;
float _brightness = 1.0f;
int _color_count = 0;
ws2812_pixel_t *_colors;
int _mode_index = MD_SOLID;
bool _reversed = false;
int _delay = 0;
float _delay_factor = 1.0f;
float _density = 0.25f;
float _fade = 0.5f;

int constrain(int input, int min, int max) {
	if (input < min) return min;
	if (input > max) return max;
	return input;
}

void setPixel(int index, ws2812_pixel_t color, float brightnessMod) {
	if (_reversed) index = (_led_count - 1) - index;
	pixels[index].red = color.red * _brightness * brightnessMod;
	pixels[index].green = color.green * _brightness * brightnessMod;
	pixels[index].blue = color.blue * _brightness * brightnessMod;
}

bool isBlack(int index) {
	ws2812_pixel_t p = pixels[index];
	return p.red + p.green + p.blue == 0;
}

void solid() {
	if (_color_count > 0) {
		for (int i = 0; i < _led_count; i++) {
			setPixel(i, _colors[0], 1.0f);
		}
		ws2812_i2s_update(pixels, PIXEL_RGB);
	}
}

void chase() {
	_position = (_position + 1) % _color_count;
	for (int i = 0; i < _led_count; i++) {
		int mod = i % _color_count;
		setPixel(i, _colors[i % _color_count], 1 / pow(2, floor(abs(_position - mod) / (_color_count * _fade * 0.25f))));
	}
	ws2812_i2s_update(pixels, PIXEL_RGB);
}

void twinkle() {
	for (int i = 0; i < _led_count; i++) {
		setPixel(i, pixels[i], _fade);
	}
	for (int i = _led_count * _density; i > 0; i--) {
		int index = rand() % _led_count;
		if (isBlack(index)) {
			setPixel(index, _colors[index % _color_count], 1.0f);
		}
	}
	ws2812_i2s_update(pixels, PIXEL_RGB);
}

void rotation(int width) {
	_position = (_position + 1) % (_color_count * width);
	int colorIndex = (_position / width) % _color_count;
	for (int i = 0; i < _led_count; i++) {
		int mod = (i + _position) % width;
		if (mod == 0 && i > 0) colorIndex = (colorIndex + 1) % _color_count;

		setPixel(i, _colors[colorIndex], 1.0f);
	}
	ws2812_i2s_update(pixels, PIXEL_RGB);
}

void comets() {
	int width = _led_count * _density;
	_position = (_position + 1) % (_color_count * width);
	int colorIndex = (_position / width) % _color_count;
	for (int i = 0; i < _led_count; i++) {
		int mod = (i + _position) % width;
		if (mod == 0 && i > 0) colorIndex = (colorIndex + 1) % _color_count;
		if (mod == 0) {
			setPixel(i, WHITE, 1.0f);
		}
		else {
			setPixel(i, _colors[colorIndex], 1 / pow(2, floor(mod / (width * _fade * 0.25f))));
		}
	}
	ws2812_i2s_update(pixels, PIXEL_RGB);
}

void fireworks() {
	for (int i = 0; i < _led_count; i++) {
		setPixel(i, pixels[i], _fade);
	}
	for (int i = _led_count * _density; i > 0; i--) {
		int index = rand() % _led_count;
		setPixel(index, _colors[rand() % _color_count], 1.0f);
	}
	ws2812_i2s_update(pixels, PIXEL_RGB);
}

void ws2812_service(void *_args) {
	uint32_t now = 0;
	uint32_t last_call_time = 0;

	while (true) {
		if (_running) {
			now = xTaskGetTickCount() * portTICK_PERIOD_MS;
			
			if (now - last_call_time > _delay * _delay_factor) {
				last_call_time = now;

				switch (_mode_index) {
					case MD_SOLID:
						_delay_factor = 1.0;
						solid();
						break;
					case MD_CHASE:
						_delay_factor = 2.0;
						chase();
						break;
					case MD_TWINKLE:
						_delay_factor = 2.0;
						twinkle();
						break;
					case MD_SEQUENCE:
						_delay_factor = 1.5f;
						rotation(1);
						break;
					case MD_STRIPES:
						_delay_factor = 1.0;
						rotation(_led_count * _density);
						break;
					case MD_COMETS:
						_delay_factor = 2.0;
						comets();
						break;
					case MD_FIREWORKS:
						_delay_factor = 2.0;
						fireworks();
						break;
					default:
						solid();
				}
			}
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void ws2812_init(int pixel_number) {
	time_t t;
	srand((unsigned) time(&t));
	
	_led_count = pixel_number;

	pixels = (ws2812_pixel_t*) malloc(_led_count * sizeof(ws2812_pixel_t));
	black = (ws2812_pixel_t*) malloc(_led_count * sizeof(ws2812_pixel_t));

	for (int i = 0; i < _led_count; i++) {
		black[i].red = black[i].green = black[i].blue = 0;
	}
	
	ws2812_i2s_init(_led_count, PIXEL_RGB);

	xTaskCreate(ws2812_service, "ws2812Service", 255, NULL, 2, NULL);
}

void ws2812_on(bool on) {
	_running = on;
	if (!_running) {
		ws2812_i2s_update(black, PIXEL_RGB);
	}
	printf("ws2812: on: %d\n", _running);
}

void ws2812_setColors(int color_count, ws2812_pixel_t *colors) {
	_color_count = color_count;
	_colors = colors;
	printf("ws2812: setColors: color_count: %d\n", _color_count);
	for (int i = 0; i < _color_count; i++) {
		printf("ws2812: setColors: color: %d %02x%02x%02x\n", 
			i, _colors[i].red, _colors[i].green, _colors[i].blue);
	}
}

void ws2812_setBrightness(int brightness) {
	_brightness = brightness / 100.0f;
	printf("ws2812: setBrightness: %f\n", _brightness);
}

void ws2812_setMode(int mode_index) {
	_mode_index = mode_index;
	printf("ws2812: setMode: %d\n", _mode_index);
}

void ws2812_setSpeed(int speed) {
	// max delay 250ms, min delay 0ms
	_delay = speed * -2.5f + 250;
	printf("ws2812: setSpeed: %d\n", _delay);
}

void ws2812_setReverseDirection(bool reversed) {
	_reversed = reversed;
	printf("ws2812: setReversedDirection: %d\n", _reversed);
}

void ws2812_setDensity(int density) {
	_density = density / 100.0f;
	printf("ws2812: setDensity: %f\n", _density);
}

void ws2812_setFade(int fade) {
	_fade = (100.0f - fade) / 100.0f;
	printf("ws2812: setFade: %f\n'", _fade);
}
