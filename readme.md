# MidnightSign

MidnightSign is a suite of tools used to cryptographically sign binaries for [MidnightOS](https://github.com/MidnightMagenta/MidnightOS).

Information about the usage of the tools may be obtained by running [tool-name] --help

Available tools are:

- md-keygen - used to generate the keys
- md-keytoarr - used to generate a C style array from binary data
- md-sign - used to sign binaries
- md-verify - used to verify signatures of binaries

## How to build it

Install [CMake](https://cmake.org/) if not already installed

From the root directory of the project run the following commands

```
mkdir build
cd build
cmake ..
make
```

Optionally you may run `sudo make install` to install the built binaries into /usr/local/bin