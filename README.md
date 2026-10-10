# PTX-JIT Studio

A C++17 arithmetic-expression compiler prototype that parses expressions, optimizes an abstract syntax tree (AST), generates NVIDIA PTX, and executes generated GPU code through the CUDA Driver API.

## Features

- Recursive-descent parsing with arithmetic precedence, parentheses, and unary signs.
- AST constant folding and algebraic simplification.
- PTX text generation for scalar arithmetic expressions using variable `x`.
- CPU reference evaluator and CUDA Driver API execution.
- GPU-versus-CPU numerical comparison.
- Compiler, optimizer-equivalence, and invalid-input tests.
- Compiler and GPU microbenchmarks.
- CPU-only CMake build and GitHub Actions CI.
- Automated PTX assembly validation.

## Requirements

- C++17 compiler and CMake.
- NVIDIA GPU, compatible NVIDIA driver, CUDA Driver API development files, and `ptxas` for GPU execution.
- The CPU-only build does not require CUDA.

## Build and test

CUDA-enabled build:

```bash
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

CPU-only build:

```bash
cmake -S . -B build-ci -DPTXJIT_ENABLE_CUDA=OFF
cmake --build build-ci -j2
ctest --test-dir build-ci --output-on-failure
```

## Compile an expression

```bash
./build/ptxjit "x * 1 + (2 + 3)"
./build/ptxjit "(x + 0) * 2" artifacts/example.ptx
```

The compiler reports the original expression, optimized expression, AST node counts, and generated PTX path.

## GPU demo

```bash
./scripts/demo.sh "x * 2 + 3" 4
./scripts/demo.sh "x * 1 + (2 + 3)" 4
```

The demo generates PTX, assembles it for `sm_86`, executes the cubin on the GPU, and compares the result with the CPU evaluator.

## Validate generated PTX

```bash
./scripts/validate_ptx.sh
```

This assembles six representative expressions with `ptxas -arch=sm_86`. It requires `ptxas` to be available on `PATH`.

## Benchmarks

Compiler benchmark:

```bash
./build/ptxjit_benchmark
```

This measures parsing, optimization, and PTX string generation across 5,000 compilations after warm-up.

GPU benchmark:

```bash
./build/ptxjit "x + 5" build/gpu_benchmark.ptx
ptxas -arch=sm_86 build/gpu_benchmark.ptx -o build/gpu_benchmark.cubin
./build/ptxjit_gpu_benchmark build/gpu_benchmark.cubin "x + 5" 2.5
```

### Observed prototype results

Results from the development environment (NVIDIA GeForce RTX 3050, `sm_86`):

| Measurement | Observed result |
|---|---:|
| Compiler benchmark average | 1.460 us per compilation |
| GPU benchmark average device time | 1.4036 us per launch |
| GPU demo: `x * 2 + 3`, input `4` | 11 |
| CPU reference for the same input | 11 |
| Absolute error in the demo | 0 |

These are preliminary measurements from a specific environment, not performance guarantees. GPU timing uses CUDA events around repeated launches. Device execution time is not equivalent to end-to-end host launch latency, and a tiny scalar kernel does not represent general GPU throughput.

## Testing and continuous integration

The test suite covers compiler behavior, optimizer equivalence across representative expressions and input values, and invalid inputs. GitHub Actions runs the CPU-only build and tests on pushes and pull requests to `main`.

## Design

See [docs/DESIGN.md](docs/DESIGN.md) for additional design details.

## Limitations

- Supports scalar arithmetic expressions with one variable, `x`.
- Does not implement general tensor operations, vector indexing, or a complete compiler intermediate representation.
- Generated PTX and supported expressions are intentionally limited to this prototype.
- Benchmarks are microbenchmarks and should not be interpreted as production performance claims.

## Future work

- Expand parser and optimizer regression coverage.
- Add broader expression and numerical edge-case testing.
- Improve PTX validation and compiler diagnostics.
- Explore richer expression types and more advanced compiler optimizations.

## Local Streamlit Frontend

PTX-JIT Studio includes a local browser interface for compiling supported
scalar expressions, inspecting generated PTX, and optionally assembling
and executing the result on a compatible NVIDIA GPU.

### Requirements

- Python 3 and `venv`
- CMake and a C++17 compiler
- NVIDIA driver and CUDA tools (`ptxas`) for GPU execution

### Run locally

Build the C++ tools:

```bash
cmake -S . -B build
cmake --build build -j2
```

Set up the frontend:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements-frontend.txt
streamlit run app.py
```

Open the local URL printed by Streamlit, usually `http://localhost:8501`.

The interface supports the compiler's scalar expression syntax using `x`,
floating-point constants, arithmetic operators, unary signs, and parentheses.
GPU execution requires the CUDA tools and a compatible NVIDIA GPU. Compilation
can still be explored without GPU execution by disabling the GPU option.
