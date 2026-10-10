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
