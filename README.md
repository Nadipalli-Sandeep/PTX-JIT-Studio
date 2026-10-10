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

- Recursive-descent parser with arithmetic precedence and unary signs.
- AST-based constant folding and algebraic simplification.
- PTX generation for scalar arithmetic expressions using variable x.
- CUDA Driver API runner for executing generated cubins.
- CPU reference evaluator and CPU/GPU result comparison.
- Compiler and CPU evaluator tests.
- Compiler and GPU microbenchmarks.

## Build and test

    cmake -S . -B build
    cmake --build build -j2
    ctest --test-dir build --output-on-failure

## GPU demo

    ./scripts/demo.sh "x * 2 + 3" 4
    ./scripts/demo.sh "x * 1 + (2 + 3)" 4

The demo generates PTX, assembles a cubin with ptxas for sm_86,
executes the kernel, and compares the result with the CPU evaluator.

## Benchmarks

Run the compiler benchmark:

    ./build/ptxjit_benchmark

It measures parsing, optimization, and PTX string generation across
5,000 compilations after warm-up.

Run the GPU benchmark after generating a cubin:

    ./build/ptxjit "x + 5" artifacts/gpu_benchmark.ptx
    ptxas -arch=sm_86 artifacts/gpu_benchmark.ptx -o artifacts/gpu_benchmark.cubin
    ./build/ptxjit_gpu_benchmark artifacts/gpu_benchmark.cubin "x + 5" 2.5

The GPU benchmark uses CUDA events around repeated kernel launches.
Observed results are preliminary and specific to the test environment.
A single-scalar kernel does not represent general GPU throughput or
end-to-end host launch latency.

## Limitations

- Supports scalar arithmetic expressions with one variable, x.
- Does not implement general tensor/vector indexing or a full compiler IR.
- Benchmarks are microbenchmarks and are not production performance claims.

See docs/DESIGN.md for additional design details.
