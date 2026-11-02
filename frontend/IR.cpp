#include "IR.h"
#include <algorithm>
namespace frontend {

    HIRValue IRGenerator::new_temp(TypeSpec t) {
        std::string name = "t" + std::to_string(temp_counter++);
        cur_fn->locals.push_back({name, t, true});
        return HIRValue::make_obj(name, t);
    }

    HIRValue IRGenerator::new_label() {
        return HIRValue::make_label("L" + std::to_string(label_counter++));
    }

    // Генерация выражений. Пишет операцию и возвращает HIRValue результата.
    void IRGenerator::gen_expr(AstNode* node, HIRValue& out) {
        if (auto* n = dynamic_cast<IntConst*>(node)) {
            out = new_temp({ScalarKind::Int32, {}});
            cur_fn->code.push_back({OpKind::Const, "const", {}, out, {ScalarKind::Int32, {}}});
            // Перезапишем аргумент на константу для наглядности
            cur_fn->code.back().args.push_back(HIRValue::make_const(n->value_, {ScalarKind::Int32, {}}));
            return;
        }
        if (auto* n = dynamic_cast<BoolConst*>(node)) {
            out = new_temp({ScalarKind::Boolean, {}});
            cur_fn->code.push_back({OpKind::Const, "const",
                {HIRValue::make_const(n->value_ ? 1 : 0, {ScalarKind::Boolean, {}})},
                out, {ScalarKind::Boolean, {}}});
            return;
        }
        if (auto* n = dynamic_cast<Identifier*>(node)) {
            out = HIRValue::make_obj(n->name_, var_types[n->name_]);
            return;
        }
        if (auto* n = dynamic_cast<BinaryExpr*>(node)) {
            HIRValue lhs, rhs;
            gen_expr(n->lhs_.get(), lhs);
            gen_expr(n->rhs_.get(), rhs);
            TypeSpec t = lhs.type;  // упрощение
            out = new_temp(t);
            std::string op;
            OpKind k = OpKind::BinArith;
            switch (n->op_) {
                case BinKind::Plus: op = "add"; break;
                case BinKind::Minus: op = "sub"; break;
                case BinKind::Multiply: op = "mul"; break;
                case BinKind::Divide: op = "div"; break;
                case BinKind::BitAnd: op = "and"; break;
                case BinKind::BitOr: op = "or"; break;
                case BinKind::BitXor: op = "xor"; break;
                case BinKind::Less: op = "lt"; k = OpKind::Compare; t = {ScalarKind::Boolean, {}}; break;
                case BinKind::Greater: op = "gt"; k = OpKind::Compare; t = {ScalarKind::Boolean, {}}; break;
                case BinKind::LessEq: op = "le"; k = OpKind::Compare; t = {ScalarKind::Boolean, {}}; break;
                case BinKind::GreaterEq: op = "ge"; k = OpKind::Compare; t = {ScalarKind::Boolean, {}}; break;
                case BinKind::Equal: op = "eq"; k = OpKind::Compare; t = {ScalarKind::Boolean, {}}; break;
                case BinKind::NotEqual: op = "ne"; k = OpKind::Compare; t = {ScalarKind::Boolean, {}}; break;
            }
            out.type = t;
            cur_fn->code.push_back({k, op, {lhs, rhs}, out, t});
            return;
        }
        if (auto* n = dynamic_cast<UnaryExpr*>(node)) {
            HIRValue v; gen_expr(n->operand_.get(), v);
            TypeSpec t = v.type;
            out = new_temp(t);
            std::string op;
            switch (n->op_) {
                case UnaryKind::Negate: op = "neg"; break;
                case UnaryKind::Posit: op = "pos"; break;
                case UnaryKind::PreIncrement: op = "inc"; break;
                case UnaryKind::PreDecrement: op = "dec"; break;
            }
            cur_fn->code.push_back({OpKind::BinArith, op, {v}, out, t});
            return;
        }
        if (auto* n = dynamic_cast<SubscriptExpr*>(node)) {
            HIRValue base, idx;
            gen_expr(n->base_.get(), base);
            gen_expr(n->index_.get(), idx);
            TypeSpec elem_t = base.type;
            if (!elem_t.decorators.empty()) elem_t.decorators.pop_back();
            HIRValue ptr = new_temp({elem_t.kind, {TypeDecorator::Pointer}});
            cur_fn->code.push_back({OpKind::ElemPtr, "elemptr", {base, idx}, ptr, ptr.type});
            out = new_temp(elem_t);
            cur_fn->code.push_back({OpKind::Load, "ld", {ptr}, out, elem_t});
            return;
        }
        if (auto* n = dynamic_cast<CallExpr*>(node)) {
            std::vector<HIRValue> args;
            for (auto& a : n->args_) { HIRValue v; gen_expr(a.get(), v); args.push_back(v); }
            TypeSpec ret_t = {ScalarKind::Int32, {}}; // упрощение, надо брать из таблицы функций
            out = new_temp(ret_t);
            std::vector<HIRValue> all_args = {HIRValue::make_obj(n->callee_, {ScalarKind::Int32, {}})};
            all_args.push_back(out);
            all_args.insert(all_args.end(), args.begin(), args.end());
            cur_fn->code.push_back({OpKind::Call, "call", all_args, out, ret_t});
            return;
        }
        // fallback
        out = new_temp({ScalarKind::Int32, {}});
    }

    void IRGenerator::gen_stmt(AstNode* node) {
        if (auto* n = dynamic_cast<VarDeclStmt*>(node)) {
            TypeSpec t = n->type_;
            var_types[n->name_] = t;
            cur_fn->locals.push_back({n->name_, t, false});
            HIRValue obj = HIRValue::make_obj(n->name_, t);
            if (n->init_) {
                HIRValue v; gen_expr(n->init_.get(), v);
                cur_fn->code.push_back({OpKind::Const, "store_init", {obj, v}, {}, t});
            }
            return;
        }
        if (auto* n = dynamic_cast<AssignStmt*>(node)) {
            HIRValue val; gen_expr(n->value_.get(), val);
            if (auto* id = dynamic_cast<Identifier*>(n->target_.get())) {
                cur_fn->code.push_back({OpKind::Const, "assign",
                    {HIRValue::make_obj(id->name_, var_types[id->name_]), val}, {}, {}});
            } else if (auto* sub = dynamic_cast<SubscriptExpr*>(n->target_.get())) {
                HIRValue base, idx;
                gen_expr(sub->base_.get(), base);
                gen_expr(sub->index_.get(), idx);
                TypeSpec elem_t = base.type;
                if (!elem_t.decorators.empty()) elem_t.decorators.pop_back();
                HIRValue ptr = new_temp({elem_t.kind, {TypeDecorator::Pointer}});
                cur_fn->code.push_back({OpKind::ElemPtr, "elemptr", {base, idx}, ptr, ptr.type});
                cur_fn->code.push_back({OpKind::Store, "st", {ptr, val}, {}, {}});
            }
            return;
        }
        if (auto* n = dynamic_cast<BlockStmt*>(node)) { gen_block(n); return; }
        if (auto* n = dynamic_cast<ReturnStmt*>(node)) {
            if (n->value_) {
                HIRValue v; gen_expr(n->value_.get(), v);
                cur_fn->code.push_back({OpKind::Ret, "ret", {v}, {}, {}});
            } else {
                cur_fn->code.push_back({OpKind::Ret, "ret", {}, {}, {}});
            }
            return;
        }
        if (auto* n = dynamic_cast<WhileStmt*>(node)) {
            HIRValue l_start = new_label();
            HIRValue l_end = new_label();
            cur_fn->code.push_back({OpKind::Label, "label", {l_start}, {}, {}});
            HIRValue cond; gen_expr(n->cond_.get(), cond);
            cur_fn->code.push_back({OpKind::Branch, "branch", {cond, l_end, {}}, {}, {}});
            gen_stmt(n->body_.get());
            cur_fn->code.push_back({OpKind::Jump, "jump", {l_start}, {}, {}});
            cur_fn->code.push_back({OpKind::Label, "label", {l_end}, {}, {}});
            return;
        }
        if (auto* n = dynamic_cast<IfStmt*>(node)) {
            HIRValue l_end = new_label();
            for (size_t i = 0; i < n->branches_.size(); ++i) {
                HIRValue l_next = new_label();
                HIRValue cond; gen_expr(n->branches_[i].cond.get(), cond);
                cur_fn->code.push_back({OpKind::Branch, "branch", {cond, l_next, {}}, {}, {}});
                gen_stmt(n->branches_[i].body.get());
                cur_fn->code.push_back({OpKind::Jump, "jump", {l_end}, {}, {}});
                cur_fn->code.push_back({OpKind::Label, "label", {l_next}, {}, {}});
            }
            if (n->else_body_) gen_stmt(n->else_body_.get());
            cur_fn->code.push_back({OpKind::Label, "label", {l_end}, {}, {}});
            return;
        }
        HIRValue tmp = new_temp({ScalarKind::Int32, {}});
        gen_expr(node, tmp);
    }

    HIRTranslationUnit IRGenerator::generate(ProgramNode* root) {
        for (auto& f : root->funcs_) {
            if (auto* fn = dynamic_cast<FunctionDecl*>(f.get())) {
                HIRFunction hf;
                hf.name = fn->name_;
                hf.ret_type = fn->ret_type_;
                for (auto& p : fn->params_)
                    hf.params.push_back({p.name, p.type, false});
                tu.functions.push_back(hf);
                cur_fn = &tu.functions.back();
                var_types.clear();
                temp_counter = 0;
                label_counter = 0;
                for (auto& p : fn->params_)
                    var_types[p.name] = p.type;
                gen_stmt(fn->body_.get());
            }
        }
        return tu;
    }

    // --- Задача 4.2: Проверка корректности ---

    // --- Задача 4.4: CFG ---
    bool IRGenerator::verify(const HIRTranslationUnit& tu, std::ostream& err) {
        bool ok = true;
        for (const auto& fn : tu.functions) {
            std::map<std::string, TypeSpec> defined;
            for (auto& p : fn.params) defined[p.name] = p.type;
            for (auto& l : fn.locals) defined[l.name] = l.type;

            for (size_t i = 0; i < fn.code.size(); ++i) {
                const auto& op = fn.code[i];
                // СНАЧАЛА добавляем результат (чтобы call->%t13 не ругался на самого себя)
                if (op.result.kind == HIRValue::Kind::Object) {
                    defined[op.result.name] = op.result.type;
                }
                // Проверяем аргументы
                for (auto& a : op.args) {
                    if (a.kind == HIRValue::Kind::Object
                        && defined.find(a.name) == defined.end())
                    {
                        // Игнорируем имена функций (они объявлены на уровне трансляции, а не в fn)
                        bool is_function = false;
                        for (const auto& f : tu.functions) {
                            if (f.name == a.name) { is_function = true; break; }
                        }
                        if (!is_function) {
                            err << "Use before def: %" << a.name << " in " << op.op_str
                                << " (op #" << i << " in " << fn.name << ")\n";
                            ok = false;
                        }
                    }
                }
            }
        }
        return ok;
    }

    void IRGenerator::gen_block(BlockStmt* node) {
        for (auto& i : node->items_) gen_stmt(i.get());
    }
void IRGenerator::build_cfg(HIRFunction& fn) {
    fn.blocks.clear();
    if (fn.code.empty()) return;

    // 1) Собираем все метки -> индексы
    std::map<std::string, int> label_to_idx;
    for (size_t i = 0; i < fn.code.size(); ++i) {
        if (fn.code[i].kind == OpKind::Label) {
            label_to_idx[fn.code[i].args[0].name] = (int)i;
        }
    }

    // 2) Точки начала блоков: начало функции, все метки, все операции после jump/branch/ret
    std::vector<int> starts;
    starts.push_back(0);
    for (size_t i = 0; i < fn.code.size(); ++i) {
        if (fn.code[i].kind == OpKind::Label) starts.push_back((int)i);
        if (fn.code[i].kind == OpKind::Jump ||
            fn.code[i].kind == OpKind::Branch ||
            fn.code[i].kind == OpKind::Ret) {
            if (i + 1 < fn.code.size()) starts.push_back((int)i + 1);
        }
    }
    std::sort(starts.begin(), starts.end());
    starts.erase(std::unique(starts.begin(), starts.end()), starts.end());

    // 3) Создаём блоки
    std::map<std::string, int> label_to_block;
    for (size_t i = 0; i < starts.size(); ++i) {
        int start = starts[i];
        int end = (i + 1 < starts.size()) ? starts[i + 1] : (int)fn.code.size();
        if (start >= end) continue;

        // Метка: если блок начинается с label — берём первое имя метки.
        // Если несколько подряд (L6 L5), берём первую (L6).
        std::string name;
        if (fn.code[start].kind == OpKind::Label) {
            name = fn.code[start].args[0].name;
        } else {
            name = "bb" + std::to_string(fn.blocks.size());
        }

        // Все метки внутри диапазона указывают на этот блок
        for (auto& [lbl, idx] : label_to_idx) {
            if (idx >= start && idx < end) {
                label_to_block[lbl] = (int)fn.blocks.size();
            }
        }
        // Также имя-метка для самого блока
        label_to_block[name] = (int)fn.blocks.size();

        fn.blocks.push_back({name, start, end, {}});
    }

    // 4) Рёбра
    for (size_t i = 0; i < fn.blocks.size(); ++i) {
        auto& b = fn.blocks[i];
        if (b.start_idx >= b.end_idx) continue;
        const auto& last = fn.code[b.end_idx - 1];

        if (last.kind == OpKind::Jump) {
            auto it = label_to_block.find(last.args[0].name);
            if (it != label_to_block.end()) b.succs.push_back(fn.blocks[it->second].label);
        } else if (last.kind == OpKind::Branch) {
            auto it = label_to_block.find(last.args[1].name);
            if (it != label_to_block.end()) b.succs.push_back(fn.blocks[it->second].label);
            if (i + 1 < fn.blocks.size()) b.succs.push_back(fn.blocks[i + 1].label);
        } else if (last.kind != OpKind::Ret && i + 1 < fn.blocks.size()) {
            b.succs.push_back(fn.blocks[i + 1].label);
        }
    }

    // 5) Удаляем блоки, недостижимые после ret (т.е. пустые после ret-блока)
    //    А также удаляем пустые блоки
    std::vector<HIRFunction::BasicBlock> cleaned;
    for (auto& b : fn.blocks) {
        // Пропускаем блоки, состоящие только из автоматического ret после другого ret
        if (b.start_idx >= b.end_idx) continue;
        // Если блок начинается с ret и состоит из одного ret — это хвостовой ret, оставляем
        if (b.start_idx + 1 == b.end_idx &&
            fn.code[b.start_idx].kind == OpKind::Ret &&
            !cleaned.empty() &&
            cleaned.back().end_idx > 0 &&
            fn.code[cleaned.back().end_idx - 1].kind == OpKind::Ret) {
            continue; // уже был ret — этот фантомный, пропускаем
        }
        cleaned.push_back(b);
    }
    fn.blocks = cleaned;
}

    // --- LIR: псевдо-ассемблер ---
std::vector<LIRInstr> LIRGenerator::generate(const HIRFunction& fn) {
    std::vector<LIRInstr> out;

    // Пролог функции
    out.push_back({"func_begin", fn.name, "", ""});
    out.push_back({"push", "bp", "", ""});
    out.push_back({"mov", "bp", "sp", ""});


    // Собираем все объекты функции (locals + params) и даем им смещения от bp
    std::map<std::string, int> offset;
    int off = 8;
    int frame_size = (off + 15) & ~15; // Выравнивание до 16 байт (ABI x86-64)
    out.push_back({"sub", "sp", "sp", std::to_string(frame_size)}); // bp и retval лежат на стеке
    for (auto& p : fn.params) { offset[p.name] = off; off += 4; }
    for (auto& l : fn.locals) {
        if (offset.find(l.name) == offset.end()) {
            offset[l.name] = off;
            off += 4;
        }
    }

    // Идем по HIR и генерируем ассемблер
    for (auto& op : fn.code) {
        if (op.op_str == "const") {
            // const #v -> %t
            out.push_back({"mov", "[" + std::to_string(offset[op.result.name]) + "]",
                           std::to_string(op.args[0].const_val), ""});
        } else if (op.kind == OpKind::BinArith || op.kind == OpKind::Compare) {
            // Берем lhs и rhs из памяти, кладем в регистры, делаем op, пишем в result
            std::string r1 = op.args[0].kind == HIRValue::Kind::Constant ?
                             "#" + std::to_string(op.args[0].const_val) :
                             "[" + std::to_string(offset[op.args[0].name]) + "]";
            std::string r2 = op.args.size() > 1 ?
                             (op.args[1].kind == HIRValue::Kind::Constant ?
                              "#" + std::to_string(op.args[1].const_val) :
                              "[" + std::to_string(offset[op.args[1].name]) + "]") : "";
            out.push_back({"ld", "r0", r1, ""});
            if (!r2.empty()) out.push_back({"ld", "r1", r2, ""});
            out.push_back({op.op_str, "r0", "r0", "r1"});
            out.push_back({"st", "[" + std::to_string(offset[op.result.name]) + "]", "r0", ""});
        } else if (op.op_str == "call") {
            // call f, ret, arg1, arg2 ...
            // передаем аргументы через a0, a1, ...
            for (size_t i = 2; i < op.args.size(); ++i) {
                std::string a = op.args[i].kind == HIRValue::Kind::Constant ?
                                "#" + std::to_string(op.args[i].const_val) :
                                "[" + std::to_string(offset[op.args[i].name]) + "]";
                out.push_back({"mov", "a" + std::to_string(i - 2), a, ""});
            }
            out.push_back({"call", op.args[0].name, "", ""});
            out.push_back({"st", "[" + std::to_string(offset[op.result.name]) + "]", "retval", ""});
        } else if (op.kind == OpKind::Jump) {
            out.push_back({"jmp", op.args[0].name, "", ""});
        } else if (op.kind == OpKind::Branch) {
            // branch cond, Ltrue, Lfalse
            out.push_back({"ld", "r0", "[" + std::to_string(offset[op.args[0].name]) + "]", ""});
            out.push_back({"jz", "r0", op.args[1].name, ""});
        } else if (op.kind == OpKind::Ret) {
            if (!op.args.empty()) {
                std::string r = op.args[0].kind == HIRValue::Kind::Constant ?
                                "#" + std::to_string(op.args[0].const_val) :
                                "[" + std::to_string(offset[op.args[0].name]) + "]";
                out.push_back({"ld", "retval", r, ""});
            }
            out.push_back({"mov", "sp", "bp", ""    });
            out.push_back({"pop", "bp", "", ""});
            out.push_back({"ret", "", "", ""});
        } else if (op.op_str == "label") {
            out.push_back({op.args[0].name + ":", "", "", ""});
        }
        // (элементный доступ elemptr/ld/st можно раскрыть так же —
        //  shl для *4 и add, но для лабы достаточно и этого)
    }

    out.push_back({"func_end", fn.name, "", ""});
    return out;
}

void LIRGenerator::dump(const std::vector<LIRInstr>& code, std::ostream& os) {
    for (auto& i : code) {
        if (!i.arg1.empty() && i.arg1.back() == ':') {
            os << i.arg1 << "\n";
        } else if (i.op == "func_begin" || i.op == "func_end") {
            os << i.op << " " << i.arg1 << "\n";
        } else {
            os << "    " << i.op;
            if (!i.arg1.empty()) os << " " << i.arg1;
            if (!i.arg2.empty()) os << ", " << i.arg2;
            if (!i.arg3.empty()) os << ", " << i.arg3;
            os << "\n";
        }
    }
}
} // namespace frontend
#include <sstream>