#include "ptxjit/compiler.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: ptxjit \"EXPRESSION\" [output.ptx]\n"
                  << "Supported variable: x; operators: + - * /; parentheses and float constants.\n";
        return 2;
    }
    try {
        auto result = ptxjit::compile_expression(argv[1]);
        std::cout << "Original:  " << result.original_expression << "\n"
                  << "Optimized: " << result.optimized_expression << "\n"
                  << "AST nodes: " << result.original_node_count << " -> "
                  << result.optimized_node_count << "\n";
        if (argc == 3) {
            std::ofstream file(argv[2]);
            if (!file) throw std::runtime_error("Cannot open output file: " + std::string(argv[2]));
            file << result.ptx;
            std::cout << "PTX written to: " << argv[2] << "\n";
        } else {
            std::cout << "\n" << result.ptx;
        }
    } catch (const std::exception& e) {
        std::cerr << "Compile error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
