#pragma once
#include "AST.h"
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <memory>

namespace frontend {

// --- Типы операндов ---
struct HIRValue {
    enum class Kind { Object, Constant, Label } kind;
    std::string name;        // имя объекта/метки
    TypeSpec type;           // тип для объектов
    int const_val = 0;       // для констант

    static HIRValue make_obj(const std::string& n, TypeSpec t) {
        return {Kind::Object, n, t, 0};
    }
    static HIRValue make_const(int v, TypeSpec t) {
        return {Kind::Constant, "", t, v};
    }
    static HIRValue make_label(const std::string& l) {
        return {Kind::Label, l, {}, 0};
    }
    std::string str() const {
        if (kind == Kind::Object)   return "%" + name;
        if (kind == Kind::Constant) return "#" + std::to_string(const_val);
        return name;
    }
};

// --- Категории операций ---
enum class OpKind {
    Const,          // const obj, #v
    BinArith,       // add/sub/mul/div/and/or/xor obj, obj1, obj2
    Compare,        // eq/ne/lt/gt/le/ge obj, obj1, obj2
    Load,           // ld obj, obj_ptr
    Store,          // st obj_ptr, obj
    AddrOf,         // addr obj, obj_ptr (взятие адреса)
    ElemPtr,        // elemptr obj, arr, idx
    Cast,           // cast obj, obj_src, type
    Label,          // label Lx:
    Jump,           // jump Lx
    Branch,         // branch cond, L_true, L_false
    Call,           // call callee, result, arg1, arg2, ...
    Ret             // ret obj (или ret)
};

// --- Операция в HIR ---
struct HIRInstr {
    OpKind kind;
    std::string op_str;              // "add", "lt", "jump", ...
    std::vector<HIRValue> args;      // фиксированные аргументы (до 3)
    HIRValue result;                 // куда писать (для операций, возвращающих значение)
    TypeSpec result_type;            // тип результата (для проверки)
};

// --- Объект (переменная или временное значение) ---
struct HIRObject {
    std::string name;
    TypeSpec type;
    bool is_temp;
};

// --- Функция ---
struct HIRFunction {
    std::string name;
    TypeSpec ret_type;
    std::vector<HIRObject> params;
    std::vector<HIRObject> locals;
    std::vector<HIRInstr> code;      // линейный IR

    // CFG: блоки и рёбра
    struct BasicBlock {
        std::string label;
        int start_idx;
        int end_idx;                 // эксклюзивно
        std::vector<std::string> succs;
    };
    std::vector<BasicBlock> blocks;  // заполняется в build_cfg
};

// --- Единица трансляции ---
struct HIRTranslationUnit {
    std::vector<HIRFunction> functions;
};

// --- Генератор ---
class IRGenerator {
public:
    HIRTranslationUnit generate(ProgramNode* root);

    // Проверка корректности (Задача 4.2)
    bool verify(const HIRTranslationUnit& tu, std::ostream& errors);

    // Разбиение на CFG (Задача 4.4)
    void build_cfg(HIRFunction& fn);

private:
    HIRTranslationUnit tu;
    HIRFunction* cur_fn = nullptr;
    int temp_counter = 0;
    int label_counter = 0;

    std::map<std::string, TypeSpec> var_types;

    HIRValue new_temp(TypeSpec t);
    HIRValue new_label();

    void gen_expr(AstNode* n, HIRValue& out);
    void gen_stmt(AstNode* n);
    void gen_block(BlockStmt* n);
};

    // --- Низкоуровневое представление (LIR) ---
    struct LIRInstr {
        std::string op;              // "mov", "add", "ld", "st", "push", "pop", "call", "ret", "jmp", "jz"
        std::string arg1, arg2, arg3;
    };

    class LIRGenerator {
    public:
        // Превращает HIR в псевдо-ассемблер
        std::vector<LIRInstr> generate(const HIRFunction& fn);
        void dump(const std::vector<LIRInstr>& code, std::ostream& os);
    };

} // namespace frontend