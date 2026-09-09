XC8 ?= xc8-cc
PYTHON ?= python3

MCU := 18F45K50
BUILD_DIR := build
DIST_DIR := dist
BOOT_DIR := bootloader/TECKIO_USB_Bootloader.X
APP_DIR := application/Aplicacion_TECKIO.X
USB_ROOT := third_party/USB-Stack/USB_Stack
USB_CORE := $(USB_ROOT)/USB
USB_CDC_SHARED := $(USB_ROOT)/Examples/CDC_Examples/Shared_Files

COMMON_FLAGS := -mcpu=$(MCU) -std=c99 -O1 -mwarn=-3 -DXPRJ_default=default \
	-I$(USB_CORE) -I$(USB_CDC_SHARED)

USB_SOURCES := \
	$(USB_CORE)/usb.c \
	$(USB_CORE)/usb_cdc_acm.c \
	$(USB_CDC_SHARED)/usb_app.c \
	$(USB_CDC_SHARED)/usb_descriptors.c

BOOT_SOURCES := \
	$(BOOT_DIR)/main.c \
	$(BOOT_DIR)/pic18_flash.c \
	$(BOOT_DIR)/teckio_protocol.c \
	$(USB_SOURCES)

APP_SOURCES := \
	$(APP_DIR)/main.c \
	$(APP_DIR)/teckio_boot_api.c \
	$(USB_SOURCES)

.PHONY: all bootloader application factory test clean

all: factory

$(BUILD_DIR) $(DIST_DIR):
	mkdir -p $@

bootloader: $(DIST_DIR)/TECKIO_bootloader.hex

$(DIST_DIR)/TECKIO_bootloader.hex: $(BOOT_SOURCES) | $(BUILD_DIR) $(DIST_DIR)
	$(XC8) $(COMMON_FLAGS) -I$(BOOT_DIR) -mreserve=rom@2000:7fff \
		-Wl,-Map=$(BUILD_DIR)/TECKIO_bootloader.map \
		-o $@ $(BOOT_SOURCES)

application: $(DIST_DIR)/Aplicacion_TECKIO.hex

$(DIST_DIR)/Aplicacion_TECKIO.hex: $(APP_SOURCES) | $(BUILD_DIR) $(DIST_DIR)
	$(XC8) $(COMMON_FLAGS) -I$(APP_DIR) -mcodeoffset=0x2000 \
		-mreserve=rom@7fc0:7fff \
		-Wl,-Map=$(BUILD_DIR)/Aplicacion_TECKIO.map \
		-o $@ $(APP_SOURCES)

factory: $(DIST_DIR)/TECKIO_factory.hex

$(DIST_DIR)/TECKIO_factory.hex: $(DIST_DIR)/TECKIO_bootloader.hex \
		$(DIST_DIR)/Aplicacion_TECKIO.hex tools/make_factory_hex.py
	$(PYTHON) tools/make_factory_hex.py \
		--bootloader $(DIST_DIR)/TECKIO_bootloader.hex \
		--application $(DIST_DIR)/Aplicacion_TECKIO.hex \
		--output $@

test: | $(BUILD_DIR)
	$(PYTHON) -m unittest discover -s tools/tests -v
	$(CC) -std=c99 -Wall -Wextra -Werror \
		-I$(BOOT_DIR) $(BOOT_DIR)/teckio_protocol.c \
		$(BOOT_DIR)/tests/protocol_host_test.c \
		-o $(BUILD_DIR)/protocol_host_test
	$(BUILD_DIR)/protocol_host_test

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(DIST_DIR)/*.hex
