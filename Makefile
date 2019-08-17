################################################################################
## ESP32
ifndef IDF_PATH
	$(error If building for ESP32, you must supply the IDF_PATH variable.)
endif

BUILD_ROOT           := $(shell pwd)

MANUVR_OPTIONS += -D__MANUVR_ESP32

export MANUVR_PLATFORM  = ESP32
export OUTPUT_PATH      = $(BUILD_ROOT)/build

CXXFLAGS += $(MANUVR_OPTIONS)

PROJECT_NAME         := woose-tracker
BUILD_DIR_BASE       := $(OUTPUT_PATH)
EXTRA_COMPONENT_DIRS := $(BUILD_ROOT)/lib/ManuvrOS/ManuvrOS
COMPONENT_EXTRA_INCLUDES := $(PROJECT_PATH)/lib


# Pull in the esp-idf...
include $(IDF_PATH)/make/project.mk
