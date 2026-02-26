OS := $(shell uname -s)
BUILD_TYPE ?= Release
BUILD_TESTING ?= ON
BUILD_DIR := build

ifeq ($(OS),Darwin)
    NPROC := $(shell sysctl -n hw.ncpu)
    BREW_PREFIX := $(shell brew --prefix)
    CMAKE_FLAGS := -DCMAKE_PREFIX_PATH=$(BREW_PREFIX)
else
    NPROC := $(shell nproc)
    CMAKE_FLAGS := 
endif

CMAKE_FLAGS += -DBUILD_TESTING=$(BUILD_TESTING)

.PHONY: all setup build debug clean

all: build

setup:
	@mkdir -p $(BUILD_DIR)
	cmake -S . -B $(BUILD_DIR) $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CMAKE_ARGS)

build: setup
	cmake --build $(BUILD_DIR) --parallel $(NPROC)

debug:
	$(MAKE) BUILD_TYPE=Debug build

clean:
	rm -rf build