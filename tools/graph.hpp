#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <algorithm>
#include <list>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

// Макрос для проверки инвариантов графа
// Активен только при сборке с флагом -DDEBUG
#ifdef DEBUG
    #define GRAPH_ASSERT(c, m) do { if (!(c)) throw std::logic_error(m); } while (0)
#else
    #define GRAPH_ASSERT(c, m) ((void)0)
#endif

namespace graph {

// Заглушка для хранения "пустоты", если пользователь передал тип void
// (например, graph::orgraph_t<void, void>). Позволяет избежать ошибок компиляции
struct void_data_t {};

// Проверка допустимости типа данных: тип должен поддерживать копирование,
// либо быть void (в этом случае мы будем хранить void_data_t).
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

    // Основное требование задания: минимальная инвалидация итераторов.
    // std::list гарантирует, что при добавлении/удалении элементов 
    // итераторы на остальные элементы остаются валидными.
    using node_iterator       = typename std::list<node_t>::iterator;
    using const_node_iterator = typename std::list<node_t>::const_iterator;
    using edge_iterator       = typename std::list<edge_t>::iterator;
    using const_edge_iterator = typename std::list<edge_t>::const_iterator;

    /**
     * Класс узла графа.
     * Хранит пользовательские данные и списки итераторов на инцидентные дуги.
     */
    class node_t {
        friend class orgraph_t;
        
        // Выбираем, что хранить: сам тип данных или заглушку, если передан void
        using data_t = std::conditional_t<std::is_void_v<Tnode_data>, void_data_t, Tnode_data>;

        data_t _data;
        
        // Входящие и исходящие дуги. Храним итераторы на списки дуг.
        // Это позволяет обходить граф как в прямом, так и в обратном направлении.
        std::list<edge_iterator> _in, _out;

    public:
        template <typename... Args>
        explicit node_t(Args&&... args) {
            // Если тип данных не void, конструируем его с переданными аргументами
            if constexpr (!std::is_void_v<Tnode_data>)
                _data = Tnode_data(std::forward<Args>(args)...);
        }
        
        data_t& get_data() { return _data; }
        const data_t& get_data() const { return _data; }
        const std::list<edge_iterator>& get_in_edges() const { return _in; }
        const std::list<edge_iterator>& get_out_edges() const { return _out; }
    };

    /**
     * Класс дуги графа.
     * Хранит пользовательские данные и итераторы на узлы-источник (from) и приемник (to).
     */
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
    // Хранилища узлов и дуг. Используем std::list для стабильности итераторов.
    std::list<node_t> _nodes;
    std::list<edge_t> _edges;

public:
    /**
     * Возвращает вектор итераторов на все узлы графа.
     * Удобно для обхода всего графа без знания внутренней структуры _nodes.
     */
    std::vector<node_iterator> get_nodes_iterator() {
        std::vector<node_iterator> v;
        for (auto it = _nodes.begin(); it != _nodes.end(); ++it) v.push_back(it);
        return v;
    }

    /**
     * Создает новый узел с переданными аргументами.
     * Возвращает итератор на созданный узел.
     */
    template <typename... Args>
    node_iterator create_node(Args&&... args) {
        _nodes.emplace_back(std::forward<Args>(args)...);
        return std::prev(_nodes.end()); // Итератор на только что добавленный узел
    }

    /**
     * Создает новую дугу между двумя узлами.
     * Автоматически регистрирует дугу в списках _out (у источника) и _in (у приемника).
     */
    template <typename... Args>
    edge_iterator create_edge(node_iterator from, node_iterator to, Args&&... args) {
        _edges.emplace_back(from, to, std::forward<Args>(args)...);
        auto e = std::prev(_edges.end());
        
        // Регистрируем новую дугу в узлах
        from->_out.push_back(e);
        to->_in.push_back(e);
        
        GRAPH_ASSERT(check(), "create_edge: broken invariant");
        return e;
    }

    /**
     * Заменяет узел-предок (источник) для дуги.
     * Отвязывает дугу от старого узла и привязывает к новому.
     */
    void replace_prev_node(edge_iterator e, node_iterator n) {
        if (e->_from == n) return; // Если узел тот же, ничего не делаем
        
        e->_from->_out.remove(e); // Удаляем из старого списка исходящих
        e->_from = n;             // Обновляем итератор внутри дуги
        n->_out.push_back(e);     // Добавляем в список исходящих нового узла
        
        GRAPH_ASSERT(check(), "replace_prev_node: broken invariant");
    }

    /**
     * Заменяет узел-потомок (приемник) для дуги.
     */
    void replace_next_node(edge_iterator e, node_iterator n) {
        if (e->_to == n) return;
        
        e->_to->_in.remove(e);
        e->_to = n;
        n->_in.push_back(e);
        
        GRAPH_ASSERT(check(), "replace_next_node: broken invariant");
    }

    /**
     * Удаляет конкретную дугу из графа.
     * Сначала убирает ссылки на дугу из узлов, затем удаляет сам элемент.
     */
    void delete_edge(edge_iterator e) {
        e->_from->_out.remove(e);
        e->_to->_in.remove(e);
        _edges.erase(e);
        
        GRAPH_ASSERT(check(), "delete_edge: broken invariant");
    }

    /**
     * Удаляет узел и ВСЕ инцидентные ему дуги (каскадное удаление).
     */
    void delete_node(node_iterator n) {
        // Пока есть исходящие дуги, удаляем их
        while (!n->_out.empty()) delete_edge(n->_out.front());
        // Пока есть входящие дуги, удаляем их
        while (!n->_in.empty()) delete_edge(n->_in.front());
        
        _nodes.erase(n); // Удаляем сам узел
        
        GRAPH_ASSERT(check(), "delete_node: broken invariant");
    }

    /**
     * Метод проверки инвариантов графа.
     * Проверяет, что все ссылки (итераторы) друг на друга корректны.
     * Используется для отладки, чтобы вовремя поймать ошибку изменения структуры.
     */
    bool check() const {
        std::size_t count = 0;
        
        // Проходим по всем узлам
        for (auto it = _nodes.begin(); it != _nodes.end(); ++it) {
            // Проверяем, что в списках узла лежат корректные дуги
            for (auto e : it->_out) {
                if (e->_from != it) return false; // Дуга не ссылается на этот узел как на источник
                const auto& in = e->_to->_in;     // Проверяем, что у приемника эта дуга есть во входящих
                if (std::find(in.begin(), in.end(), e) == in.end()) return false;
            }
            for (auto e : it->_in) {
                if (e->_to != it) return false;   // Дуга не ссылается на этот узел как на приемник
            }
            count += it->_out.size(); // Считаем общее количество дуг
        }
        
        // Количество исходящих дуг во всех узлах должно совпадать с реальным размером _edges
        return count == _edges.size();
    }

    /**
     * Экспорт графа в формат Graphviz (DOT).
     * Принимает функции-форматтеры для узлов и дуг (позволяет выводить любые данные).
     */
    template <typename NF, typename EF>
    void dump_dot(std::ostream& os, NF nf, EF ef) const {
        os << "digraph G {\n";
        
        // Выводим узлы. В качестве ID используем адрес в памяти (&*it)
        for (auto it = _nodes.begin(); it != _nodes.end(); ++it)
            os << "  node_" << &*it << " [" << nf(it->get_data()) << "];\n";
            
        // Выводим дуги (связи)
        for (auto it = _edges.begin(); it != _edges.end(); ++it)
            os << "  node_" << &*it->_from << " -> node_" << &*it->_to
               << " [" << ef(it->get_data()) << "];\n";
               
        os << "}\n";
    }
};

}
#endif
