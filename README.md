# Texture

A small tool for generating and compressing mipmaps from a single PNG picture and packing them into one file for use with E01.

## Contents

- [Requirements](#requirements)
- [Configuration](#configuration)
- [Building](#building)
- [Usage](#usage)
- [Development](#development)

[Back to top](#texture)

## Requirements

For building, a C compiler and compatible C library of your choice can be used. This project was developed using GCC and GLIBC.

Additional libraries required are SDL2, SDL2 Image, OpenGL 4.6 Core. Etcpak is also required as a shared library, but this will be provided in the [configuration](#configuration) process.

To run or debug this tool, a valid PNG, JPG or BMP texture will be required too. More details are specified in the [Usage](#usage) section below.

[Back to top](#texture)

## Configuration

This project can be configured for building using the `./configure` script. This script allows you to specify which platform you'd like to build for.

Currently, only `linux` is supported with plans to add support for Windows later.

[Back to top](#texture)

## Building

After [configuring](#configuration), the project can be built using the `make` command. This will build the executable in debug mode and attempt to convert and pack a texture from the specified texture file (see [Usage](#usage) below).

For more `make` options, run `make help`.

[Back to top](#texture)

## Usage

For executable usage, you can run `./texture --help`.

This project needs at least one texture file either placed in this directory or in this directory's parent directory. After preparing this file, update `common.mk` to refer to it.

[Back to top](#texture)

## Development

To debug this project, GDB will be required. It can be launched using `make debug`. Support for debugging via VS Code's C/C++ extension is also available.

[Back to top](#texture)
