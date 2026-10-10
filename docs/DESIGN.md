# PTX-JIT Studio — Milestone 1

## Current scope
This milestone implements a small expression compiler in C++17. The input grammar supports:
- one input variable: `x`
- floating-point constants
- `+`, `-`, `*`, `/`, unary `+`/`-`, and parentheses

The pipeline is lexer/parser -> AST -> optimization -> PTX emission.

## Optimizations
- Constant folding for constant-only arithmetic subtrees.
- Identity simplification for `x + 0`, `0 + x`, `x * 1`, `1 * x`, `x - 0`, and `x / 1`.
- Unsupported variables and constant division by zero are rejected.

## Important limitations
- This is a teaching compiler, not a general-purpose tensor compiler.
- The emitted PTX is a scalar demonstration kernel taking an output pointer and one scalar input. It is not yet a tensor/vector kernel.
- PTX has not been validated by `ptxas` or executed on a GPU in this milestone.
- Floating-point identity rewrites may differ for signed zero, NaNs, infinities, and strict IEEE edge cases; this project currently targets ordinary finite values.
- The PTX output's register declarations are emitted for inspection, but the next milestone must validate declaration/parameter consistency with NVIDIA's assembler before execution.
