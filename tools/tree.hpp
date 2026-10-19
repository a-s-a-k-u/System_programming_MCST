#ifndef TREE_HPP
#define TREE_HPP

#include "graph.hpp"

#include <cstddef>
#include <queue>
#include <vector>

namespace tree {

/**
 * Класс ориентированного дерева.
 * 
 * По своей сути дерево — это частный случай ориентированного графа (DAG).
 * Поэтому мы наследуемся от orgraph_t и переиспользуем всю логику добавления/удаления узлов и дуг.
 * Отличия дерева от графа:
 * 1. Должен быть ровно один корень (узел без входящих дуг).
 * 2. У каждого узла (кроме корня) должен быть ровно один предок.
 * 3. В дереве не должно быть циклов.
 */
template <typename Tnode_data, typename Tedge_data>
class tree_t : public graph::orgraph_t<Tnode_data, Tedge_data> {
public:
    using base_graph    = graph::orgraph_t<Tnode_data, Tedge_data>;
    using node_iterator = typename base_graph::node_iterator;

    /**
     * Метод поиска корня дерева.
     * Корень — это узел, в который не входит ни одна дуга (список входящих пуст).
     * 
     * @return Итератор на корень. Если корня нет (например, пустой граф или цикл),
     *         возвращает дефолтный (пустой) итератор.
     */
    node_iterator get_root() {
        for (auto it : this->get_nodes_iterator()) {
            if (it->get_in_edges().empty()) {
                return it;
            }
        }
        return node_iterator{}; // Корень не найден
    }

    /**
     * Метод проверки корректности дерева.
     * 
     * Проверка выполняется в три этапа:
     * 1. Базовая проверка связей графа (вызов метода родителя).
     * 2. Проверка топологии дерева: ровно один корень, и у остальных узлов ровно 1 родитель.
     * 3. Поиск циклов и проверка связности с помощью обхода в ширину (BFS).
     */
    bool check() {
        // Шаг 1: Проверяем, что базовый граф структурно целостен (нет висячих итераторов)
        if (!base_graph::check()) {
            return false;
        }

        auto all = this->get_nodes_iterator();
        
        // Пустое дерево считаем валидным (0 узлов, 0 дуг)
        if (all.empty()) {
            return true;
        }

        // Шаг 2: Проверка степеней входа (in-degree).
        // У каждого узла не может быть больше одного предка. 
        // А корень (без предков) должен быть ровно один.
        std::size_t roots = 0;
        for (auto it : all) {
            const std::size_t parents = it->get_in_edges().size();
            if (parents == 0)     ++roots;       // Нашли корень
            else if (parents > 1) return false;  // У узла несколько родителей (это уже граф, а не дерево)
        }
        
        if (roots != 1) {
            return false; // Либо корней нет (сплошной цикл), либо их несколько (лес)
        }

        // Шаг 3: Поиск циклов и проверка связности.
        // Используем BFS (обход в ширину) от корня.
        // 
        // Почему BFS, а не DFS (рекурсия)?
        // 1. BFS работает итеративно (через очередь), что спасает от переполнения стека (Stack Overflow) 
        //    на очень глубоких деревьях.
        // 2. Валидное дерево с одним корнем и <=1 родителем на узел не может иметь 
        //    "перекрестных" циклов, достижимых из корня. Если BFS из корня посетит 
        //    ВСЕ узлы, значит граф связен и ацикличен. Если посетит не все — 
        //    значит где-то есть изолированный цикл.
        
        std::queue<node_iterator> frontier;
        frontier.push(get_root());

        std::size_t reached = 0; // Счетчик посещенных узлов
        
        while (!frontier.empty()) {
            auto cur = frontier.front();
            frontier.pop();
            ++reached;
            
            // Добавляем в очередь всех потомков текущего узла
            for (auto e : cur->get_out_edges()) {
                frontier.push(e->get_to());
            }
        }

        // Если мы обошли столько же узлов, сколько всего в графе — 
        // дерево связно и не содержит недостижимых циклов.
        return reached == all.size();
    }
};

}  // namespace tree

#endif
