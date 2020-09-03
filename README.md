[![Build Status](https://github.com/tsoding/something/workflows/CI/badge.svg)](https://github.com/tsoding/something/actions)

# something

**WARNING! The game is in an active development state and is not even
alpha yet. Use it at your own risk. Nothing is documented, anything
can be changed at any moment or stop working at all.**

## Quick Start

```console
$ # Dependencies
$ ## Debian
$ sudo apt-get install libsdl2-dev
$ ## Manjaro
$ sudo pacman -S sdl2
$ ## (add your distro here)
$ # Build
$ make -B
$ ./something.debug
```

## WebAssembly build

```console
$ # Dependencies
$ # [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
$ # Build
$ emsdk_env
$ build
$ # Run
$ host on any http-server
```
