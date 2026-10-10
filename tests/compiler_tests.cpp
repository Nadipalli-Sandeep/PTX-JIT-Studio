#include "ptxjit/compiler.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

static void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
int main() {
    try {
        auto a = ptxjit::compile_expression("x * 1 + (2 + 3)");
        require(a.optimized_expression.find("x") != std::string::npos, "variable lost");
        require(a.optimized_expression.find("5.0") != std::string::npos, "constant folding failed");
        require(a.optimized_node_count < a.original_node_count, "optimizer did not reduce AST nodes");
        require(a.ptx.find(".visible .entry ptxjit_kernel") != std::string::npos, "kernel missing");
        require(a.ptx.find("add.f32") != std::string::npos, "expected x + folded constant operation");
        auto b = ptxjit::compile_expression("(x + 0) * 2");
        require(b.optimized_expression.find("x") != std::string::npos, "identity simplification failed");
        require(b.ptx.find("mul.f32") != std::string::npos, "multiply instruction missing");
        bool rejected = false;
        try { (void)ptxjit::compile_expression("x + y"); }
        catch (...) { rejected = true; }
        require(rejected, "unsupported variable should be rejected");
        rejected = false;
        try { (void)ptxjit::compile_expression("2 / 0"); }
        catch (...) { rejected = true; }
        require(rejected, "constant division by zero should be rejected");
        std::cout << "All compiler tests passed.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "TEST FAILURE: " << e.what() << "\n";
        return 1;
    }
}
