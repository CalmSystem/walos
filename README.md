# walos

WebAssembly Language based Operating System is a toy OS using the [Language-based System](https://en.wikipedia.org/wiki/Language-based_system) approach. Unlike standard OS, walos ignores hardware protection (Ring0, single address space). This idea simplifies the system architecture and improves performance by avoiding syscalls.

Processes are [WASM](https://webassembly.org/) binary converted to safe native assembly at runtime. By avoiding hardware protection toggle, syscalls are simple function calls triggered using extended [WASI](https://wasi.dev/).


### Built With

* Make
* LLVM toolchain: `clang`, `lld`
* QEMU: `x86_64`, `ovmf`
* [WebAssembly Micro Runtime](https://github.com/bytecodealliance/wasm-micro-runtime)
* Love and insomnia

This proof-of-concept version is built in C and assembly. Further versions may be implemented in Rust to enjoy safety and lastest WebAssembly improvements.

## Features

* UEFI x86_64 hello

## Getting started

* Debian/Ubuntu: `sudo apt-get install qemu-system ovmf clang lld`


1. Clone
```sh
git clone https://github.com/CalmSystem/walos.git --recursive
cd walos
```
2. Compile
```sh
make all
```
3. Start with QEMU
```sh
make qemu
```

## Similar projects
* [Nebulet](https://github.com/nebulet/nebulet) - A microkernel that implements a WebAssembly "usermode" that runs in Ring 0
* [Kwast](https://github.com/kwast-os/kwast) - Rust operating system running WebAssembly as userspace in ring 0
* [Etheryal](https://github.com/etheryal/etheryal-kernel) - Open Source Rust kernel; Runs WASM and WASI as lightweight containers.

## License

Distributed under the GPLv3 License. See [LICENSE](LICENSE) for more information.