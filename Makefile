
SHELL := /bin/sh
.DELETE_ON_ERROR:

NT_API_PATH ?= $(firstword $(wildcard ../distingNT_API ../../distingNT_API ../distingNT_API-v119) ../distingNT_API)
INCLUDE_PATH := $(NT_API_PATH)/include
API_HEADER := $(INCLUDE_PATH)/distingnt/api.h

HOST_CXX ?= g++
ARM_CXX ?= arm-none-eabi-c++
ARM_NM ?= arm-none-eabi-nm
ARM_READELF ?= arm-none-eabi-readelf
ARM_SIZE ?= arm-none-eabi-size

BUILD_DIR := build
RELEASE_DIR := release
SOURCE := plugins/Witchboard/Witchboard.cpp
OUTPUT := plugins/WitchboardX.o
HOST_TEST := $(BUILD_DIR)/WitchboardCleanTest

HOST_FLAGS := -std=c++11 -O2 -Wall -Wextra -fno-exceptions -fno-rtti
ARM_ARCH := -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb
ARM_FLAGS := -std=c++11 $(ARM_ARCH) -Os -fPIC -fno-rtti -fno-exceptions -Wall

.PHONY: all check-api test test-gain test-ducker test-channels hardware inspect verify package clean

all: hardware

check-api:
	@test -f "$(API_HEADER)" || { \
		echo "Missing $(API_HEADER). Set NT_API_PATH to the official distingNT_API checkout." >&2; \
		exit 1; \
	}

$(BUILD_DIR):
	mkdir -p "$@"

$(HOST_TEST): tests/WitchboardCleanTest.cpp tests/NtJsonTestHost.h $(SOURCE) $(API_HEADER) | check-api $(BUILD_DIR)
	$(HOST_CXX) $(HOST_FLAGS) -I"$(INCLUDE_PATH)" "$<" -o "$@"

GAIN_TESTS := $(foreach rate,32000 44100 48000 96000,$(BUILD_DIR)/WitchboardGainTest-$(rate))

$(BUILD_DIR)/WitchboardGainTest-%: tests/WitchboardGainTest.cpp tests/WitchboardCleanTest.cpp tests/NtJsonTestHost.h $(SOURCE) $(API_HEADER) | check-api $(BUILD_DIR)
	$(HOST_CXX) $(HOST_FLAGS) -DWITCHBOARD_TEST_SAMPLE_RATE=$* -I"$(INCLUDE_PATH)" "$<" -o "$@"

test-gain: $(GAIN_TESTS)
	@set -e; for test in $(GAIN_TESTS); do "$$test"; done

DUCKER_TESTS := $(foreach rate,32000 44100 48000 96000,$(BUILD_DIR)/WitchboardDuckerTest-$(rate))

$(BUILD_DIR)/WitchboardDuckerTest-%: tests/WitchboardDuckerTest.cpp tests/WitchboardCleanTest.cpp tests/NtJsonTestHost.h $(SOURCE) $(API_HEADER) | check-api $(BUILD_DIR)
	$(HOST_CXX) $(HOST_FLAGS) -DWITCHBOARD_TEST_SAMPLE_RATE=$* -I"$(INCLUDE_PATH)" "$<" -o "$@"

test-ducker: $(DUCKER_TESTS)
	@set -e; for test in $(DUCKER_TESTS); do "$$test"; done

CHANNEL_TESTS := $(foreach rate,32000 44100 48000 96000,$(BUILD_DIR)/WitchboardChannelsTest-$(rate))

$(BUILD_DIR)/WitchboardChannelsTest-%: tests/WitchboardChannelsTest.cpp tests/WitchboardCleanTest.cpp tests/NtJsonTestHost.h $(SOURCE) $(API_HEADER) | check-api $(BUILD_DIR)
	$(HOST_CXX) $(HOST_FLAGS) -DWITCHBOARD_TEST_SAMPLE_RATE=$* -I"$(INCLUDE_PATH)" "$<" -o "$@"

test-channels: $(CHANNEL_TESTS)
	@set -e; for test in $(CHANNEL_TESTS); do "$$test"; done

test: $(HOST_TEST) test-gain test-ducker test-channels
	"$(HOST_TEST)"
	python3 tests/test_preset_migration.py

hardware: $(OUTPUT)

$(OUTPUT): $(SOURCE) $(API_HEADER) | check-api
	mkdir -p "$(@D)"
	$(ARM_CXX) $(ARM_FLAGS) -I"$(INCLUDE_PATH)" -c "$<" -o "$@"

inspect: hardware
	ARM_NM="$(ARM_NM)" ARM_READELF="$(ARM_READELF)" ARM_SIZE="$(ARM_SIZE)" \
		bash scripts/inspect_object.sh "$(OUTPUT)"

verify: test inspect

package: verify
	OBJECT="$(OUTPUT)" RELEASE_DIR="$(RELEASE_DIR)" bash scripts/package_release.sh

clean:
	-rm -rf -- "$(BUILD_DIR)" "$(RELEASE_DIR)" "$(OUTPUT)"
