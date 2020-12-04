/*
* HomeKit interface for ws2812 led strip.
* 
* Derived from the led_strip_animation example
* in https://github.com/maximkulkin/esp-homekit-demo
*/
#include <stdio.h>
#include <stdlib.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <math.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>

#include "converters.h"
#include "ws2812.h"

#define LED_COUNT 299      // this is the number of WS2812B leds on the strip
#define LED_INBUILT_GPIO 2 // this is the onboard LED used to show on/off only

#define UUID_MODE       "1C52000A-457C-4D3C-AABA-E6F207422A10"
#define UUID_SPEED      "1C52000A-457C-4D3C-AABA-E6F207422A11"
#define UUID_REVERSE    "1C52000A-457C-4D3C-AABA-E6F207422A12"
#define UUID_DENSITY    "1C52000A-457C-4D3C-AABA-E6F207422A13"
#define UUID_FADE       "1C52000A-457C-4D3C-AABA-E6F207422A14"
#define UUID_COUNT      "1C52000A-457C-4D3C-AABA-E6F207422A15"
#define UUID_MODE_NAME  "1C52000A-457C-4D3C-AABA-E6F207422A16"

ws2812_pixel_t *_colors;

// Home Kit variables
bool hk_on[]          = {true, true, true, true, true, true, true};
float hk_hue[]        = {   0,  240,  120,  360,  180,   60,  300};
float hk_saturation[] = {   0,  100,  100,  100,  100,  100,  100};
int hk_brightness = 100;
int hk_mode = 1;
int hk_speed = 100;
bool hk_reverse = false;
int hk_density = 25;
int hk_fade = 50;

void identify_task(void *_args) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 9; j++) {
            gpio_write(LED_INBUILT_GPIO, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
            gpio_write(LED_INBUILT_GPIO, 1);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void identify(homekit_value_t _value) {
    xTaskCreate(identify_task, "LED identify", 128, NULL, 2, NULL);
}

void updateColors() {
    int color_count = 0;
    for (int i = 0; i < 7; i++) {
        if (hk_on[i]) color_count++;
    }
    ws2812_pixel_t *colors = (ws2812_pixel_t*) malloc(color_count * sizeof(ws2812_pixel_t));
    
    int color_idx = 0;
    for (int i = 0; i < 7; i++) {
        if (hk_on[i]) {
            ws2812_pixel_t color = {{ 0, 0, 0, 0 }};
            hs2rgb(hk_hue[i], hk_saturation[i] / 100.0f, &color);
            colors[color_idx] = color;
            color_idx++;
        }
    }
    ws2812_pixel_t *old = _colors;
    _colors = colors;
    ws2812_setColors(color_count, _colors);
    free(old);
}

int getColorIndex(const homekit_characteristic_t *ch) {
    return ch->service->id - 1;
}

homekit_value_t led_on_get(const homekit_characteristic_t *ch) {
    int index = getColorIndex(ch);
    return HOMEKIT_BOOL(hk_on[index] && hk_on[0]);
}

void led_on_set(homekit_characteristic_t *ch, const homekit_value_t value) {
    int index = getColorIndex(ch);
    hk_on[index] = value.bool_value;
    if (index == 0) ws2812_on(value.bool_value);
    updateColors();
}

homekit_value_t led_brightness_get() {
    return HOMEKIT_INT(hk_brightness);
}

void led_brightness_set(homekit_value_t value) {
    hk_brightness = value.int_value;
    ws2812_setBrightness(hk_brightness);
}

homekit_value_t led_hue_get(const homekit_characteristic_t *ch) {
    int index = getColorIndex(ch);
    return HOMEKIT_FLOAT(hk_hue[index]);
}

void led_hue_set(homekit_characteristic_t *ch, const homekit_value_t value) {
    int index = getColorIndex(ch);
    hk_hue[index] = value.float_value;
    updateColors();
}

homekit_value_t led_saturation_get(const homekit_characteristic_t *ch) {
    int index = getColorIndex(ch);
    return HOMEKIT_FLOAT(hk_saturation[index]);
}

void led_saturation_set(homekit_characteristic_t *ch, const homekit_value_t value) {
    int index = getColorIndex(ch);
    hk_saturation[index] = value.float_value;
    updateColors();
}

homekit_value_t led_mode_get() {
    return HOMEKIT_INT(hk_mode);
}

void led_mode_set(homekit_value_t value) {
    hk_mode = value.int_value;
    ws2812_setMode(hk_mode);
}

homekit_value_t led_mode_name_get() {
    char *name;
    switch (hk_mode) {
        case MD_SOLID: 
            name = "Solid";
            break;
        case MD_CHASE: 
            name = "Chase";
            break;
        case MD_TWINKLE: 
            name = "Twinkle";
            break;
        case MD_SEQUENCE: 
            name = "Sequence";
            break;
        case MD_STRIPES: 
            name = "Sripes";
            break;
        case MD_COMETS:
            name = "Comets";
            break;
        case MD_FIREWORKS: 
            name = "Fireworks";
            break;
        default: 
            name = "Ooopies";
    }
    return HOMEKIT_STRING(name, .is_static=true);
}

homekit_value_t led_speed_get() {
    return HOMEKIT_INT(hk_speed);
}

void led_speed_set(homekit_value_t value) {
    hk_speed = value.int_value;
    ws2812_setSpeed(hk_speed);
}

homekit_value_t led_reverse_get() {
    return HOMEKIT_BOOL(hk_reverse);
}

void led_reverse_set(homekit_value_t value) {
    hk_reverse = value.bool_value;
    ws2812_setReverseDirection(hk_reverse);
}

homekit_value_t led_density_get() {
    return HOMEKIT_INT(hk_density);
}

void led_density_set(homekit_value_t value) {
    hk_density = value.int_value;
    ws2812_setDensity(hk_density);
}

homekit_value_t led_fade_get() {
    return HOMEKIT_INT(hk_fade);
}

void led_fade_set(homekit_value_t value) {
    hk_fade = value.int_value;
    ws2812_setFade(hk_fade);
}

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "Chihiro");

homekit_service_t color_2 = 
    HOMEKIT_SERVICE_(LIGHTBULB, .id = 2, .primary = false, .characteristics = (homekit_characteristic_t*[]) {
        HOMEKIT_CHARACTERISTIC(NAME, "Color-2"),
        HOMEKIT_CHARACTERISTIC(
            ON, true,
        .getter_ex = led_on_get,
        .setter_ex = led_on_set
            ),
        HOMEKIT_CHARACTERISTIC(
            HUE, 240,
        .getter_ex = led_hue_get,
        .setter_ex = led_hue_set
            ),
        HOMEKIT_CHARACTERISTIC(
            SATURATION, 100,
        .getter_ex = led_saturation_get,
        .setter_ex = led_saturation_set
            ),
        NULL
    });
    
homekit_service_t color_3 = 
    HOMEKIT_SERVICE_(LIGHTBULB, .id = 3, .primary = false, .characteristics = (homekit_characteristic_t*[]) {
        HOMEKIT_CHARACTERISTIC(NAME, "Color-3"),
        HOMEKIT_CHARACTERISTIC(
            ON, true,
        .getter_ex = led_on_get,
        .setter_ex = led_on_set
            ),
        HOMEKIT_CHARACTERISTIC(
            HUE, 120,
        .getter_ex = led_hue_get,
        .setter_ex = led_hue_set
            ),
        HOMEKIT_CHARACTERISTIC(
            SATURATION, 100,
        .getter_ex = led_saturation_get,
        .setter_ex = led_saturation_set
            ),
        NULL
    });
    
homekit_service_t color_4 = 
    HOMEKIT_SERVICE_(LIGHTBULB, .id = 4, .primary = false, .characteristics = (homekit_characteristic_t*[]) {
        HOMEKIT_CHARACTERISTIC(NAME, "Color-4"),
        HOMEKIT_CHARACTERISTIC(
            ON, true,
        .getter_ex = led_on_get,
        .setter_ex = led_on_set
            ),
        HOMEKIT_CHARACTERISTIC(
            HUE, 360,
        .getter_ex = led_hue_get,
        .setter_ex = led_hue_set
            ),
        HOMEKIT_CHARACTERISTIC(
            SATURATION, 100,
        .getter_ex = led_saturation_get,
        .setter_ex = led_saturation_set
            ),
        NULL
    });
    
homekit_service_t color_5 = 
    HOMEKIT_SERVICE_(LIGHTBULB, .id = 5, .primary = false, .characteristics = (homekit_characteristic_t*[]) {
        HOMEKIT_CHARACTERISTIC(NAME, "Color-5"),
        HOMEKIT_CHARACTERISTIC(
            ON, true,
        .getter_ex = led_on_get,
        .setter_ex = led_on_set
            ),
        HOMEKIT_CHARACTERISTIC(
            HUE, 180,
        .getter_ex = led_hue_get,
        .setter_ex = led_hue_set
            ),
        HOMEKIT_CHARACTERISTIC(
            SATURATION, 100,
        .getter_ex = led_saturation_get,
        .setter_ex = led_saturation_set
            ),
        NULL
    });
    
homekit_service_t color_6 = 
    HOMEKIT_SERVICE_(LIGHTBULB, .id = 6, .primary = false, .characteristics = (homekit_characteristic_t*[]) {
        HOMEKIT_CHARACTERISTIC(NAME, "Color-6"),
        HOMEKIT_CHARACTERISTIC(
            ON, true,
        .getter_ex = led_on_get,
        .setter_ex = led_on_set
            ),
        HOMEKIT_CHARACTERISTIC(
            HUE, 60,
        .getter_ex = led_hue_get,
        .setter_ex = led_hue_set
            ),
        HOMEKIT_CHARACTERISTIC(
            SATURATION, 100,
        .getter_ex = led_saturation_get,
        .setter_ex = led_saturation_set
            ),
        NULL
    });

homekit_service_t color_7 = 
    HOMEKIT_SERVICE_(LIGHTBULB, .id = 7, .primary = false, .characteristics = (homekit_characteristic_t*[]) {
        HOMEKIT_CHARACTERISTIC(NAME, "Color-7"),
        HOMEKIT_CHARACTERISTIC(
            ON, true,
        .getter_ex = led_on_get,
        .setter_ex = led_on_set
            ),
        HOMEKIT_CHARACTERISTIC(
            HUE, 300,
        .getter_ex = led_hue_get,
        .setter_ex = led_hue_set
            ),
        HOMEKIT_CHARACTERISTIC(
            SATURATION, 100,
        .getter_ex = led_saturation_get,
        .setter_ex = led_saturation_set
            ),
        NULL
    });

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(
        .id = 1,
        .category = homekit_accessory_category_lightbulb,
        .services = (homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .id = 100, .characteristics = (homekit_characteristic_t*[]) {
            &name,
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "John Hug"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "2baff4898fe9"),
            HOMEKIT_CHARACTERISTIC(MODEL, "WS2812-FX"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.5"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, identify),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB,
            .id = 1, 
            .primary = true,
            .characteristics = (homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Chihiro"),
            HOMEKIT_CHARACTERISTIC(
                ON, true,
            .getter_ex = led_on_get,
            .setter_ex = led_on_set
                ),
            HOMEKIT_CHARACTERISTIC(
                BRIGHTNESS, 100,
            .getter = led_brightness_get,
            .setter = led_brightness_set
                ),
            HOMEKIT_CHARACTERISTIC(
                HUE, 0,
            .getter_ex = led_hue_get,
            .setter_ex = led_hue_set
                ),
            HOMEKIT_CHARACTERISTIC(
                SATURATION, 0,
            .getter_ex = led_saturation_get,
            .setter_ex = led_saturation_set
                ),
            HOMEKIT_CHARACTERISTIC(
                CUSTOM,
            .type = UUID_MODE,
            .description = "FXMode",
            .format = homekit_format_int,
            .permissions = homekit_permissions_paired_read
                         | homekit_permissions_paired_write,
            .min_value = (float[]) {1},
            .max_value = (float[]) {7},
            .min_step = (float[]) {1},
            .value = HOMEKIT_INT_(1),
            .getter = led_mode_get,
            .setter = led_mode_set
                ),
            HOMEKIT_CHARACTERISTIC(
                CUSTOM,
            .type = UUID_MODE_NAME,
            .description = "FXModeName",
            .format = homekit_format_string,
            .permissions = homekit_permissions_paired_read,
            .value = HOMEKIT_STRING_("Init", .is_static=true),
            .getter = led_mode_name_get
                ),
            HOMEKIT_CHARACTERISTIC(
                CUSTOM,
            .type = UUID_SPEED,
            .description = "Speed",
            .format = homekit_format_int,
            .permissions = homekit_permissions_paired_read
                         | homekit_permissions_paired_write,
            .min_value = (float[]) {0},
            .max_value = (float[]) {100},
            .min_step = (float[]) {1},
            .value = HOMEKIT_INT_(100),
            .getter = led_speed_get,
            .setter = led_speed_set
                ),
            HOMEKIT_CHARACTERISTIC(
                CUSTOM,
            .type = UUID_REVERSE,
            .description = "Reverse",
            .format = homekit_format_bool,
            .permissions = homekit_permissions_paired_read
                         | homekit_permissions_paired_write,
            .value = HOMEKIT_BOOL_(false),
            .getter = led_reverse_get,
            .setter = led_reverse_set
                ),
            HOMEKIT_CHARACTERISTIC(
                CUSTOM,
            .type = UUID_DENSITY,
            .description = "Density",
            .format = homekit_format_int,
            .permissions = homekit_permissions_paired_read
                         | homekit_permissions_paired_write,
            .min_value = (float[]) {1},
            .max_value = (float[]) {100},
            .min_step = (float[]) {1},
            .value = HOMEKIT_INT_(25),
            .getter = led_density_get,
            .setter = led_density_set
                ),
            HOMEKIT_CHARACTERISTIC(
                CUSTOM,
            .type = UUID_FADE,
            .description = "Fade",
            .format = homekit_format_int,
            .permissions = homekit_permissions_paired_read
                         | homekit_permissions_paired_write,
            .min_value = (float[]) {1},
            .max_value = (float[]) {100},
            .min_step = (float[]) {1},
            .value = HOMEKIT_INT_(50),
            .getter = led_fade_get,
            .setter = led_fade_set
                ),
            HOMEKIT_CHARACTERISTIC(
                CUSTOM,
            .type = UUID_COUNT,
            .description = "LEDCount",
            .format = homekit_format_int,
            .permissions = homekit_permissions_paired_read,
            .value = HOMEKIT_INT_(LED_COUNT),
                ),
            NULL
        },
        .linked = (homekit_service_t*[]) {
            &color_2, 
            &color_3, 
            &color_4, 
            &color_5, 
            &color_6, 
            &color_7,
            NULL
        }),
        &color_2,
        &color_3,
        &color_4,
        &color_5,
        &color_6,
        &color_7,
        NULL
    }),
    NULL
};

void on_homekit_event(homekit_event_t event) {
    if (event == HOMEKIT_EVENT_CLIENT_VERIFIED) {
        identify(HOMEKIT_INT(1));    
    }
}

homekit_server_config_t config = {
    .accessories = accessories,
    .category = homekit_accessory_category_lightbulb,
    .password = "925-97-411",
    .setupId = "8327",
    .on_event = on_homekit_event
};

void on_wifi_ready() {
    identify(HOMEKIT_INT(1));    
}

void user_init(void) {
    gpio_enable(LED_INBUILT_GPIO, GPIO_OUTPUT);

    updateColors();
    ws2812_init(LED_COUNT);

    wifi_config_init("Chihiro", NULL, on_wifi_ready);

    uint8_t macaddr[6];
    sdk_wifi_get_macaddr(STATION_IF, macaddr);
    int name_len = 7 + 1 + 6 + 1;
    char *name_value = malloc(name_len);
    snprintf(name_value, name_len, "Chihiro-%02X%02X%02X", macaddr[3], macaddr[4], macaddr[5]);
    name.value = HOMEKIT_STRING(name_value);

    homekit_server_init(&config);
}
