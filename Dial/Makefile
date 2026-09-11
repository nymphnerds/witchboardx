SHELL := /bin/sh
.DELETE_ON_ERROR:

NT_API_PATH ?= $(firstword $(wildcard ../../distingNT_API ../distingNT_API ../../distingNT_API-v119) ../../distingNT_API)
INCLUDE_PATH := $(NT_API_PATH)/include
API_HEADER := $(INCLUDE_PATH)/distingnt/api.h

ARM_CXX ?= arm-none-eabi-c++
ARM_ARCH := -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb
ARM_FLAGS := -std=c++11 $(ARM_ARCH) -Os -fPIC -fno-rtti -fno-exceptions -Wall -Wextra -Werror

SOURCE := loopy_profile_selector.cpp
OUTPUT := LoopyDial.o

.PHONY: all check-api clean

all: $(OUTPUT)

check-api:
	@test -f "$(API_HEADER)" || { \
		echo "Missing $(API_HEADER). Set NT_API_PATH to the official distingNT_API checkout." >&2; \
		exit 1; \
	}

$(OUTPUT): $(SOURCE) $(API_HEADER) | check-api
	$(ARM_CXX) $(ARM_FLAGS) -I"$(INCLUDE_PATH)" -c "$<" -o "$@"

clean:
	-rm -f -- "$(OUTPUT)"
