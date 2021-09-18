#include "ws2812.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "FreeRTOS.h"
#include "task.h"
#include <math.h>

ws2812_pixel_t WHITE = {{255,255,255,0}};

int _led_count;
int _order_type;

typedef struct {
	uint8_t blue;
    uint8_t green;
    uint8_t red;
    float brightness;
    bool increasing;
} working_pixel_t; 

working_pixel_t *working_pixels;
ws2812_pixel_t *pixel_buffer;
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

float constrainf(float input) {
	if (input > 1.0f) return 1.0f;
	if (input < _fade * 0.25f) return 0.0f;
	return input;
}

void update() {
	for (int i = 0; i < _led_count; i++) {
		int targetIndex = _reversed ? (_led_count - 1) - i : i;
		working_pixel_t wp = working_pixels[i];
		ws2812_pixel_t *p = &pixel_buffer[targetIndex];
		if (_order_type == OT_RGB) {
			// underlying library assumes ws2812 which is GRB bit ordering
			p->green = wp.red;
			p->red = wp.green;
			p->blue = wp.blue;
		}		
		else {
			// default GRB bit ordering
			p->green = wp.green;
			p->red = wp.red;
			p->blue = wp.blue;
		}
		float adjust = wp.brightness * _brightness;
		p->green *= adjust;
		p->red *= adjust;
		p->blue *= adjust;
	}
	ws2812_i2s_update(pixel_buffer, PIXEL_RGB);
}

void setPixel(int index, ws2812_pixel_t color, float brightnessMod) {
	working_pixel_t *wp = &working_pixels[index];
	wp->red = color.red;
	wp->green = color.green;
	wp->blue = color.blue;
	wp->brightness = brightnessMod;
	wp->increasing = false;
}

void fadePixel(int index, float brightnessMod) {
	working_pixel_t *wp = &working_pixels[index];
	if (wp->increasing) brightnessMod = 1.0f / brightnessMod;
	wp->brightness = constrainf(wp->brightness * brightnessMod);
}

void setFade(int index, bool increasing) {
	working_pixels[index].increasing = increasing;
}

bool isBright(int index) {
	return working_pixels[index].brightness == 1.0f;
}

bool isDark(int index) {
	return working_pixels[index].brightness == 0.0f;
}

void solid() {
	if (_color_count > 0) {
		for (int i = 0; i < _led_count; i++) {
			setPixel(i, _colors[0], 1.0f);
		}
		update();
	}
}

void chase() {
	_position = (_position + 1) % _color_count;
	for (int i = 0; i < _led_count; i++) {
		int mod = i % _color_count;
		setPixel(i, _colors[i % _color_count], 1 / pow(2, floor(abs(_position - mod) / (_color_count * _fade * 0.25f))));
	}
	update();
}

void twinkle() {
	for (int i = 0; i < _led_count; i++) {
		if (isBright(i)) {
			setFade(i, false);
		}
		fadePixel(i, _fade);
	}
	for (int i = _led_count * _density; i > 0; i--) {
		int index = rand() % _led_count;
		if (isDark(index)) {
			setPixel(index, _colors[index % _color_count], _fade * 0.25f);
			setFade(index, true);
		}
	}
	update();
}

void rotation(int width) {
	_position = (_position + 1) % (_color_count * width);
	int colorIndex = (_position / width) % _color_count;
	for (int i = 0; i < _led_count; i++) {
		int mod = (i + _position) % width;
		if (mod == 0 && i > 0) colorIndex = (colorIndex + 1) % _color_count;

		setPixel(i, _colors[colorIndex], 1.0f);
	}
	update();
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
	update();
}

void fireworks() {
	for (int i = 0; i < _led_count; i++) {
		fadePixel(i, _fade);
	}
	for (int i = _led_count * _density; i > 0; i--) {
		int index = rand() % _led_count;
		setPixel(index, _colors[rand() % _color_count], 1.0f);
	}
	update();
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

void ws2812_init(int pixel_number, int order_type) {
	time_t t;
	srand((unsigned) time(&t));
	
	_led_count = pixel_number;
	_order_type = order_type;

	working_pixels = (working_pixel_t*) malloc(_led_count * sizeof(working_pixel_t));
	pixel_buffer = (ws2812_pixel_t*) malloc(_led_count * sizeof(ws2812_pixel_t));
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
