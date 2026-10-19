#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <ostream>

namespace frontend {

class SemanticAnalyzer; // <--- Предварительное объявление

// --- Типы ---------------------------------------------------------------
enum class ScalarKind { Int32, Character, Boolean };
enum class TypeDecorator { Pointer, Array };

struct TypeSpec {
    ScalarKind kind;
    std::vector<TypeDecorator> decorators;

    std::string render() const {
        std::string out;
        for (auto d : decorators) {
            if (d == TypeDecorator::Pointer) out += "ptr$ ";
        }
        switch (kind) {
            case ScalarKind::Int32:     out += "i32";  break;
            case ScalarKind::Character: out += "char"; break;
            case ScalarKind::Boolean:   out += "bool"; break;
        }
        for (auto d : decorators) {
            if (d == TypeDecorator::Array) out += "[]";
        }
        return out;
    }
};

// --- Операторы ----------------------------------------------------------
enum class BinKind {
    Plus, Minus, Multiply, Divide,
    BitAnd, BitOr, BitXor,
    Equal, NotEqual, Less, Greater, LessEq, GreaterEq
};

enum class UnaryKind { Negate, Posit };

inline const char* bin_kind_name(BinKind k) {
    switch (k) {
        case BinKind::Plus:      return "+";
        case BinKind::Minus:     return "-";
        case BinKind::Multiply:  return "*";
        case BinKind::Divide:    return "/";
        case BinKind::BitAnd:    return "&";
        case BinKind::BitOr:     return "|";
        case BinKind::BitXor:    return "^";
        case BinKind::Equal:     return "==";
        case BinKind::NotEqual:  return "!=";
        case BinKind::Less:      return "<";
        case BinKind::Greater:   return ">";
        case BinKind::LessEq:    return "<=";
        case BinKind::GreaterEq: return ">=";
    }
    return "?";
}

// --- Базовый узел -------------------------------------------------------
class AstNode {
protected:
    static void write_indent(int depth) {
        while (depth-- > 0) std::cout << "  ";
    }
public:
    friend class SemanticAnalyzer; // <---
    virtual ~AstNode() = default;
    virtual void describe(int depth = 0) const = 0;
    virtual void emit_dot(std::ostream& out) const = 0;
    std::size_t uid() const { return reinterpret_cast<std::size_t>(this); }
};

// --- Литералы -----------------------------------------------------------
class IntConst : public AstNode {
    friend class SemanticAnalyzer; // <---
    int value_;
public:
    explicit IntConst(int v) : value_(v) {}
    void describe(int depth = 0) const override {
        write_indent(depth); std::cout << "Int(" << value_ << ")\n";
    }
    void emit_dot(std::ostream& out) const override {
        out << "  node_" << uid() << " [label=\"int: " << value_ << "\", fillcolor=\"#c8e6c9\"];\n";
    }
};

class CharConst : public AstNode {
    friend class SemanticAnalyzer; // <---
    char value_;
public:
    explicit CharConst(char v) : value_(v) {}
    void describe(int depth = 0) const override {
        write_indent(depth); std::cout << "Char('" << value_ << "')\n";
    }
    void emit_dot(std::ostream& out) const override {
        out << "  node_" << uid() << " [label=\"char: '" << value_ << "'\", fillcolor=\"#c8e6c9\"];\n";
    }
};

class BoolConst : public AstNode {
    friend class SemanticAnalyzer; // <---
    bool value_;
public:
    explicit BoolConst(bool v) : value_(v) {}
    void describe(int depth = 0) const override {
        write_indent(depth); std::cout << "Bool(" << (value_ ? "true" : "false") << ")\n";
    }
    void emit_dot(std::ostream& out) const override {
        out << "  node_" << uid() << " [label=\"bool: " << (value_ ? "true" : "false") << "\", fillcolor=\"#c8e6c9\"];\n";
    }
};

class StrConst : public AstNode {
    friend class SemanticAnalyzer; // <---
    std::string value_;
public:
    explicit StrConst(const std::string& v) : value_(v) {}
    void describe(int depth = 0) const override {
        write_indent(depth); std::cout << "Str(\"" << value_ << "\")\n";
    }
    void emit_dot(std::ostream& out) const override {
        out << "  node_" << uid() << " [label=\"str: \\\"" << value_ << "\\\"\", fillcolor=\"#c8e6c9\"];\n";
    }
};

class Identifier : public AstNode {
    friend class SemanticAnalyzer; // <---
    std::string name_;
public:
    explicit Identifier(const std::string& n) : name_(n) {}
    const std::string& name() const { return name_; }
    void describe(int depth = 0) const override {
        write_indent(depth); std::cout << "Id(" << name_ << ")\n";
    }
    void emit_dot(std::ostream& out) const override {
        out << "  node_" << uid() << " [label=\"name: " << name_ << "\", fillcolor=\"#c8e6c9\"];\n";
    }
};

// --- Выражения ----------------------------------------------------------
class SubscriptExpr : public AstNode {
    friend class SemanticAnalyzer; // <---
    std::unique_ptr<AstNode> base_, index_;
public:
    SubscriptExpr(AstNode* base, AstNode* index) : base_(base), index_(index) {}
    void describe(int depth = 0) const override {
        write_indent(depth); std::cout << "Subscript:\n";
        base_->describe(depth + 1); index_->describe(depth + 1);
    }
    void emit_dot(std::ostream& out) const override {
        out << "  node_" << uid() << " [label=\"[]\"];\n";
        out << "  node_" << uid() << " -> node_" << base_->uid() << ";\n"; base_->emit_dot(out);
        out << "  node_" << uid() << " -> node_" << index_->uid() << ";\n"; index_->emit_dot(out);
    }
};

class BinaryExpr : public AstNode {
    friend class SemanticAnalyzer; // <---
    BinKind op_;
    std::unique_ptr<AstNode> lhs_, rhs_;
public:
    BinaryExpr(BinKind op, AstNode* lhs, AstNode* rhs) : op_(op), lhs_(lhs), rhs_(rhs) {}
    void describe(int depth = 0) const override {
        write_indent(depth); std::cout << "BinOp(" << bin_kind_name(op_) << "):\n";
        lhs_->describe(depth + 1); rhs_->describe(depth + 1);
    }
    void emit_dot(std::ostream& out) const override {
        out << "  node_" << uid() << " [label=\"" << bin_kind_name(op_) << "\"];\n";
        out << "  node_" << uid() << " -> node_" << lhs_->uid() << ";\n"; lhs_->emit_dot(out);
        out << "  node_" << uid() << " -> node_" << rhs_->uid() << ";\n"; rhs_->emit_dot(out);
    }
};

class UnaryExpr : public AstNode {
    friend class SemanticAnalyzer; // <---
    UnaryKind op_;
    std::unique_ptr<AstNode> operand_;
public:
    UnaryExpr(UnaryKind op, AstNode* operand) : op_(op), operand_(operand) {}
    void describe(int depth = 0) const override {
        write_indent(depth); std::cout << "Unary(" << (op_ == UnaryKind::Negate ? "-" : "+") << ")\n";
        operand_->describe(depth + 1);
    }
    void emit_dot(std::ostream& out) const override {
        out << "  node_" << uid() << " [label=\"" << (op_ == UnaryKind::Negate ? "-" : "+") << "\"];\n";
        out << "  node_" << uid() << " -> node_" << operand_->uid() << ";\n"; operand_->emit_dot(out);
    }
};

class CallExpr : public AstNode {
    friend class SemanticAnalyzer; // <---
    std::string callee_;
    std::vector<std::unique_ptr<AstNode>> args_;
public:
    CallExpr(const std::string& callee, std::vector<AstNode*> args) : callee_(callee) {
        for (auto a : args) args_.emplace_back(a);
    }
    void describe(int depth = 0) const override {
        write_indent(depth); std::cout << "Call(" << callee_ << ")\n";
        for (const auto& a : args_) a->describe(depth + 1);
    }
    void emit_dot(std::ostream& out) const override {
        out << "  node_" << uid() << " [label=\"call " << callee_ << "\"];\n";
        int i = 0;
        for (const auto& a : args_) {
            out << "  node_" << uid() << " -> node_" << a->uid() << " [label=\"arg" << i++ << "\"];\n";
            a->emit_dot(out);
        }
    }
};

// --- Инструкции ---------------------------------------------------------
class BlockStmt : public AstNode {
    friend class SemanticAnalyzer; // <---
    std::vector<std::unique_ptr<AstNode>> items_;
public:
    void append(AstNode* stmt) { if (stmt) items_.emplace_back(stmt); }
    void describe(int depth = 0) const override {
        write_indent(depth); std::cout << "Block:\n";
        for (const auto& s : items_) s->describe(depth + 1);
    }
    void emit_dot(std::ostream& out) const override {
        out << "  node_" << uid() << " [label=\"block\"];\n";
        for (const auto& s : items_) {
            out << "  node_" << uid() << " -> node_" << s->uid() << ";\n"; s->emit_dot(out);
        }
    }
};

class VarDeclStmt : public AstNode {
    friend class SemanticAnalyzer; // <---
    TypeSpec type_;
    std::string name_;
    std::unique_ptr<AstNode> init_;
public:
    VarDeclStmt(TypeSpec type, const std::string& name, AstNode* init = nullptr)
        : type_(std::move(type)), name_(name), init_(init) {}
    void describe(int depth = 0) const override {
        write_indent(depth); std::cout << "VarDecl(" << type_.render() << " " << name_ << ")\n";
        if (init_) init_->describe(depth + 1);
    }
    void emit_dot(std::ostream& out) const override {
        out << "  node_" << uid() << " [label=\"var " << name_ << "\\n" << type_.render() << "\"];\n";
        if (init_) { out << "  node_" << uid() << " -> node_" << init_->uid() << ";\n"; init_->emit_dot(out); }
    }
};

class AssignStmt : public AstNode {
    friend class SemanticAnalyzer; // <---
    std::unique_ptr<AstNode> target_, value_;
    bool add_;
public:
    AssignStmt(AstNode* target, AstNode* value, bool add) : target_(target), value_(value), add_(add) {}
    void describe(int depth = 0) const override {
        write_indent(depth); std::cout << (add_ ? "AddAssign:\n" : "Assign:\n");
        target_->describe(depth + 1); value_->describe(depth + 1);
    }
    void emit_dot(std::ostream& out) const override {
        out << "  node_" << uid() << " [label=\"" << (add_ ? "+=" : "=") << "\"];\n";
        out << "  node_" << uid() << " -> node_" << target_->uid() << " [label=\"target\"];\n"; target_->emit_dot(out);
        out << "  node_" << uid() << " -> node_" << value_->uid() << " [label=\"value\"];\n"; value_->emit_dot(out);
    }
};

class ReturnStmt : public AstNode {
    friend class SemanticAnalyzer; // <---
    std::unique_ptr<AstNode> value_;
public:
    explicit ReturnStmt(AstNode* value = nullptr) : value_(value) {}
    void describe(int depth = 0) const override {
        write_indent(depth); std::cout << "Return:\n";
        if (value_) value_->describe(depth + 1);
    }
    void emit_dot(std::ostream& out) const override {
        out << "  node_" << uid() << " [label=\"return\"];\n";
        if (value_) { out << "  node_" << uid() << " -> node_" << value_->uid() << ";\n"; value_->emit_dot(out); }
    }
};

class WhileStmt : public AstNode {
    friend class SemanticAnalyzer; // <---
    std::unique_ptr<AstNode> cond_, body_;
public:
    WhileStmt(AstNode* cond, AstNode* body) : cond_(cond), body_(body) {}
    void describe(int depth = 0) const override {
        write_indent(depth); std::cout << "While:\n";
        write_indent(depth + 1); std::cout << "cond:\n"; cond_->describe(depth + 2);
        write_indent(depth + 1); std::cout << "body:\n"; body_->describe(depth + 2);
    }
    void emit_dot(std::ostream& out) const override {
        out << "  node_" << uid() << " [label=\"while\"];\n";
        out << "  node_" << uid() << " -> node_" << cond_->uid() << " [label=\"cond\"];\n"; cond_->emit_dot(out);
        out << "  node_" << uid() << " -> node_" << body_->uid() << " [label=\"body\"];\n"; body_->emit_dot(out);
    }
};

class IfStmt : public AstNode {
    friend class SemanticAnalyzer; // <---
public:
    struct Branch { std::unique_ptr<AstNode> cond, body; };
private:
    std::vector<Branch> branches_;
    std::unique_ptr<AstNode> else_body_;
public:
    IfStmt(AstNode* cond, AstNode* body) { add_branch(cond, body); }
    void add_branch(AstNode* cond, AstNode* body) { branches_.push_back({std::unique_ptr<AstNode>(cond), std::unique_ptr<AstNode>(body)}); }
    void set_else(AstNode* body) { else_body_.reset(body); }
    void describe(int depth = 0) const override {
        write_indent(depth); std::cout << "If:\n";
        for (size_t i = 0; i < branches_.size(); ++i) {
            write_indent(depth + 1); std::cout << (i == 0 ? "if:\n" : "elif:\n");
            branches_[i].cond->describe(depth + 2);
            write_indent(depth + 1); std::cout << "then:\n";
            branches_[i].body->describe(depth + 2);
        }
        if (else_body_) { write_indent(depth + 1); std::cout << "else:\n"; else_body_->describe(depth + 2); }
    }
    void emit_dot(std::ostream& out) const override {
        out << "  node_" << uid() << " [label=\"if-chain\"];\n";
        int i = 0;
        for (const auto& b : branches_) {
            std::string pre = (i++ == 0) ? "if" : "elif";
            out << "  node_" << uid() << " -> node_" << b.cond->uid() << " [label=\"" << pre << " cond\"];\n"; b.cond->emit_dot(out);
            out << "  node_" << uid() << " -> node_" << b.body->uid() << " [label=\"" << pre << " body\"];\n"; b.body->emit_dot(out);
        }
        if (else_body_) { out << "  node_" << uid() << " -> node_" << else_body_->uid() << " [label=\"else\"];\n"; else_body_->emit_dot(out); }
    }
};

struct ParamSpec { TypeSpec type; std::string name; };

class FunctionDecl : public AstNode {
    friend class SemanticAnalyzer; // <---
    TypeSpec ret_type_;
    std::string name_;
    std::vector<ParamSpec> params_;
    std::unique_ptr<AstNode> body_;
public:
    FunctionDecl(TypeSpec ret, const std::string& name, const std::vector<ParamSpec>& params, AstNode* body)
        : ret_type_(std::move(ret)), name_(name), params_(params), body_(body) {}
    void describe(int depth = 0) const override {
        write_indent(depth); std::cout << "Func(" << ret_type_.render() << " " << name_ << ")\n";
        write_indent(depth + 1); std::cout << "params:\n";
        for (const auto& p : params_) { write_indent(depth + 2); std::cout << p.type.render() << " " << p.name << "\n"; }
        write_indent(depth + 1); std::cout << "body:\n"; body_->describe(depth + 2);
    }
    void emit_dot(std::ostream& out) const override {
        std::string ps;
        for (size_t i = 0; i < params_.size(); ++i) {
            ps += params_[i].type.render() + " " + params_[i].name;
            if (i + 1 < params_.size()) ps += ", ";
        }
        out << "  node_" << uid() << " [label=\"func " << name_ << "\\n" << ret_type_.render() << "(" << ps << ")\"];\n";
        out << "  node_" << uid() << " -> node_" << body_->uid() << " [label=\"body\"];\n"; body_->emit_dot(out);
    }
};

class ProgramNode : public AstNode {
    friend class SemanticAnalyzer; // <---
    std::vector<std::unique_ptr<AstNode>> funcs_;
public:
    void add_function(AstNode* fn) { if (fn) funcs_.emplace_back(fn); }
    void describe(int depth = 0) const override {
        std::cout << "Program:\n";
        for (const auto& f : funcs_) f->describe(depth + 1);
    }
    void emit_dot(std::ostream& out) const override {
        out << "  node_" << uid() << " [label=\"program\"];\n";
        for (const auto& f : funcs_) { out << "  node_" << uid() << " -> node_" << f->uid() << ";\n"; f->emit_dot(out); }
    }
};

} // namespace frontend