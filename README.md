# walos

WebAssembly Language based Operating System is a toy OS using the [Language-based System](https://en.wikipedia.org/wiki/Language-based_system) approach. Unlike standard OS, walos ignores hardware protection (Ring0, single address space). This idea simplifies the system architecture and improves performance by avoiding syscalls.

Processes are [WASM](https://webassembly.org/) binary converted to safe native assembly at runtime. By avoiding hardware protection toggle, syscalls are simple function calls triggered using extended [WASI](https://wasi.dev/).


## Built With

* Make
* LLVM toolchain - `clang`, `lld`, `wasm-ld`
* QEMU - `x86_64`, `ovmf`
* [WASM3](https://github.com/wasm3/wasm3)
* Love and insomnia

This proof-of-concept version is built in C and assembly. Further versions may be implemented in Rust to enjoy safety and lastest WebAssembly improvements.

## Features

* Wasm interpreter
* Service manager
* VGA graphics
* ELF kernel
* UEFI64 loader

### TODO

* Allocator
* Scheduling
* WASI
* Multi-core

### MAYBE

* Zero-copy srv_send
* GRUB Multiboot2

## Getting started

### Prerequisites

OS | Needed | Optional | Export
-- | -- | -- | --
Debian / Ubuntu | `make qemu-system ovmf clang lld` | `mtools xorriso`
Arch / Manjaro | `make qemu edk2-ovmf clang lld` | `mtools libisoburn` | `OVMF=/usr/share/ovmf/x64/OVMF.fd`

### Setup

1. Clone
```sh
git clone https://github.com/CalmSystem/walos.git --recursive
cd walos
```
2. Compile
```sh
make root
```
3. Start with QEMU
```sh
make qemu
```
* Create ISO *with optional prerequisites*
```sh
make iso
```

## Project structure

* bin/ - Assets to copy in system root
* build/ - Temporary build files
* include/ - Shared structures declarations
* kernel/ - OS core
    * libc/ - Minimal C library
    * wax/ - WebAssebly eXecution using WASM3
* loader/ - Boot loaders
* srv/ - Wasm services aka `driverspace`

### Outputs

* root/ - Filesystem root
* walos.img - Usb device image
* walos.iso - Cdrom image

## Similar projects
* [Nebulet](https://github.com/nebulet/nebulet) - A microkernel that implements a WebAssembly "usermode" that runs in Ring 0
* [Kwast](https://github.com/kwast-os/kwast) - Rust operating system running WebAssembly as userspace in ring 0
* [Etheryal](https://github.com/etheryal/etheryal-kernel) - Open Source Rust kernel; Runs WASM and WASI as lightweight containers.

## License

Distributed under the GPLv3 License. See [LICENSE](LICENSE) for more information.