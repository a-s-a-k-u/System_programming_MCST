#include <iostream>
#include <fstream>
#include <string>

#include "graph.hpp"
#include "tree.hpp"

#include <windows.h> // в самом начале файла



void test_graph() {

    std::cout << "   ЛАБА 1: ОРИЕНТИРОВАННЫЙ ГРАФ (orgraph_t)\n";


    graph::orgraph_t<std::string, int> g;

    std::cout << "1. Создание узлов...\n";
    auto a = g.create_node("Node_A");
    auto b = g.create_node("Node_B");
    auto c = g.create_node("Node_C");
    std::cout << "   Создано узлов: " << g.get_nodes_iterator().size() << "\n";

    std::cout << "2. Создание дуг...\n";
    auto e1 = g.create_edge(a, b, 100);
    auto e2 = g.create_edge(b, c, 200);
    std::cout << "   Граф валиден (проверка инвариантов): " << (g.check() ? "ДА" : "НЕТ") << "\n";

    std::cout << "3. Изменение предка дуги (e1: A->B меняется на C->B)...\n";
    g.replace_prev_node(e1, c);
    std::cout << "   Граф валиден: " << (g.check() ? "ДА" : "НЕТ") << "\n";

    std::cout << "4. Экспорт в DOT (graph.dot)...\n";
    std::ofstream dot("graph.dot");
    auto fmt_node = [](const std::string& s) { return "label=\"" + s + "\""; };
    auto fmt_edge = [](int w) { return "label=\"" + std::to_string(w) + "\""; };
    g.dump_dot(dot, fmt_node, fmt_edge);
    dot.close();
    std::cout << "   Файл сохранен.\n\n";
}

void test_tree() {

    std::cout << "   ЛАБА 1: ДЕРЕВО (tree_t)\n";


    tree::tree_t<int, int> t;

    std::cout << "1. Создание дерева...\n";
    auto root = t.create_node(10);
    auto child1 = t.create_node(20);
    auto child2 = t.create_node(30);
    t.create_edge(root, child1, 0);
    t.create_edge(root, child2, 0);

    std::cout << "   Корень дерева: " << t.get_root()->get_data() << "\n";
    std::cout << "   Дерево валидно (нет циклов): " << (t.check() ? "ДА" : "НЕТ") << "\n";

    std::cout << "2. Ломаем дерево (создаем цикл child1 -> root)...\n";
    t.create_edge(child1, root, 0);
    std::cout << "   Дерево валидно: " << (t.check() ? "ДА" : "НЕТ") << " (ожидалось НЕТ)\n\n";
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    test_graph();
    test_tree();
    return 0;
}