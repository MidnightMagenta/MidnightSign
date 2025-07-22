# MidnightSign

MidnightSign is a suite of tools used to cryptographically sign binaries for [MidnightOS](https://github.com/MidnightMagenta/MidnightOS).

Available tools are:

- `md-keygen` - Generate public/private key pairs for signing
- `md-keytoarr` - Convert binary files (e.g. keys) into C-style arrays
- `md-sign` - Cryptographically sign a binary using a private key
- `md-verify` - Verify a signed binary using a public key

## Usage

Information about the usage of the tools may be obtained by running [tool-name] --help

## How to build it

Install [CMake](https://cmake.org/) if not already installed

Clone the repository 

```
git clone https://github.com/MidnightMagenta/MidnightSign.git
```

From the root directory of the project run the following commands

```
mkdir build
cd build
cmake ..
make
```

Optionally you may run `sudo make install` to install the built binaries system-wide (into `/usr/local/bin`)