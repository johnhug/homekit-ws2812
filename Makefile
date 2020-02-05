PROGRAM = homekit-ws2812

EXTRA_COMPONENTS = \
	extras/http-parser \
	extras/i2s_dma \
	extras/ws2812_i2s \
	$(abspath ../esp-cjson) \
	$(abspath ../esp-wolfssl) \
	$(abspath ../esp-homekit)

FLASH_SIZE ?= 32
# FLASH_SIZE ?= 8
# HOMEKIT_SPI_FLASH_BASE_ADDR ?= 0x7A000
HOMEKIT_SPI_FLASH_BASE_ADDR=0x7A000

EXTRA_CFLAGS += -I../.. -DHOMEKIT_SHORT_APPLE_UUIDS

include $(SDK_PATH)/common.mk
include wifi.h

LIBS += m

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)
