# walos

WebAssembly Language based Operating System is a toy OS using the [Language-based System](https://en.wikipedia.org/wiki/Language-based_system) approach. Unlike generalist OS, walos ignores hardware protection (Ring0, single address space). This idea simplifies the system architecture and improves performance by avoiding context switching. On the downside, the OS is not protected against CPU side-channel attacks.

Processes and drivers *(services)* are [WASM](https://webassembly.org/) binary executed by design in a sandboxed environment. By avoiding hardware protection, syscalls are simple function calls triggered using interfaces like [WASI](https://wasi.dev/). So drivers can be implemented in any language targeting WebAssembly.

## Built With

* Make
* LLVM toolchain - `clang`, `lld`, `wasm-ld`
* QEMU - `x86_64`, `ovmf`
* [WASM3](https://github.com/wasm3/wasm3)
* [WASI SDK](https://github.com/WebAssembly/wasi-sdk)
* Love and insomnia

## Features

* WASM runtime
* Multi ABI kernel
* UEFIx64 loaders

### Planned

* Multitasking
* Dependency tree
* WASM custom section
* Optional service streaming

## Getting started

### Prerequisites

OS | Needed | Optional
--- | --- | --- | ---
Debian / Ubuntu | `make clang lld` | `qemu-system ovmf mtools xorriso`
Arch / Manjaro | `make clang lld` | `qemu edk2-ovmf mtools libisoburn`

### Setup

1. Clone
```sh
git clone https://github.com/CalmSystem/walos.git
cd walos
```
2. Compile system with all drivers
```sh
make all
```
3. Start `ENTRY` as **ELF binary**
```sh
make run ENTRY=sample/shell LOADER=elf
```
* Start `ENTRY` with **QEMU and OVMF**
```sh
make run ENTRY=sample/shell
```
* Create a **bootable ISO** using optional prerequisites
```sh
make package ENTRY=sample/hello.c
```

### Known environment errors

Message | Possible solution
--- | ---
`recipe for target 'target/efi-app/srv/entry.wasm' failed` | `ENTRY` argument missing
`llvm-ar: Command not found` | `export PATH=$PATH:/usr/lib/llvm-10/bin`
`Executable "wasm-ld" doesn't exist!` | 🔼 🔼
`lld: error: unable to find library -lc` | `export LDPATH=/usr/lib64`
`/bin/sh: 1: ./run: not found` | `export DLINK=/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2`
`qemu: could not load PC BIOS` | `export OVMF=/usr/share/ovmf/x64/OVMF.fd`

## Project structure

* include/ - Shared structures declarations
  * kernel/ - OS core library declarations
  * utils/ - Header only utilities
  * w/ - Common services declarations
* kernel/ - OS core library
  * libc/ - Freestanding libC
* loader/ - Boot loaders implementations
  * efi-app/ - In UEFI application
  * efi-os/ - UEFI exit loader
  * elf/ - Linux binary loader
* engine/ - WASM engine implementations
  * wasm3/ - [WASM3](https://github.com/wasm3/wasm3) interpreter
* service/ - Services implementations
* sample/ - Sample entry points
  * entry.c - Makefile for `*.c`
  * hello.c - Print `Hello world`
  * vga/ - Display `wasm.tga` on screen and wait
  * exec/ - Run `embed.c` using system WASM engine
  * shell/ - Interactive shell using [WAPM](https://wapm.io/)

### Outputs

* build/ - Temporary build files
* target/ - Output files

## Similar projects

* [Nebulet](https://github.com/nebulet/nebulet) - A microkernel that implements a WebAssembly "usermode" that runs in Ring 0
* [Kwast](https://github.com/kwast-os/kwast) - Rust operating system running WebAssembly as userspace in ring 0
* [Etheryal](https://github.com/etheryal/etheryal-kernel) - Open Source Rust kernel; Runs WASM and WASI as lightweight containers
* [IncludeOS](https://github.com/includeos/includeos) - C++ Unikernel

## License

Distributed under the GPLv3 License. See [LICENSE](LICENSE) for more information.