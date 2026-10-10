#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

EXPR="${1:-x + 5}"
INPUT="${2:-2.5}"
EXPECTED="${3:-7.5}"

echo "=== 1. Compile expression to PTX ==="
./build/ptxjit "$EXPR" artifacts/generated.ptx

echo
echo "=== 2. Assemble PTX for RTX 3050 ==="
ptxas -arch=sm_86 artifacts/generated.ptx \
    -o artifacts/generated_sm86.cubin

echo
echo "=== 3. Execute on GPU and verify result ==="
./build/gpu_runner artifacts/generated_sm86.cubin "$INPUT" "$EXPECTED"
