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

## Using the program

The program shows a numbered menu; type a number and press Enter.

- **Shipments:** accept a new shipment (the cost is calculated and the waybill printed), find
  one by tracking ID (e.g. `CR00000001`, any letter case), list all shipments by ID or by cost,
  report the undelivered ones, and filter by service, status, destination or cost.
- **Delivery:** dispatch the next waiting shipment (express ones go first) or change a
  shipment's status (Accepted, In transit, Delivered; one step forward at a time). Status
  changes of express and fragile shipments are reported by the dispatch module.
- **Changes:** reroute a shipment, modify its weight, size or declared value, assign it to a
  courier or office or remove the assignment. Each of these can be undone and redone.
- **Couriers and offices:** register them by a short code such as `C-01` and list them with
  the shipments assigned to them.
- **History:** every operation with its local time, and the operations that were undone.

When a prompt shows a value in brackets, such as `[0.00]`, pressing Enter keeps that value.
Where there is none, an empty line cancels the action and returns to the menu. Invalid input
is explained and changes nothing. Ctrl+Z then Enter (Windows) or Ctrl+D (Linux) ends the
program like choosing 0.

Everything is saved after each change to three CSV files in the `data` folder of the current
working directory (`assignees.csv`, `shipments.csv`, `history.csv`) and loaded again at the
next start. If the files are damaged, the program says what is wrong and asks before starting
with no data. The undo and redo lists last only while the program runs.

## Project layout

| Folder             | Contents                                                   |
|--------------------|------------------------------------------------------------|
| `include/courier/` | Public headers of the `courier_core` library               |
| `src/`             | Library sources and the console entry point (`main.cpp`)   |
| `tests/`           | Catch2 unit tests, one file per class                      |
| `docs/`            | Assignment text                                            |
