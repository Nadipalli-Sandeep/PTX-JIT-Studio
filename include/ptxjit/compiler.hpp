#pragma once
#include <string>
#include <vector>

namespace ptxjit {
struct CompileResult {
    std::string original_expression;
    std::string optimized_expression;
    std::string ptx;
    int original_node_count = 0;
    int optimized_node_count = 0;
};
CompileResult compile_expression(const std::string& expression);
}
