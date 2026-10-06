# MiniPomo

A Pomodoro timer in C. For personal use only.

Two frontends are supported:
1. a cli terminal app
2. a web app (layout by [Clay](https://github.com/nicbarker/clay)) compiled to wasm

Inspired by the look of [pomofocus.io](https://pomofocus.io)

## Quick start

```
make serve      # build, then serve at http://localhost:8000
build/minipomo  # terminal app
```

## Build

```
make core       # build/libminipomo.a
make cli        # build/minipomo
make web        # build/web/ (the core is compiled again for wasm32)
make            # all three
make install    # copy build/minipomo to $PREFIX/bin (PREFIX=/usr/local)
make serve      # serve build/web/ (PORT=8000)
make setup      # install clang with the wasm32 target and wasm-ld if missing
make clean
```

## Credits

The design, colors, animations, and timing rules come from [pomofocus.io](https://pomofocus.io) by Yuya Uzu.
MiniPomo is an re-implementation for personal use and has no affiliation with pomofocus.io.
