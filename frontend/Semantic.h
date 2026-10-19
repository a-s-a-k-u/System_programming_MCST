//
// Created by lost-mem-ori on 18.10.2026.
//

#ifndef SYSTEM_PROGRAMMING_SEMANTIC_H
#define SYSTEM_PROGRAMMING_SEMANTIC_H
#pragma once
#include "AST.h"
#include <iostream>
#include <map>
#include <vector>
#include <string>

namespace frontend {

class SemanticAnalyzer {
    // Стек областей видимости: имя -> тип
    std::vector<std::map<std::string, TypeSpec>> scopes;
    // Таблица функций: имя -> {возвращаемый тип, список типов аргументов}
    std::map<std::string, std::pair<TypeSpec, std::vector<TypeSpec>>> functions;
    TypeSpec currentReturnType;
    bool hasError = false;

    void enterScope() { scopes.push_back({}); }
    void exitScope() { scopes.pop_back(); }

    void reportError(const std::string& msg) {
        std::cerr << "Semantic Error: " << msg << "\n";
        hasError = true;
    }

    void declareVar(const std::string& name, const TypeSpec& type) {
        auto& current = scopes.back();
        if (current.find(name) != current.end()) {
            reportError("Redeclaration of variable '" + name + "'");
        }
        current[name] = type;
    }

    TypeSpec lookupVar(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return found->second;
            }
        }
        reportError("Undeclared variable '" + name + "'");
        return {ScalarKind::Int32, {}}; // Заглушка, чтобы не крашить программу
    }

    bool checkTypesEqual(const TypeSpec& a, const TypeSpec& b) {
        return a.kind == b.kind && a.decorators == b.decorators;
    }

    TypeSpec checkExpr(AstNode* expr) {
        if (auto* n = dynamic_cast<IntConst*>(expr)) return {ScalarKind::Int32, {}};
        if (auto* n = dynamic_cast<CharConst*>(expr)) return {ScalarKind::Character, {}};
        if (auto* n = dynamic_cast<BoolConst*>(expr)) return {ScalarKind::Boolean, {}};
        if (auto* n = dynamic_cast<StrConst*>(expr)) return {ScalarKind::Character, {TypeDecorator::Pointer}};
        if (auto* n = dynamic_cast<Identifier*>(expr)) return lookupVar(n->name_);

        if (auto* n = dynamic_cast<SubscriptExpr*>(expr)) {
            TypeSpec baseType = checkExpr(n->base_.get());
            if (baseType.decorators.empty()) {
                reportError("Subscript operator applied to non-array/pointer type");
                return baseType;
            }
            baseType.decorators.pop_back(); // Убираем один уровень вложенности
            return baseType;
        }

        if (auto* n = dynamic_cast<BinaryExpr*>(expr)) {
            TypeSpec lhs = checkExpr(n->lhs_.get());
            TypeSpec rhs = checkExpr(n->rhs_.get());
            if (!checkTypesEqual(lhs, rhs)) {
                reportError("Type mismatch in binary expression");
            }
            switch (n->op_) {
                case BinKind::Less: case BinKind::Greater:
                case BinKind::LessEq: case BinKind::GreaterEq:
                case BinKind::Equal: case BinKind::NotEqual:
                    return {ScalarKind::Boolean, {}};
                default:
                    return lhs;
            }
        }

        if (auto* n = dynamic_cast<UnaryExpr*>(expr)) {
            return checkExpr(n->operand_.get());
        }

        if (auto* n = dynamic_cast<CallExpr*>(expr)) {
            auto it = functions.find(n->callee_);
            if (it == functions.end()) {
                reportError("Call to undeclared function '" + n->callee_ + "'");
                return {ScalarKind::Int32, {}};
            }
            if (it->second.second.size() != n->args_.size()) {
                reportError("Argument count mismatch for function '" + n->callee_ + "'");
            } else {
                for (size_t i = 0; i < n->args_.size(); ++i) {
                    TypeSpec argT = checkExpr(n->args_[i].get());
                    if (!checkTypesEqual(argT, it->second.second[i])) {
                        reportError("Argument type mismatch for function '" + n->callee_ + "'");
                    }
                }
            }
            return it->second.first; // Возвращаемый тип функции
        }

        return {ScalarKind::Int32, {}};
    }

    void checkStmt(AstNode* stmt) {
        if (auto* n = dynamic_cast<BlockStmt*>(stmt)) {
            enterScope();
            for (auto& item : n->items_) checkStmt(item.get());
            exitScope();
            return;
        }
        if (auto* n = dynamic_cast<VarDeclStmt*>(stmt)) {
            if (n->init_) {
                TypeSpec initType = checkExpr(n->init_.get());
                if (!checkTypesEqual(n->type_, initType)) {
                    reportError("Type mismatch in initialization of '" + n->name_ + "'");
                }
            }
            declareVar(n->name_, n->type_);
            return;
        }
        if (auto* n = dynamic_cast<AssignStmt*>(stmt)) {
            TypeSpec targetType = checkExpr(n->target_.get());
            TypeSpec valType = checkExpr(n->value_.get());
            if (!checkTypesEqual(targetType, valType)) {
                reportError("Type mismatch in assignment");
            }
            return;
        }
        if (auto* n = dynamic_cast<WhileStmt*>(stmt)) {
            TypeSpec condType = checkExpr(n->cond_.get());
            if (condType.kind != ScalarKind::Boolean || !condType.decorators.empty()) {
                reportError("While condition must be of type bool");
            }
            checkStmt(n->body_.get());
            return;
        }
        if (auto* n = dynamic_cast<IfStmt*>(stmt)) {
            for (auto& branch : n->branches_) {
                TypeSpec condType = checkExpr(branch.cond.get());
                if (condType.kind != ScalarKind::Boolean || !condType.decorators.empty()) {
                    reportError("If condition must be of type bool");
                }
                checkStmt(branch.body.get());
            }
            if (n->else_body_) {
                checkStmt(n->else_body_.get());
            }
            return;
        }
        if (auto* n = dynamic_cast<ReturnStmt*>(stmt)) {
            if (n->value_) {
                TypeSpec retType = checkExpr(n->value_.get());
                if (!checkTypesEqual(retType, currentReturnType)) {
                    reportError("Return type mismatch");
                }
            }
            return;
        }

        // Если это просто выражение (expr SEMI)
        checkExpr(stmt);
    }

public:
    void analyze(ProgramNode* root) {
        scopes.clear();
        functions.clear();
        hasError = false;
        enterScope(); // Глобальная область видимости

        // Проход 1: Собираем сигнатуры всех функций
        for (const auto& f : root->funcs_) {
            if (auto* func = dynamic_cast<FunctionDecl*>(f.get())) {
                std::vector<TypeSpec> paramTypes;
                for (const auto& p : func->params_) paramTypes.push_back(p.type);
                if (functions.find(func->name_) != functions.end()) {
                    reportError("Redeclaration of function '" + func->name_ + "'");
                }
                functions[func->name_] = {func->ret_type_, paramTypes};
            }
        }

        // Проход 2: Проверяем тела функций
        for (const auto& f : root->funcs_) {
            if (auto* func = dynamic_cast<FunctionDecl*>(f.get())) {
                currentReturnType = func->ret_type_;
                enterScope();
                for (const auto& p : func->params_) {
                    declareVar(p.name, p.type);
                }
                checkStmt(func->body_.get());
                exitScope();
            }
        }

        exitScope();

        if (hasError) {
            std::cerr << "\nSemantic analysis failed with errors.\n";
        } else {
            std::cout << "\nSemantic analysis passed successfully!\n";
        }
    }
};

} // namespace frontend
#endif //SYSTEM_PROGRAMMING_SEMANTIC_H