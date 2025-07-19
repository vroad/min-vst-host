# MinVSTHost

This repository contains a minimal host for VST3 plug-ins. The Steinberg
VST3 SDK is included as a Git submodule.

The host currently only supports Linux.

## Getting Started

### Initialize submodules

Clone the repository and initialize the VST3 SDK submodule:

```bash
git submodule update --init --recursive
```

### Build

Use CMake to configure and build the project. The example below uses the
"Unix Makefiles" generator, but other generators may also be used:

```bash
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

The resulting executable `min-vst-host` will be placed in
`build/bin/RelWithDebInfo` (replace `RelWithDebInfo` with your build
configuration).

### Run

```bash
build/bin/RelWithDebInfo/min-vst-host
```
