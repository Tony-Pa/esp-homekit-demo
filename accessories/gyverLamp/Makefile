PROGRAM = gyverLamp

EXTRA_COMPONENTS = \
	extras/http-parser \
	extras/dhcpserver \
	$(abspath libraries/adv_button) \
	$(abspath ../../components/esp-8266/wifi_config) \
	$(abspath ../../components/common/wolfssl) \
	$(abspath ../../components/esp-8266/cJSON) \
	$(abspath ../../components/common/homekit)

EXTRA_COMPONENTS += extras/i2s_dma extras/ws2812_i2s

FLASH_SIZE ?= 32

EXTRA_CFLAGS += -I../.. -DHOMEKIT_SHORT_APPLE_UUIDS

include $(SDK_PATH)/common.mk

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)
