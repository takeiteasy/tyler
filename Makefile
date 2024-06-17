ifeq ($(OS),Windows_NT)
	PROG_EXT=.exe
	LIB_EXT=dll
	CFLAGS=-O2 -DSOKOL_D3D11 -lkernel32 -luser32 -lshell32 -ldxgi -ld3d11 -lole32 -lgdi32
	EXTRA_FILES=deps/osdialog/osdialog_win.c
	ARCH=win32
else
	UNAME:=$(shell uname -s)
	PROG_EXT=
	ifeq ($(UNAME),Darwin)
		LIB_EXT=dylib
		CFLAGS=-x objective-c -DSOKOL_METAL -fobjc-arc -framework Metal -framework Cocoa -framework MetalKit -framework Quartz -framework AudioToolbox
		EXTRA_FILES=deps/osdialog/osdialog_mac.m
		ARCH:=$(shell uname -m)
		ifeq ($(ARCH),arm64)
			ARCH=osx_arm64
		else
			ARCH=osx
		endif
	else ifeq ($(UNAME),Linux)
		LIB_EXT=so
		CFLAGS=-DSOKOL_GLCORE33 -pthread -lGL -ldl -lm -lX11 -lasound -lXi -lXcursor
		EXTRA_FILES=deps/osdialog/osdialog_gtk3.c
		ARCH=linux
	else
		$(error OS not supported by this Makefile)
	endif
endif

cimgui:
	$(CXX) -std=c++11 -shared -fpic deps/cimgui.cpp deps/imgui/*.cpp -Ideps -Ideps/imgui -o build/libcimgui_$(ARCH).$(LIB_EXT)

tyler: cimgui
	$(CC) $(CFLAGS) $(EXTRA_FILES) src/*.c deps/osdialog/osdialog.c -Isrc -Iaux -Ideps -Ideps/imgui -Lbuild -lcimgui_$(ARCH) -o build/tyler_$(ARCH)$(PROG_EXT)

example:
	$(CC) $(CFLAGS) aux/example.c -Iaux -Ideps -Ideps/imgui -o build/example_$(ARCH)$(PROG_EXT)

all: tyler example

default: tyler

.PHONY: cimgui tyler example
