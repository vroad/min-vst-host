# MinVSTHost

This repository contains a minimal host for VST3 plug-ins. The Steinberg
VST3 SDK is not tracked in this repository. Instead, use the helper
script to download the SDK into `external/vst3sdk`.

The host currently only supports Linux.

## Getting Started

### Fetch the VST3 SDK

Run the helper script to download the SDK specified in
`vst3sdk-source.json`:

```bash
./scripts/download_vst3sdk.sh
```

### Build

Use CMake to configure and build the project. The example below uses the
"Unix Makefiles" generator, but other generators may also be used:

```bash
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

To try a locally modified VST3 SDK, pass `-DVST3SDK_PATH=/path/to/sdk` to the
CMake configuration command.

The resulting executable `min-vst-host` will be placed in
`build/bin/RelWithDebInfo` (replace `RelWithDebInfo` with your build
configuration).

### Build with Nix Flakes

If you have Nix with flakes enabled, you can build the host using the
provided `flake.nix`:

```bash
nix build
```

### Run

```bash
build/bin/RelWithDebInfo/min-vst-host
```
