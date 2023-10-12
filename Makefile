CMAKE_PRESET?=Linux_x64_Release

all: build


.PHONY: build


generate:
	cmake --preset $(CMAKE_PRESET) ./

build: generate
	cmake --build --preset $(CMAKE_PRESET) -j
