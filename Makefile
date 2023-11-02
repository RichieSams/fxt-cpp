ifeq ($(OS),Windows_NT)
export SHELL=cmd
EXT=.exe
endif

CMAKE_EXTRA_ARGS?=

ifeq ($(OS),Windows_NT)
CMAKE_PRESET?=Win_x64_Debug
else
CMAKE_PRESET?=Unix_x64_Debug
endif

ifeq ($(patsubst %Debug,,$(CMAKE_PRESET)),)
TEST_DIR=build/Debug
else
TEST_DIR=build/RelWithDebInfo
endif


all: build

.PHONY: build


#################################
# Building
#################################

generate:
	cmake --version
	cmake --preset=$(CMAKE_PRESET) .

build: generate
	cmake --build --preset=$(CMAKE_PRESET) -j


#################################
# Testing
#################################


generate:
	cmake --preset $(CMAKE_PRESET) ./

build: generate
	cmake --build --preset $(CMAKE_PRESET) -j

test: build
	./$(TEST_DIR)/tests/src/fxt-test$(EXT)



#################################
# Miscellaneous
#################################

clean:
ifeq ($(OS),Windows_NT)
	if exist build del /s /q build > nul
	if exist build rmdir /s /q build > nul
else
	rm -rf build
endif
