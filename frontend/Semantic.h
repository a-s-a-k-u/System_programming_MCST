#ifndef SYSTEM_PROGRAMMING_SEMANTIC_H
#define SYSTEM_PROGRAMMING_SEMANTIC_H

#pragma once

#include "AST.h"
#include <iostream>
#include <map>
#include <vector>
#include <string>

namespace frontend {

/**
 * Семантический анализатор.
 * Реализует проверку областей видимости (scope resolution) и типов.
 * 
 * Архитектурное решение: мы не добавляем виртуальный метод check() в AstNode.
 * Вместо этого используем dynamic_cast. Почему?
 * 1. Разделение ответственности (SRP): AST-узлы ничего не знают о типах и таблицах символов.
 * 2. Легче поддерживать: вся логика проверок собрана в одном файле.
 * 3. В реальных компиляторах для этого обычно используется паттерн Visitor, 
 *    но dynamic_cast проще для понимания и вполне подходит для учебного проекта.
 */
class SemanticAnalyzer {
    // Стек для хранения областей видимости.
    // Каждый элемент стека — это хэш-таблица (map) имен переменных текущего блока.
    // При входе в блок (if, while, функция) мы делаем push, при выходе — pop.
    std::vector<std::map<std::string, TypeSpec>> scopes;
    
    // Глобальная таблица функций. 
    // Хранит сигнатуры (возвращаемый тип и типы аргументов), чтобы проверять вызовы.
    std::map<std::string, std::pair<TypeSpec, std::vector<TypeSpec>>> functions;
    
    // Ожидаемый тип возврата для текущей функции (для проверки return).
    TypeSpec currentReturnType;
    
    // Флаг, чтобы понять, были ли ошибки (для вывода итогового статуса).
    bool hasError = false;

    void enterScope() { scopes.push_back({}); }
    void exitScope() { scopes.pop_back(); }

    void reportError(const std::string& msg) {
        std::cerr << "Semantic Error: " << msg << "\n";
        hasError = true;
    }

    // Добавляет переменную в текущую область видимости.
    void declareVar(const std::string& name, const TypeSpec& type) {
        auto& current = scopes.back();
        if (current.find(name) != current.end()) {
            reportError("Redeclaration of variable '" + name + "'");
        }
        current[name] = type;
    }

    // Ищет переменную в стеке областей видимости (от внутренней к внешней).
    TypeSpec lookupVar(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return found->second;
            }
        }
        reportError("Undeclared variable '" + name + "'");
        return {ScalarKind::Int32, {}}; // Возвращаем заглушку, чтобы избежать каскада ошибок
    }

    // Простое сравнение типов (совпадает базовый тип и все модификаторы).
    bool checkTypesEqual(const TypeSpec& a, const TypeSpec& b) {
        return a.kind == b.kind && a.decorators == b.decorators;
    }

    // Рекурсивная проверка выражения. Возвращает тип, к которому сводится выражение.
    TypeSpec checkExpr(AstNode* expr) {
        // Литералы имеют фиксированные типы.
        if (auto* n = dynamic_cast<IntConst*>(expr)) return {ScalarKind::Int32, {}};
        if (auto* n = dynamic_cast<CharConst*>(expr)) return {ScalarKind::Character, {}};
        if (auto* n = dynamic_cast<BoolConst*>(expr)) return {ScalarKind::Boolean, {}};
        // Строковый литерал в C-подобных языках — это указатель на char.
        if (auto* n = dynamic_cast<StrConst*>(expr)) return {ScalarKind::Character, {TypeDecorator::Pointer}};
        
        if (auto* n = dynamic_cast<Identifier*>(expr)) return lookupVar(n->name_);

        // Проверка индексации: base должен быть массивом или указателем.
        if (auto* n = dynamic_cast<SubscriptExpr*>(expr)) {
            TypeSpec baseType = checkExpr(n->base_.get());
            if (baseType.decorators.empty()) {
                reportError("Subscript operator applied to non-array/pointer type");
                return baseType;
            }
            baseType.decorators.pop_back(); // При взятии индекса снимаем один уровень вложенности
            return baseType;
        }

        // Проверка бинарных операций (арифметика, сравнения).
        if (auto* n = dynamic_cast<BinaryExpr*>(expr)) {
            TypeSpec lhs = checkExpr(n->lhs_.get());
            TypeSpec rhs = checkExpr(n->rhs_.get());
            
            // Для простоты требуем строгого совпадения типов (без неявных приведений).
            if (!checkTypesEqual(lhs, rhs)) {
                reportError("Type mismatch in binary expression");
            }
            
            // Операции сравнения всегда возвращают bool.
            switch (n->op_) {
                case BinKind::Less: case BinKind::Greater:
                case BinKind::LessEq: case BinKind::GreaterEq:
                case BinKind::Equal: case BinKind::NotEqual:
                    return {ScalarKind::Boolean, {}};
                default:
                    return lhs; // Арифметика возвращает тип операндов
            }
        }

        if (auto* n = dynamic_cast<UnaryExpr*>(expr)) {
            return checkExpr(n->operand_.get());
        }

        // Проверка вызова функции.
        if (auto* n = dynamic_cast<CallExpr*>(expr)) {
            auto it = functions.find(n->callee_);
            if (it == functions.end()) {
                reportError("Call to undeclared function '" + n->callee_ + "'");
                return {ScalarKind::Int32, {}};
            }
            
            // Проверяем количество и типы аргументов.
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
            return it->second.first; // Возвращаем тип, который объявлен у функции
        }

        return {ScalarKind::Int32, {}}; // Fallback
    }

    // Рекурсивная проверка инструкций.
    void checkStmt(AstNode* stmt) {
        if (auto* n = dynamic_cast<BlockStmt*>(stmt)) {
            enterScope(); // Вход в новый блок = новая область видимости
            for (auto& item : n->items_) checkStmt(item.get());
            exitScope();  // Выход из блока = удаление локальных переменных
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
            // Условие цикла должно быть строго bool.
            if (condType.kind != ScalarKind::Boolean || !condType.decorators.empty()) {
                reportError("While condition must be of type bool");
            }
            checkStmt(n->body_.get());
            return;
        }
        
        if (auto* n = dynamic_cast<IfStmt*>(stmt)) {
            // Проверяем все ветки (if и elif)
            for (auto& branch : n->branches_) {
                TypeSpec condType = checkExpr(branch.cond.get());
                if (condType.kind != ScalarKind::Boolean || !condType.decorators.empty()) {
                    reportError("If condition must be of type bool");
                }
                checkStmt(branch.body.get());
            }
            // Проверяем else, если он есть
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

        // Если это просто выражение (например, вызов функции без присваивания).
        checkExpr(stmt);
    }

public:
    void analyze(ProgramNode* root) {
        scopes.clear();
        functions.clear();
        hasError = false;
        
        // Глобальная область видимости (для глобальных переменных, если бы они были).
        enterScope(); 

        // ПЕРВЫЙ ПРОХОД: Собираем сигнатуры всех функций.
        // Это нужно, чтобы функция могла вызывать другую функцию, 
        // объявленную ниже в исходном коде (forward declaration).
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

        // ВТОРОЙ ПРОХОД: Проверяем тела функций.
        for (const auto& f : root->funcs_) {
            if (auto* func = dynamic_cast<FunctionDecl*>(f.get())) {
                currentReturnType = func->ret_type_;
                
                // Область видимости параметров функции.
                enterScope();
                for (const auto& p : func->params_) {
                    declareVar(p.name, p.type);
                }
                
                checkStmt(func->body_.get());
                
                exitScope();
            }
        }

        exitScope(); // Выход из глобальной области

        if (hasError) {
            std::cerr << "\nSemantic analysis failed with errors.\n";
        } else {
            std::cout << "\nSemantic analysis passed successfully!\n";
        }
    }
};

} // namespace frontend
#endif //SYSTEM_PROGRAMMING_SEMANTIC_H
