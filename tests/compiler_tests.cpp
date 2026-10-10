#include "ptxjit/compiler.hpp"
#include <cmath>
#include <functional>
#include <iostream>
#include <random>
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


static void test_optimizer_equivalence() {
    const std::vector<std::string> expressions = {
        "x + 0", "0 + x", "x - 0", "x * 1", "1 * x",
        "x / 1", "x * 1 + (2 + 3)", "(x + 0) * 2",
        "((x + 3) - 3) * 1", "2 + 3 * (x - 1)",
        "-(x + 2)", "--x", "x / (x + 20)",
        "1.25 * x + 0.5", "10 - 6 / 2"
    };
    const std::vector<float> inputs = {
        -10.0f, -3.25f, -1.0f, -0.5f, 0.0f,
        0.25f, 1.0f, 2.5f, 10.0f
    };

    std::size_t checks = 0;
    for (const auto& expression : expressions) {
        const auto compiled = ptxjit::compile_expression(expression);
        for (float x : inputs) {
            const float original = ptxjit::evaluate_expression(expression, x);
            const float optimized =
                ptxjit::evaluate_expression(compiled.optimized_expression, x);
            const float tolerance = 1e-4f * (1.0f + std::fabs(original));

            require(
                std::isfinite(original) && std::isfinite(optimized) &&
                std::fabs(original - optimized) <= tolerance,
                "Optimizer changed result for: " + expression
            );
            ++checks;
        }
    }
    std::cout << "Optimizer equivalence checks passed: "
              << checks << '\n';
}


static std::string generate_expression(std::mt19937& rng, int depth) {
    std::uniform_int_distribution<int> leaf_choice(0, 3);
    std::uniform_int_distribution<int> constant(1, 5);
    std::uniform_int_distribution<int> operation(0, 2);

    if (depth == 0 || leaf_choice(rng) == 0) {
        if (leaf_choice(rng) == 0)
            return "x";
        return std::to_string(constant(rng));
    }

    const char ops[] = {'+', '-', '*'};
    const char op = ops[operation(rng)];

    return "(" + generate_expression(rng, depth - 1) +
           " " + op + " " +
           generate_expression(rng, depth - 1) + ")";
}

static void test_randomized_optimizer_equivalence() {
    std::mt19937 rng(20261011);
    std::uniform_int_distribution<int> depth_distribution(1, 3);

    const std::vector<float> inputs = {
        -2.0f, -1.0f, -0.25f, 0.0f, 0.5f, 1.0f, 2.0f
    };

    constexpr int expression_count = 300;
    std::size_t checks = 0;

    for (int i = 0; i < expression_count; ++i) {
        const std::string expression =
            generate_expression(rng, depth_distribution(rng));
        const auto compiled = ptxjit::compile_expression(expression);

        for (float x : inputs) {
            const float original =
                ptxjit::evaluate_expression(expression, x);
            const float optimized =
                ptxjit::evaluate_expression(
                    compiled.optimized_expression, x);

            const float tolerance =
                1e-4f * (1.0f + std::fabs(original));

            require(
                std::isfinite(original) && std::isfinite(optimized) &&
                std::fabs(original - optimized) <= tolerance,
                "Randomized optimizer mismatch; expression: " +
                expression + ", input: " + std::to_string(x)
            );
            ++checks;
        }
    }

    std::cout << "Randomized optimizer checks passed: "
              << checks << " across " << expression_count
              << " generated expressions.\\n";
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

        test_optimizer_equivalence();
        test_randomized_optimizer_equivalence();

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
        require_throws(
            [] { (void)ptxjit::compile_expression("1.2.3"); },
            "Malformed decimal should be rejected"
        );
        require_throws(
            [] { (void)ptxjit::compile_expression("."); },
            "A lone decimal point should be rejected"
        );
        require_throws(
            [] { (void)ptxjit::compile_expression("1e+"); },
            "Incomplete exponent should be rejected"
        );
        require_throws(
            [] { (void)ptxjit::compile_expression("1e39"); },
            "Float overflow should be rejected"
        );

        std::cout << "All compiler and CPU evaluator tests passed.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "TEST FAILURE: " << e.what() << '\n';
        return 1;
    }
}
