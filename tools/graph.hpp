#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <algorithm>
#include <list>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef DEBUG
    #define GRAPH_ASSERT(c, m) do { if (!(c)) throw std::logic_error(m); } while (0)
#else
    #define GRAPH_ASSERT(c, m) ((void)0)
#endif

namespace graph {

struct void_data_t {};

template <typename T>
constexpr bool valid_data_v = std::is_void_v<T> ||
    (std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>);

template <typename Tnode_data, typename Tedge_data,
          typename = std::enable_if_t<valid_data_v<Tnode_data>>,
          typename = std::enable_if_t<valid_data_v<Tedge_data>>>
class orgraph_t {
public:
    class node_t;
    class edge_t;

    using node_iterator       = typename std::list<node_t>::iterator;
    using const_node_iterator = typename std::list<node_t>::const_iterator;
    using edge_iterator       = typename std::list<edge_t>::iterator;
    using const_edge_iterator = typename std::list<edge_t>::const_iterator;

    class node_t {
        friend class orgraph_t;
        using data_t = std::conditional_t<std::is_void_v<Tnode_data>, void_data_t, Tnode_data>;

        data_t _data;
        std::list<edge_iterator> _in, _out;

    public:
        template <typename... Args>
        explicit node_t(Args&&... args) {
            if constexpr (!std::is_void_v<Tnode_data>)
                _data = Tnode_data(std::forward<Args>(args)...);
        }
        data_t& get_data() { return _data; }
        const data_t& get_data() const { return _data; }
        const std::list<edge_iterator>& get_in_edges() const { return _in; }
        const std::list<edge_iterator>& get_out_edges() const { return _out; }
    };

    class edge_t {
        friend class orgraph_t;
        using data_t = std::conditional_t<std::is_void_v<Tedge_data>, void_data_t, Tedge_data>;

        data_t _data;
        node_iterator _from, _to;

    public:
        template <typename... Args>
        explicit edge_t(node_iterator from, node_iterator to, Args&&... args)
            : _from(from), _to(to) {
            if constexpr (!std::is_void_v<Tedge_data>)
                _data = Tedge_data(std::forward<Args>(args)...);
        }
        data_t& get_data() { return _data; }
        const data_t& get_data() const { return _data; }
        node_iterator get_from() const { return _from; }
        node_iterator get_to() const { return _to; }
    };

    virtual ~orgraph_t() = default;

private:
    std::list<node_t> _nodes;
    std::list<edge_t> _edges;

public:
    std::vector<node_iterator> get_nodes_iterator() {
        std::vector<node_iterator> v;
        for (auto it = _nodes.begin(); it != _nodes.end(); ++it) v.push_back(it);
        return v;
    }

    template <typename... Args>
    node_iterator create_node(Args&&... args) {
        _nodes.emplace_back(std::forward<Args>(args)...);
        return std::prev(_nodes.end());
    }

    template <typename... Args>
    edge_iterator create_edge(node_iterator from, node_iterator to, Args&&... args) {
        _edges.emplace_back(from, to, std::forward<Args>(args)...);
        auto e = std::prev(_edges.end());
        from->_out.push_back(e);
        to->_in.push_back(e);
        GRAPH_ASSERT(check(), "create_edge: broken invariant");
        return e;
    }

    void replace_prev_node(edge_iterator e, node_iterator n) {
        if (e->_from == n) return;
        e->_from->_out.remove(e);
        e->_from = n;
        n->_out.push_back(e);
        GRAPH_ASSERT(check(), "replace_prev_node: broken invariant");
    }

    void replace_next_node(edge_iterator e, node_iterator n) {
        if (e->_to == n) return;
        e->_to->_in.remove(e);
        e->_to = n;
        n->_in.push_back(e);
        GRAPH_ASSERT(check(), "replace_next_node: broken invariant");
    }

    void delete_edge(edge_iterator e) {
        e->_from->_out.remove(e);
        e->_to->_in.remove(e);
        _edges.erase(e);
        GRAPH_ASSERT(check(), "delete_edge: broken invariant");
    }

    void delete_node(node_iterator n) {
        while (!n->_out.empty()) delete_edge(n->_out.front());
        while (!n->_in.empty()) delete_edge(n->_in.front());
        _nodes.erase(n);
        GRAPH_ASSERT(check(), "delete_node: broken invariant");
    }

    bool check() const {
        std::size_t count = 0;
        for (auto it = _nodes.begin(); it != _nodes.end(); ++it) {
            for (auto e : it->_out) {
                if (e->_from != it) return false;
                const auto& in = e->_to->_in;
                if (std::find(in.begin(), in.end(), e) == in.end()) return false;
            }
            for (auto e : it->_in)
                if (e->_to != it) return false;
            count += it->_out.size();
        }
        return count == _edges.size();
    }

    template <typename NF, typename EF>
    void dump_dot(std::ostream& os, NF nf, EF ef) const {
        os << "digraph G {\n";
        for (auto it = _nodes.begin(); it != _nodes.end(); ++it)
            os << "  node_" << &*it << " [" << nf(it->get_data()) << "];\n";
        for (auto it = _edges.begin(); it != _edges.end(); ++it)
            os << "  node_" << &*it->_from << " -> node_" << &*it->_to
               << " [" << ef(it->get_data()) << "];\n";
        os << "}\n";
    }
};

}
#endif