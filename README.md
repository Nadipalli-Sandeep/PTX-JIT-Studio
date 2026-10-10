# PTX-JIT Studio

A small C++17 compiler prototype for arithmetic expressions, with AST optimization and PTX text generation. The goal is to grow this into a GPU-runtime-backed compiler analysis project relevant to compiler engineering.

## Build
```bash
./scripts/build_and_test.sh
```

## Try it
```bash
./build/ptxjit "x * 1 + (2 + 3)"
./build/ptxjit "(x + 0) * 2" artifacts/example.ptx
```

## Current milestone
- Recursive-descent parser with arithmetic precedence.
- AST-based constant folding and algebraic simplification.
- PTX text emitter for a scalar expression.
- Automated tests for optimization and input validation.

## Not implemented yet
- CUDA Driver API module loading and kernel launch.
- Validation with `ptxas` / `cuobjdump`.
- Tensor/vector indexing and GPU benchmarks.

See [docs/DESIGN.md](docs/DESIGN.md) for the scope and limitations.

## Benchmarks

PTX-JIT Studio provides separate compiler and GPU microbenchmarks.

### Compiler benchmark

Build and run:

    cmake -S . -B build
    cmake --build build -j2
    ./build/ptxjit_benchmark

The benchmark warms up the compiler, then measures 5,000 compilations
across five expressions. It measures parsing, optimization, and PTX string
generation, excluding PTX assembly and GPU execution.

Observed runs varied from approximately 1.39 to 1.95 microseconds per
compilation. These are preliminary measurements from the development
environment, not a universal performance guarantee.

### GPU execution benchmark

Generate a cubin and run the benchmark:

    ./build/ptxjit "x + 5" artifacts/gpu_benchmark.ptx
    ptxas -arch=sm_86 artifacts/gpu_benchmark.ptx -o artifacts/gpu_benchmark.cubin
    ./build/ptxjit_gpu_benchmark artifacts/gpu_benchmark.cubin "x + 5" 2.5

The benchmark uses 100 warm-up launches and 10,000 measured launches.
CUDA events measure elapsed time on the GPU timeline. The output is
compared with the CPU evaluator.

Three earlier runs measured approximately 1.406 to 1.414 microseconds
per launch. A later run measured 1.40768 microseconds per launch.

Limitations: this kernel evaluates one scalar expression for one input.
Its timing is not general GPU arithmetic throughput or end-to-end host
launch latency. Results depend on the GPU, driver, system load, and
measurement method.

### Correctness tests

Run compiler and CPU evaluator tests:

    ctest --test-dir build --output-on-failure

Run CPU-versus-GPU demonstrations:

    ./scripts/demo.sh "x * 2 + 3" 4
    ./scripts/demo.sh "x * 1 + (2 + 3)" 4
