# walos

WebAssembly Language based Operating System is a toy OS using the [Language-based System](https://en.wikipedia.org/wiki/Language-based_system) approach.

Processes and drivers *(services)* are [WASM](https://webassembly.org/) binary executed by design in a sandboxed environment. So drivers can be implemented in any language targeting WebAssembly.

Unlike generalist OS, walos ignores hardware protection (Ring0, single address space). This idea simplifies the system architecture and improves performance by avoiding context switching. Syscalls are simple function calls triggered using interfaces like [WASI](https://wasi.dev/). On the downside, the OS is not protected against CPU side-channel attacks.
## Built With

* [LLVM](https://llvm.org): `clang`, `lld`, `wasm-ld` - Make
* [WASI SDK](https://github.com/WebAssembly/wasi-sdk) - [WASM3](https://github.com/wasm3/wasm3)
* [QEMU](https://www.qemu.org) - [OVMF](https://www.tianocore.org)
* [Limine](https://limine-bootloader.org)
* Love and insomnia

## Features

* WASM runtime
* Multi loaders
  * `os` (EFI | BIOS)
  * `efi` (EFI runtime)
  * `elf` (Unix executable)

### Planned

* Multitasking
* Trim services tree
* WASM custom section
* Multi arch x86_64, Risk-V & Arm64

## Getting started

### Prerequisites

OS | Needed | Optional
--- | --- | ---
Debian / Ubuntu | `make clang lld` | `qemu-system ovmf xorriso`
Arch / Manjaro | `make clang lld` | `qemu edk2-ovmf libisoburn`

### Setup

1. Clone
```sh
git clone https://github.com/CalmSystem/walos.git
cd walos
```
2. Start `sample/shell` as **ELF binary**
```sh
make run ENTRY=sample/shell LOADER=elf
```
* Start `sample/exec` with **QEMU and OVMF**
```sh
make run ENTRY=sample/exec
```
* Create a **bootable ISO** of `sample/hello.c`
```sh
make package ENTRY=sample/hello.c
```
* Start `sample/vga` with **QEMU, Limine** and graphics
```sh
make run ENTRY=sample/vga RUN_ISO=1 RUN_VGA=1
```

### Known environment errors

Message | Possible solution
--- | ---
`No rule to make target '.wasm'` | `ENTRY` argument missing
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
  * os/ - Standalone loader
  * efi/ - UEFI application
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