#ifndef TREE_HPP
#define TREE_HPP

#include "graph.hpp"

#include <cstddef>
#include <queue>
#include <vector>

namespace tree {

    template <typename Tnode_data, typename Tedge_data>
    class tree_t : public graph::orgraph_t<Tnode_data, Tedge_data> {
    public:
        using base_graph    = graph::orgraph_t<Tnode_data, Tedge_data>;
        using node_iterator = typename base_graph::node_iterator;

        node_iterator get_root() {
            for (auto it : this->get_nodes_iterator()) {
                if (it->get_in_edges().empty()) {
                    return it;
                }
            }
            return node_iterator{};
        }

        bool check() {
            if (!base_graph::check()) {
                return false;
            }

            auto all = this->get_nodes_iterator();
            if (all.empty()) {
                return true;  // пустое дерево валидно
            }

            // Не более одного предка на узел; ровно один корень.
            // Проверка "roots != 1" заодно защищает BFS ниже от get_root()==end().
            std::size_t roots = 0;
            for (auto it : all) {
                const std::size_t parents = it->get_in_edges().size();
                if (parents == 0)     ++roots;
                else if (parents > 1) return false;
            }
            if (roots != 1) {
                return false;
            }

            // BFS от корня. Так как у каждого узла <= 1 предка, каждая
            // вершина достигается не более одного раза. Если достигнуты
            // все — циклов нет.
            // BFS на векторе: vector как очередь через индекс i.
            std::vector<node_iterator> frontier{ get_root() };
            for (std::size_t i = 0; i < frontier.size(); ++i) {
                for (auto e : frontier[i]->get_out_edges()) {
                    frontier.push_back(e->get_to());
                }
            }
            return frontier.size() == all.size();
        }
    };
}  // namespace tree
#endif