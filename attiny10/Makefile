.PHONY: all build fuse-read fuse-write flash-write clean

APP_NAME = bootsel2reset

REPO_DIR = $(shell cd .. && pwd)

BUILD_DIR := build

MCU := attiny10
F_CPU := 1000000UL

CXX := avr-g++
OBJCOPY := avr-objcopy
AVR_SIZE := avr-size
AVRDUDE := avrdude

APP_SRC_DIR := src
APP_CPP_LIST := $(wildcard $(APP_SRC_DIR)/*.cpp)
APP_OBJ_LIST := $(patsubst $(APP_SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(APP_CPP_LIST))

BS2RST_INC_DIR := $(REPO_DIR)/common/include
BS2RST_SRC_DIR := $(REPO_DIR)/common/src
BS2RST_BUILD_DIR := $(BUILD_DIR)/bs2rst
BS2RST_HPP_LIST := $(wildcard $(BS2RST_INC_DIR)/bs2rst/*.hpp)
BS2RST_CPP_LIST := $(wildcard $(BS2RST_SRC_DIR)/*.cpp)
BS2RST_OBJ_LIST := $(patsubst $(BS2RST_SRC_DIR)/%.cpp,$(BS2RST_BUILD_DIR)/%.o,$(BS2RST_CPP_LIST))

ELF := $(BUILD_DIR)/$(APP_NAME).elf
HEX := $(BUILD_DIR)/$(APP_NAME).hex

CXXFLAGS := \
	-Os \
	-ffunction-sections \
	-fdata-sections \
	-flto \
	-mmcu=$(MCU) \
	-DF_CPU=$(F_CPU)

LDFLAGS := \
	-Wl,--gc-sections \
	-mmcu=$(MCU) \

EXTRA_DEPENDENCIES := \
	Makefile

all: build

build: $(HEX)

$(HEX): $(ELF) $(EXTRA_DEPENDENCIES)
	@mkdir -p $(BUILD_DIR)
	$(OBJCOPY) -j .text -j .data -j .rodata -O ihex $< $@

$(ELF): $(APP_OBJ_LIST) $(BS2RST_OBJ_LIST) $(EXTRA_DEPENDENCIES)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(APP_OBJ_LIST) $(BS2RST_OBJ_LIST) $(LDFLAGS)
	$(AVR_SIZE) $@

$(BUILD_DIR)/%.o: $(APP_SRC_DIR)/%.cpp $(EXTRA_DEPENDENCIES)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(BS2RST_INC_DIR) -c $< -o $@

${BS2RST_BUILD_DIR}/%.o: $(BS2RST_SRC_DIR)/%.cpp $(BS2RST_HPP_LIST) $(EXTRA_DEPENDENCIES)
	@mkdir -p $(BS2RST_BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(BS2RST_INC_DIR) -c $< -o $@

fuse-read:
	avrdude \
		${AVRDUDE_ISP_OPTS} \
		-p $(MCU) \
		-B 10 \
		-U fuse:r:-:h

fuse-write:
	avrdude \
		${AVRDUDE_ISP_OPTS} \
		-p $(MCU) \
		-B 10 \
		-U fuse:w:0xff:m

flash-write: $(HEX)
	avrdude \
		${AVRDUDE_ISP_OPTS} \
		-B 10 \
		-p $(MCU) \
		-U flash:w:$(HEX)

clean:
	rm -rf $(BUILD_DIR)
