CORE := minipomo save text
CORE_SRC := $(CORE:%=src/core/%.c)
CORE_OBJ := $(CORE:%=build/core/%.o)
CORE_LIB := build/libminipomo.a
CLI_SRC := $(wildcard src/cli/*.c)
WEB_SRC := $(wildcard src/web/*.c)
APP_SRC := $(wildcard src/app/*.c)
SITE := build/web
HEADERS := $(wildcard src/*/*.h)

CLAY_COMMIT := 139802baaa144c3a674db72efc25685f68fe391f
CLAY_URL := https://raw.githubusercontent.com/nicbarker/clay/$(CLAY_COMMIT)/clay.h
CLAY := 3rd/clay/clay.h

ifeq ($(shell uname),Darwin)
WASM_CC ?= $(shell brew --prefix)/opt/llvm/bin/clang
else
WASM_CC ?= clang
endif
CFLAGS := -std=c11 -Wall -Wextra -Isrc -O2
WASM_FLAGS := --target=wasm32 -Os -nostdlib -fno-builtin -mbulk-memory \
	-isystem 3rd/clay -Wl,--no-entry -Wl,--strip-all
PORT ?= 8000
PREFIX ?= /usr/local

all: core cli web
core: $(CORE_LIB)
cli: build/minipomo
web: $(SITE)/app.wasm $(SITE)/index.html $(SITE)/main.js

build/core/%.o: src/core/%.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(CORE_LIB): $(CORE_OBJ)
	$(AR) rcs $@ $^

build/minipomo: $(CLI_SRC) $(APP_SRC) $(CORE_LIB) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(CLI_SRC) $(APP_SRC) $(CORE_LIB)

$(SITE)/app.wasm: $(WEB_SRC) $(APP_SRC) $(CORE_SRC) $(HEADERS) $(CLAY) | setup
	@mkdir -p $(@D)
	$(WASM_CC) $(CFLAGS) $(WASM_FLAGS) -o $@ $(WEB_SRC) $(APP_SRC) $(CORE_SRC)

$(SITE)/%: src/web/%
	@mkdir -p $(@D)
	cp $< $@

$(CLAY):
	@mkdir -p $(@D)
	curl -fsSL -o $@ $(CLAY_URL)

install: cli
	install -d $(PREFIX)/bin
	install -m 755 build/minipomo $(PREFIX)/bin/minipomo

serve: web
	python3 -m http.server $(PORT) -d $(SITE)

setup:
	@if ! { $(WASM_CC) --print-targets 2>/dev/null | grep -q wasm32 && \
		command -v wasm-ld >/dev/null; }; then \
		if [ "$$(uname)" = Darwin ]; then brew install llvm lld; \
		else sudo apt-get update && sudo apt-get install -y clang lld; fi; \
	fi

clean:
	rm -rf build

.PHONY: all core cli web install serve setup clean
