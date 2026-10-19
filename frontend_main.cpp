#include <iostream>
#include <fstream>
#include <string>
#include "AST.h"
#include "Semantic.h" // <--- ДОБАВИТЬ

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