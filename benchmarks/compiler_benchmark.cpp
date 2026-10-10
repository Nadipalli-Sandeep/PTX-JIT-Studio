#include "ptxjit/compiler.hpp"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main() {
    const std::vector<std::string> expressions = {
        "x + 5",
        "x * 2 + 3",
        "(x + 2) * 3",
        "x * 1 + (2 + 3)",
        "(x - 4) / 2 + 7"
    };

    constexpr int warmup_rounds = 100;
    constexpr int measured_rounds = 5000;

    std::uint64_t checksum = 0;

    for (int i = 0; i < warmup_rounds; ++i) {
        auto result = ptxjit::compile_expression(
            expressions[i % expressions.size()]);
        checksum += result.ptx.size();
    }

    const auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < measured_rounds; ++i) {
        auto result = ptxjit::compile_expression(
            expressions[i % expressions.size()]);
        checksum += result.ptx.size();
        checksum += static_cast<std::uint64_t>(
            result.optimized_node_count);
    }

    const auto finish = std::chrono::steady_clock::now();

    const double elapsed_ms =
        std::chrono::duration<double, std::milli>(
            finish - start).count();

    const double average_us =
        elapsed_ms * 1000.0 / measured_rounds;

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Compiler benchmark\n";
    std::cout << "Expressions:       " << expressions.size() << '\n';
    std::cout << "Measured rounds:   " << measured_rounds << '\n';
    std::cout << "Total time:        " << elapsed_ms << " ms\n";
    std::cout << "Average per compile: " << average_us << " us\n";
    std::cout << "Checksum:          " << checksum << '\n';

    return 0;
}
