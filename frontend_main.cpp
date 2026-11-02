#include <iostream>
#include <fstream>
#include <string>
#include "AST.h"
#include "Semantic.h" // <--- ДОБАВИТЬ
#include "IR.h"
extern int yyparse();
extern frontend::ProgramNode* root;

int main() {
    std::cout << "Starting parsing...\n";

    if (yyparse() == 0 && root != nullptr) {
        std::cout << "\n--- AST ---\n";
        root->describe();

        // --- СЕМАНТИЧЕСКИЙ АНАЛИЗ ---
        std::cout << "\n--- Semantic Analysis ---\n";
        frontend::SemanticAnalyzer analyzer;
        analyzer.analyze(root);
        // ----------------------------

        std::cout << "\n--- HIR (High-level IR) ---\n";
        frontend::IRGenerator irgen;
        auto tu = irgen.generate(root);

        // Проверка корректности (Задача 4.2)
        std::cout << "\n--- Верификация IR ---\n";
        if (irgen.verify(tu, std::cerr)) {
            std::cout << "IR корректен: все объекты определены до использования.\n";
        }

        // Вывод линейного IR
        for (auto& fn : tu.functions) {
            std::cout << "\nfunc " << fn.name << ":\n";
            for (auto& op : fn.code) {
                std::cout << "    " << op.op_str;
                for (auto& a : op.args) std::cout << " " << a.str();
                if (op.result.kind == frontend::HIRValue::Kind::Object)
                    std::cout << " -> " << op.result.str();
                std::cout << "\n";
            }

            // CFG (Задача 4.4)
            irgen.build_cfg(fn);
            std::cout << "  CFG blocks:\n";
            for (auto& b : fn.blocks) {
                std::cout << "    " << b.label << " [" << b.start_idx << ".." << b.end_idx << ") ->";
                for (auto& s : b.succs) std::cout << " " << s;
                std::cout << "\n";
            }
        }

        std::cout << "\n--- LIR (Low-level IR / ассемблер) ---\n";
        frontend::LIRGenerator lirgen;
        for (auto& fn : tu.functions) {
            auto code = lirgen.generate(fn);
            lirgen.dump(code, std::cout);
        }

        std::ofstream dot("ast.dot");
        dot << "digraph AST {\n  node [shape=box, style=filled];\n";
        root->emit_dot(dot);
        dot << "}\n";
        dot.close();
        
        std::cout << "\nFile 'ast.dot' generated.\n";
        delete root;
    } else {
        std::cout << "Parsing failed.\n";
        return 1;
    }
    return 0;
}