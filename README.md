# RISCV-VP

A RISC-V VP using VP-VIBES peripherals.

This VP is based in MINRES TGC series cores and uses CoreDSL to generate the concrete ISS 
of a particular ISA + extensions. The generator approach makes it very flexible and adaptable.
Since the CoreDSL description is used to generate RTL as well as verification artifacts it 
provides a comprehensive and consistent solution to develop processor cores.

## Ultra Quick start

Using gitpod you can run the VP in the cloud. Just visit [Gitpod.io](https://www.gitpod.io/#https://github.com/Minres/RISCV-VP/tree/develop)
and follow the instructions. After the build finished you can run

```

build/src/tgc-vp -f fw/hello-world/prebuilt/hello.elf

```

or use ctest:

```

cd build
ctest

```


You will see on console the prints of the hello world firmware at fw/hello-world/hello.c

[![Open in Gitpod](https://gitpod.io/button/open-in-gitpod.svg)](https://gitpod.io/#https://github.com/Minres/TGC-VP)

## Build instructions for Linux

You need to have a C++17 capable compiler, make or Ninja, Python 3, and CMake installed.

To install conan.io version 2.0 and above (see also http://docs.conan.io/en/latest/installation.html) execute the following:
  
```
python3 -m venv .venv
. .venv/bin/activate
pip3 install conan
conan profile new default --detect
```

Building the VP is as simple as:

```
cmake -S . -B build/Release --preset Release && cmake --build build/Release -j24
```

Building a debug version is analogous:

```
cmake -S . -B build/Debug --preset Debug && cmake --build build/Debug -j24
```

To build some firmware you need to install a RISC-V toolchain like https://github.com/riscv/riscv-tools.
