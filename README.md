# MiniPomo

A Pomodoro timer in C. For personal use only.

Two frontends are supported:
1. a cli terminal app
2. a web app (layout by [Clay](https://github.com/nicbarker/clay)) compiled to wasm

Inspired by the look of [pomofocus.io](https://pomofocus.io)

## Quick start

```
make serve
build/minipomo
```

## Build

```
make core
make cli
make web
make
make install
make serve
make setup
make clean
```

## Core interface

One `struct minipomo` holds the task list and each task's timer.
`minipomo_update` applies elapsed time in milliseconds.
`minipomo_modify` updates time before applying a typed change.
Both functions return completion information for application notifications.

Applications read stored fields through const state references.
Three `minipomo_get_*` functions provide timer values, start permission, and totals.
Applications handle input, display, notifications, and storage writes.
The save functions encode and validate complete state without file access.

## Credits

The design, colors, animations, and timing rules come from [pomofocus.io](https://pomofocus.io) by Yuya Uzu.
MiniPomo is an re-implementation for personal use and has no affiliation with pomofocus.io.
