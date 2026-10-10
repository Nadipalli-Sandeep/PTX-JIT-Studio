#include "ptxjit/compiler.hpp"
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}

static void require_throws(
    const std::function<void()>& fn,
    const std::string& message
) {
    bool threw = false;
    try {
        fn();
    } catch (const std::exception&) {
        threw = true;
    }
    require(threw, message);
}

static void check_value(
    const std::string& expression,
    float x,
    float expected
) {
    const float actual = ptxjit::evaluate_expression(expression, x);
    require(
        std::isfinite(actual) && std::fabs(actual - expected) <= 1e-5f,
        "Wrong CPU result for: " + expression
    );
}

int main() {
    try {
        // Compiler and optimizer tests.
        auto a = ptxjit::compile_expression("x * 1 + (2 + 3)");
        require(a.optimized_expression.find("x") != std::string::npos,
                "Variable lost");
        require(a.optimized_expression.find("5.0") != std::string::npos,
                "Constant folding failed");
        require(a.optimized_node_count < a.original_node_count,
                "Optimizer did not reduce AST nodes");
        require(a.ptx.find(".visible .entry ptxjit_kernel") != std::string::npos,
                "Kernel missing");
        require(a.ptx.find("add.f32") != std::string::npos,
                "Expected addition instruction missing");

        auto b = ptxjit::compile_expression("(x + 0) * 2");
        require(b.optimized_expression.find("x") != std::string::npos,
                "Identity simplification failed");
        require(b.ptx.find("mul.f32") != std::string::npos,
                "Multiply instruction missing");

        auto c = ptxjit::compile_expression("x * 1 + (2 + 3)");
        require(c.optimized_node_count == 3,
                "Unexpected optimized AST node count");

        // CPU evaluator: operators, precedence, parentheses and unary signs.
        check_value("x + 5", 2.5f, 7.5f);
        check_value("x - 2", 5.0f, 3.0f);
        check_value("x * 2 + 3", 4.0f, 11.0f);
        check_value("x / 2", 10.0f, 5.0f);
        check_value("(x + 2) * 3", 4.0f, 18.0f);
        check_value("x * 1 + (2 + 3)", 4.0f, 9.0f);
        check_value("-x + 2", 3.0f, -1.0f);
        check_value("--x", 3.0f, 3.0f);
        check_value("-(x + 2)", 3.0f, -5.0f);
        check_value("10 - 6 / 2", 4.0f, 7.0f);
        check_value("2 * (x - 1)", 4.0f, 6.0f);

        // Invalid expressions and division-by-zero handling.
        require_throws(
            [] { (void)ptxjit::compile_expression("x + y"); },
            "Unsupported variable should be rejected"
        );
        require_throws(
            [] { (void)ptxjit::compile_expression("2 / 0"); },
            "Constant division by zero should be rejected"
        );
        require_throws(
            [] { (void)ptxjit::evaluate_expression("1 / (x - 2)", 2.0f); },
            "Runtime division by zero should be rejected"
        );
        require_throws(
            [] { (void)ptxjit::compile_expression("x +"); },
            "Incomplete expression should be rejected"
        );
        require_throws(
            [] { (void)ptxjit::compile_expression("(x + 2"); },
            "Unclosed parenthesis should be rejected"
        );

        std::cout << "All compiler and CPU evaluator tests passed.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "TEST FAILURE: " << e.what() << '\n';
        return 1;
    }
}
