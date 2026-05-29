# Dot Blaster Go

A NES game built with C using the cc65 compiler toolchain.

## Dependencies

### cc65

You need the [cc65](https://cc65.github.io/) compiler suite to build this project. The `cc65/` directory is not tracked in this repo.

Clone it as a sibling directory to this project:

```
git clone https://github.com/cc65/cc65.git
cd cc65
make
```

The Makefile expects cc65 to be at `../cc65` relative to this project, so your folder layout should look like:

```
games/
  cc65/          <-- cc65 toolchain built here
  Dot_Blaster_Go/
```

## Building

```
make
```

This produces `game.nes`.

## Cleaning

```
make clean
```
