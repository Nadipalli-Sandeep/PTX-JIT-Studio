#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if ! command -v ptxas >/dev/null 2>&1; then
    echo "ERROR: ptxas not found. Install or load the CUDA toolkit first."
    exit 2
fi

cmake --build build -j2
OUT="build/ptx-validation"
mkdir -p "$OUT"

expressions=(
    "x + 5"
    "x * 2 + 3"
    "x * 1 + (2 + 3)"
    "(x + 0) * 2"
    "-(x + 2)"
    "x / 2"
)

index=0
for expression in "${expressions[@]}"; do
    ptx="$OUT/case_${index}.ptx"
    cubin="$OUT/case_${index}.cubin"

    echo "Validating: $expression"
    ./build/ptxjit "$expression" "$ptx"
    ptxas -arch=sm_86 "$ptx" -o "$cubin"
    echo "PASS: $expression"
    index=$((index + 1))
done

echo "All $index PTX assembler validations passed."
