# Ethlt's REAPER Toolkit

A collection of custom actions for [REAPER](https://www.reaper.fm/) to enhance workflow and provide new capabilities.

## Features

This toolkit includes the following actions:

### Smart Adjustments

- **Smart Volume Adjust**: Intelligently adjusts the volume of selected items, tracks, or envelope points based on current focus and cursor position. If nothing is selected, it controls the system volume. Fine-tuning is available for smaller increments.
- **Smart MIDI Velocity Adjust**: Adjusts the velocity of selected MIDI notes. It offers both coarse and fine adjustments.

### Envelope Management

- **Clean Envelope Points**: Cleans up and removes redundant points from all track and take envelopes in the project, simplifying complex automation.

### MIDI and Grid

- **Append Duplicate**: Duplicates the selected MIDI notes and appends them after the selection.
- **Switch Triplet Grid**: Toggles the grid between straight and triplet timing in both the main arrange view and the MIDI editor.

### Routing

- **Setup Global MIDI Send**: Creates a send from all tracks to a designated track, designed to work with [midi_pump](https://github.com/IcEarthlight/ethlt-jsfx-collection) jsfx for global synchronized pumping effect.

## Installation

To use this toolkit, you need to build the plugin from the source code.

### Prerequisites

- [CMake](https://cmake.org/)
- A C++ compiler (MSVC, Clang or GCC)
- [Git](https://git-scm.com/downloads)
- [REAPER](https://www.reaper.fm/)

### Build Steps

1. **Clone the repository:**
   ```bash
   git clone --recursive https://github.com/IcEarthlight/ethlt-reaper-toolkit.git
   ```
2. **Configure the project with CMake:**
   ```bash
   cd ethlt-reaper-toolkit
   mkdir build
   cd build
   cmake ..
   ```
3. **Build the project:**
   ```bash
   cmake --build . --target Release
   ```
4. **Install the plugin:**
   The build process will create a dynamic library (`.dll`, `.so`, or `.dylib`) in the build directory. Copy this file to your REAPER `UserPlugins` directory manually, or use `cmake --install .` instead. You can find this directory by going to `Options > Show REAPER resource path in explorer/finder...` in REAPER.

## Usage

Once installed, the actions will be available in the REAPER "Actions" menu (`Actions > Show action list...`). You may restart REAPER after installation. You can search for them by name (e.g., "Smart Volume Adjust") and assign them to keyboard shortcuts or toolbar buttons.
