#include "ptxjit/compiler.hpp"
#include <cctype>
#include <cmath>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace ptxjit {
namespace {
struct Node {
    enum class Kind { Constant, Variable, Binary } kind;
    float value = 0.0f;
    char op = 0;
    std::string name;
    std::unique_ptr<Node> left, right;
    explicit Node(float v) : kind(Kind::Constant), value(v) {}
    explicit Node(std::string n) : kind(Kind::Variable), name(std::move(n)) {}
    Node(char c, std::unique_ptr<Node> a, std::unique_ptr<Node> b)
        : kind(Kind::Binary), op(c), left(std::move(a)), right(std::move(b)) {}
};
enum class TokenKind { Number, Identifier, Plus, Minus, Star, Slash, LParen, RParen, End };
struct Token { TokenKind kind; std::string text; };

class Parser {
    std::string input;
    std::size_t pos = 0;
    Token current{TokenKind::End, ""};

    Token lex() {
        while (pos < input.size() && std::isspace(static_cast<unsigned char>(input[pos]))) ++pos;
        if (pos == input.size()) return {TokenKind::End, ""};
        char c = input[pos];
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            const std::size_t start = pos;
            bool digits = false;

            while (pos < input.size() &&
                   std::isdigit(static_cast<unsigned char>(input[pos]))) {
                digits = true;
                ++pos;
            }

            if (pos < input.size() && input[pos] == '.') {
                ++pos;
                while (pos < input.size() &&
                       std::isdigit(static_cast<unsigned char>(input[pos]))) {
                    digits = true;
                    ++pos;
                }
            }

            if (!digits)
                throw std::runtime_error("Invalid numeric literal");

            if (pos < input.size() &&
                (input[pos] == 'e' || input[pos] == 'E')) {
                ++pos;
                if (pos < input.size() &&
                    (input[pos] == '+' || input[pos] == '-')) ++pos;

                const std::size_t exponent_start = pos;
                while (pos < input.size() &&
                       std::isdigit(static_cast<unsigned char>(input[pos]))) ++pos;

                if (pos == exponent_start)
                    throw std::runtime_error("Invalid numeric exponent");
            }

            return {TokenKind::Number, input.substr(start, pos-start)};
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::size_t start = pos++;
            while (pos < input.size() &&
                   (std::isalnum(static_cast<unsigned char>(input[pos])) || input[pos] == '_')) ++pos;
            return {TokenKind::Identifier, input.substr(start, pos-start)};
        }
        ++pos;
        switch (c) {
            case '+': return {TokenKind::Plus, "+"};
            case '-': return {TokenKind::Minus, "-"};
            case '*': return {TokenKind::Star, "*"};
            case '/': return {TokenKind::Slash, "/"};
            case '(': return {TokenKind::LParen, "("};
            case ')': return {TokenKind::RParen, ")"};
            default: throw std::runtime_error(std::string("Unexpected character: ") + c);
        }
    }
    void advance() { current = lex(); }
    std::unique_ptr<Node> primary() {
        if (current.kind == TokenKind::Number) {
            float v;
            try {
                std::size_t consumed = 0;
                v = std::stof(current.text, &consumed);
                if (consumed != current.text.size() || !std::isfinite(v))
                    throw std::runtime_error("Invalid numeric literal");
            } catch (...) {
                throw std::runtime_error("Invalid numeric literal: " + current.text);
            }
            advance(); return std::make_unique<Node>(v);
        }
        if (current.kind == TokenKind::Identifier) {
            std::string name = current.text;
            if (name != "x") throw std::runtime_error("Only variable 'x' is supported; got '" + name + "'");
            advance(); return std::make_unique<Node>(name);
        }
        if (current.kind == TokenKind::LParen) {
            advance();
            auto n = expression();
            if (current.kind != TokenKind::RParen) throw std::runtime_error("Expected ')'");
            advance(); return n;
        }
        throw std::runtime_error("Expected a number, 'x', or '('");
    }
    std::unique_ptr<Node> unary() {
        if (current.kind == TokenKind::Minus) {
            advance();
            return std::make_unique<Node>('-', std::make_unique<Node>(0.0f), unary());
        }
        if (current.kind == TokenKind::Plus) { advance(); return unary(); }
        return primary();
    }
    std::unique_ptr<Node> term() {
        auto n = unary();
        while (current.kind == TokenKind::Star || current.kind == TokenKind::Slash) {
            char op = current.kind == TokenKind::Star ? '*' : '/';
            advance(); n = std::make_unique<Node>(op, std::move(n), unary());
        }
        return n;
    }
    std::unique_ptr<Node> expression() {
        auto n = term();
        while (current.kind == TokenKind::Plus || current.kind == TokenKind::Minus) {
            char op = current.kind == TokenKind::Plus ? '+' : '-';
            advance(); n = std::make_unique<Node>(op, std::move(n), term());
        }
        return n;
    }
public:
    explicit Parser(std::string s) : input(std::move(s)) { advance(); }
    std::unique_ptr<Node> parse() {
        auto n = expression();
        if (current.kind != TokenKind::End) throw std::runtime_error("Unexpected token: " + current.text);
        return n;
    }
};

int count_nodes(const Node& n) {
    if (n.kind != Node::Kind::Binary) return 1;
    return 1 + count_nodes(*n.left) + count_nodes(*n.right);
}
std::unique_ptr<Node> clone(const Node& n) {
    if (n.kind == Node::Kind::Constant) return std::make_unique<Node>(n.value);
    if (n.kind == Node::Kind::Variable) return std::make_unique<Node>(n.name);
    return std::make_unique<Node>(n.op, clone(*n.left), clone(*n.right));
}
std::unique_ptr<Node> optimize(std::unique_ptr<Node> n) {
    if (n->kind != Node::Kind::Binary) return n;
    n->left = optimize(std::move(n->left));
    n->right = optimize(std::move(n->right));
    if (n->left->kind == Node::Kind::Constant && n->right->kind == Node::Kind::Constant) {
        float a = n->left->value, b = n->right->value, v = 0.0f;
        switch (n->op) {
            case '+': v = a+b; break; case '-': v = a-b; break;
            case '*': v = a*b; break;
            case '/':
                if (b == 0.0f) throw std::runtime_error("Division by zero in constant expression");
                v = a/b; break;
        }
        if (!std::isfinite(v)) throw std::runtime_error("Non-finite constant expression");
        return std::make_unique<Node>(v);
    }
    // Algebraic identities that preserve ordinary finite float semantics.
    if (n->op == '+' && n->right->kind == Node::Kind::Constant && n->right->value == 0.0f)
        return std::move(n->left);
    if (n->op == '+' && n->left->kind == Node::Kind::Constant && n->left->value == 0.0f)
        return std::move(n->right);
    if (n->op == '*' && n->right->kind == Node::Kind::Constant && n->right->value == 1.0f)
        return std::move(n->left);
    if (n->op == '*' && n->left->kind == Node::Kind::Constant && n->left->value == 1.0f)
        return std::move(n->right);
    if (n->op == '-' && n->right->kind == Node::Kind::Constant && n->right->value == 0.0f)
        return std::move(n->left);
    if (n->op == '/' && n->right->kind == Node::Kind::Constant && n->right->value == 1.0f)
        return std::move(n->left);
    return n;
}
std::string fmt(float v) {
    std::ostringstream os; os << std::setprecision(9) << v;
    std::string s = os.str();
    if (s.find_first_of(".eE") == std::string::npos) s += ".0";
    return s;
}
std::string expr(const Node& n) {
    if (n.kind == Node::Kind::Constant) return fmt(n.value);
    if (n.kind == Node::Kind::Variable) return n.name;
    return "(" + expr(*n.left) + " " + n.op + " " + expr(*n.right) + ")";
}
class Emitter {
    std::ostringstream out;
    int next_reg = 0;
    std::string reg() { return "%r" + std::to_string(next_reg++); }
    std::string emit(const Node& n) {
        if (n.kind == Node::Kind::Variable) return "%x";
        if (n.kind == Node::Kind::Constant) {
            std::string r = reg();
            out << "    mov.f32 " << r << ", " << fmt(n.value) << ";\n";
            return r;
        }
        std::string a = emit(*n.left), b = emit(*n.right), r = reg();
        const char* op = n.op == '+' ? "add.f32" : n.op == '-' ? "sub.f32" :
                         n.op == '*' ? "mul.f32" : "div.rn.f32";
        out << "    " << op << " " << r << ", " << a << ", " << b << ";\n";
        return r;
    }
public:
    std::string generate(const Node& n) {
        std::string result = emit(n);
        out << "    st.global.f32 [%rd_out], " << result << ";\n";
        return
            ".version 7.0\n"
            ".target sm_50\n"
            ".address_size 64\n\n"
            ".visible .entry ptxjit_kernel(\n"
            "    .param .u64 p_out,\n"
            "    .param .f32 p_x\n"
            ")\n"
            "{\n"
            "    .reg .pred %p<2>;\n"
            "    .reg .b64 %rd<2>;\n"
            "    .reg .b64 %rd_out;\n"
            "    .reg .f32 %x;\n"
            "    .reg .f32 %r<" + std::to_string(next_reg + 1) + ">;\n"
            "    ld.param.u64 %rd_out, [p_out];\n"
            "    ld.param.f32 %x, [p_x];\n" + out.str() +
            "    ret;\n}\n";
    }
};

float eval_node(const Node& n, float x) {
    if (n.kind == Node::Kind::Constant)
        return n.value;

    if (n.kind == Node::Kind::Variable)
        return x;

    const float a = eval_node(*n.left, x);
    const float b = eval_node(*n.right, x);

    switch (n.op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/':
            if (b == 0.0f)
                throw std::runtime_error("Division by zero during evaluation");
            return a / b;
        default:
            throw std::runtime_error("Unsupported operator");
    }
}
}

float evaluate_expression(const std::string& expression, float x) {
    Parser parser(expression);
    auto ast = parser.parse();
    return eval_node(*ast, x);
}

CompileResult compile_expression(const std::string& expression) {
    Parser p(expression);
    auto ast = p.parse();
    CompileResult result;
    result.original_expression = expression;
    result.original_node_count = count_nodes(*ast);
    auto optimized = optimize(clone(*ast));
    result.optimized_expression = expr(*optimized);
    result.optimized_node_count = count_nodes(*optimized);
    Emitter e;
    result.ptx = e.generate(*optimized);
    return result;
}
}
