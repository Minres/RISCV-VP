# Project guidance

## Overview and layout

RISCV-VP is a C++17 RISC-V virtual platform built on SystemC, DBT-RISE instruction set simulators, and VP-VIBES peripherals.

- `src/`: platform executable, command-line parsing, SystemC testbench, and system wiring. The executable target is `riscv-vp`.
- `dbt-rise-riscv/`: RISC-V architectures, execution backends, memory components, and SystemC core integration.
  - `src/iss/arch/`: architectural state and privileged hart implementations.
  - `src/iss/mem/`: memory interfaces, hierarchy, caches, protection, and interrupt controllers.
  - `src/sysc/`: core adapters, core complexes, and core registration.
  - `gen_input/`: ISA descriptions and generation templates.
  - `tests/sysc/`: GoogleTest tests for SystemC core integration.
- `dbt-rise-core/`: common ISS interfaces and execution infrastructure.
- `scc/`: SystemC Components and supporting utilities.
- `vpvper/`: peripheral models.
- `fw/`: firmware examples, prebuilt smoke-test firmware, and the BSP submodule.
- `build/`: local build trees and generated dependency files.

## Repository boundaries

`dbt-rise-riscv`, `dbt-rise-core`, `scc`, `vpvper`, and `fw/bsp` are Git submodules. Inspect changes in the repository that owns each file:

```bash
git status --short
git -C dbt-rise-riscv status --short
git -C dbt-rise-riscv diff
git -C dbt-rise-riscv log --follow -- src/iss/mem/clic.h
```

Preserve existing user edits in both the parent repository and submodules. Do not reset, switch submodule revisions, update dependencies, or stage unrelated changes as part of a routine fix. If commits are requested, keep submodule source commits and parent submodule-pointer updates explicit.

## Build workflow

Use the root `CMakePresets.json`. The presets select Ninja, C++17, Conan dependency integration, and separate build directories. Requirements are recorded in `requirements.txt`; the preset workflow requires CMake 3.24 or newer, with the current requirements constraining CMake below 4.0, plus Conan 2, Python 3, and a C++17 compiler.

For a fresh checkout, initialize the recorded submodule revisions:

```bash
git submodule update --init --recursive
```

Configure only when needed; reuse an existing configured build tree for incremental changes:

```bash
cmake --preset Debug
cmake --build build/Debug -j 2

cmake --preset RelWithDebInfo
cmake --build build/RelWithDebInfo -j 2

cmake --preset Release
cmake --build build/Release -j 2
```

Adjust parallelism to available memory, especially for template-heavy core compilation. Preserve the user's compiler, Conan profiles, backend options, and SystemC configuration. Treat `CMakeUserPresets.json`, Conan caches, and generated files under `build/` as environment-specific; change source configuration rather than generated Ninja files or caches.

## Validation

For a build failure, reproduce it in the requested configuration and read the first compiler error together with its template instantiation chain. Distinguish source errors from dependency or configuration failures. Do not assume an error is specific to Debug just because it appeared there.

After a code change, build the affected targets. Shared SystemC headers can affect both `dbt-rise-riscv_sc_sig` and `dbt-rise-riscv_sc_tlm`, as well as multiple RV32 and RV64 cores; validate the variants affected by the change.

The CI smoke test runs the prebuilt hello-world firmware:

```bash
build/Debug/src/riscv-vp -f fw/hello-world/prebuilt/hello.elf
```

Use the matching build directory for other configurations. A RISC-V cross compiler is needed to rebuild firmware, but not to run the prebuilt ELF.

Optional SystemC tests are enabled with `DBT_RISE_RISCV_BUILD_SC_TESTS=ON` and require GoogleTest to be available:

```bash
cmake --preset Debug -DDBT_RISE_RISCV_BUILD_SC_TESTS=ON
cmake --build build/Debug --target sysc_tests -j 2
ctest --test-dir build/Debug/dbt-rise-riscv -N
ctest --test-dir build/Debug/dbt-rise-riscv --output-on-failure
```

Inspect test discovery before claiming that tests ran. Add focused behavioral tests for changes to architectural state, CSR handling, interrupts, or memory behavior when suitable coverage exists. Report checks performed and any checks blocked by the environment.

## Editing conventions

- Follow the nearest applicable `.clang-format` and surrounding code. The root style uses four-space indentation, attached braces, and a 140-column limit. Keep formatting changes local.
- Preserve license headers and existing naming conventions.
- Before editing generated architecture or backend code, inspect `gen_input/` and the generation workflow. Prefer changes to the source descriptions or templates when regeneration would otherwise overwrite the fix.
- Keep register word types distinct from core/platform classes in templates; check RV32 and RV64 instantiations where applicable.
- Respect ownership in memory hierarchies. Containers of `std::unique_ptr` cannot be copied; inspect existing hierarchies through references when ownership should remain in place.
- Keep signal-based and TLM-based interrupt integration consistent. Verify initialization order when a component is inserted into the memory hierarchy after core construction.
- For an explanation request, diagnose and show the relevant correction without changing source files. For an implementation request, make the focused change and validate it.
- In the final response, state what changed or caused the failure, link the relevant files, and summarize validation and remaining limitations.
