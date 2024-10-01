## GPUFANCTL

This is a daemon utility that automatically controls your GPU's fan speed based on your own custom temperature curve.

I originally built this utility because I found that the default fan profile of my RTX 3080Ti FE wasn't providing
adequate enough cooling, and I was seeing temperatures of nearly 80C when under high load. By using `gpufanctl`,
I'm now seeing temperatures of ~68C under similar load.

**NOTE: While I've made every effort to prevent bugs and misuse, incorrect use of this utility could cause
permanent damage to your GPU. Use of this utility is purely at your own risk. Before using this utility,
you should consider whether the default fan profile provides adequate enough cooling for your usage. If
you're happy with the default temperature control then you should consider not using `gpufanctl`**

### Current Support Status

- Supports both X11 and Wayland
- **Only NVIDIA GPUs are currently supported**
- I've only confirmed NVIDIA driver support for version `550.120`. Older versions _may_ still work but I haven't confirmed this.

### Usage

*NOTE: In temperature control mode (the default mode), `gpufanctl` must be run as root*

Example usage from a TTY is as follows...

```
$ sudo gpufanctl '40:30,60:50,80:100'
```

This will start the utility running and will periodically read the GPU temperature at 5 second intervals.
The provided argument will define a custom fan curve with two slopes...

1. From `40C` to `60C` the fan speed will increase linearly from `30%` to `50%`
2. From `60C` to `80C` the fan speed will increase linearly from `50%` to `100%`

In this example, any temperature below `40C` will set the fan speed to the GPU's default profile, and
for any temperature above `80C`, the fan speed will remain at `100%`.

The utility will exit when receiving a `SIGINT` or `SIGTERM` signal. In a TTY, this means `Ctrl+C` will stop the utility.

When `gpufanctl` exits **it will reset the GPU to its default fan profile**.

**Running `gpufanctl` without any arguments is supported, and will just use your GPU's default fan profile**

A full list of options can be obtained using `$ gpufanctl --help`

### Building

The build system uses *CMake*. You can build from source using the following steps...

From the project's root folder:

1. `$ mkdir ./build`
2. `$ cd ./build`
3. `$ cmake ..`
4. `$ cmake --build . -- -j$(nproc)`

### Installing

From the project's root folder, follow the steps above to build the source, then additionally...

- `$ sudo make install`

This will install `gpufanctl` to `/usr/local/bin` by default. This can be overridden by replacing
step `3.` with...

- `$ cmake -DCMAKE_INSTALL_PREFIX=<YOUR_INSTALLATION_FOLDER> ..`

... replacing `<YOUR_INSTALLATION_FOLDER>` with the desired installation path. `gpufanctl` will then
be installed to `<YOUR_INSTALLATION_FOLDER>/bin/gpufanctl`

### Notes for Packagers

If you wish to package `gpufanctl` for a distribution, you should be aware that the default build
steps will automatically pull in the `exios` dependency via `git` (The method used is actually *CMake*'s
`FetchContent` module). If your packaging system / CI doesn't allow pulling in remote dependencies via this
method, then you can follow these steps...

- Examine the `exios` version required in the `./cmake/Dependencies.cmake` file. It will be the value set
  for the `GPUFANCTL_EXIOS_VERSION` variable.
- Have your build system pull in this dependency in addition to the main project's release tarball, using `https://github.com/gmbeard/exios/releases/download/${version}/exios-source-${version}.tar.xz`, replacing `${version}` with whatever `GPUFANCTL_EXIOS_VERSION` is set to, and place it into the `deps/exios` subfolder within the project directory, 
- Configure the CMake project using `$ cmake -DGPUFANCTL_USE_LOCAL_DEPENDENCIES=ON ..`.
- Build the source as normal.
