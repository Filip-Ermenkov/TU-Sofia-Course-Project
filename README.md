# Courier Shipment Management System

A C++20 console application that automates the logistics of a courier company:
accepting shipments, calculating shipping costs, tracking status, and keeping
a history of logistics operations.

## Requirements

- CMake 3.25 or newer
- A C++20 compiler (developed with MSVC from Visual Studio 2022; also built with GCC and Clang)

## Build, test and run

The build settings are shared in `CMakePresets.json`. VS Code's CMake Tools extension
lists the presets; from a terminal:

```sh
cmake --preset msvc            # configure (Windows, Visual Studio 2022)
cmake --build --preset msvc    # build
ctest --preset msvc            # run the tests
./build/msvc/Debug/courier     # run the application
```

On Linux use the `gcc` or `clang` preset instead (the program is then `./build/gcc/courier`).
The `gcc-asan` preset builds with AddressSanitizer and UndefinedBehaviorSanitizer, which
stop the program at the first memory error or undefined behaviour.

## Project layout

| Folder             | Contents                                                   |
|--------------------|------------------------------------------------------------|
| `include/courier/` | Public headers of the `courier_core` library               |
| `src/`             | Library sources and the console entry point (`main.cpp`)   |
| `tests/`           | Catch2 unit tests, one file per class                      |
| `docs/`            | Assignment text                                            |
