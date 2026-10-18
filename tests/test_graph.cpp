#include <catch2/catch_test_macros.hpp>


#include <sstream>
#include <string>

#include "../tools/graph.hpp"


// 1. Узлы: создание, payload, модификация
TEST_CASE("Graph: nodes", "[orgraph][node]") {
    graph::orgraph_t<int, int> g;

    SECTION("create_node returns iterator to a fresh node") {
        auto a = g.create_node(11);
        auto b = g.create_node(22);

        REQUIRE(a->get_data() == 11);
        REQUIRE(b->get_data() == 22);
        REQUIRE(g.get_nodes_iterator().size() == 2);
        REQUIRE(g.check());
    }

    SECTION("payload is writable via non-const getter") {
        auto n = g.create_node(7);
        n->get_data() = 99;
        REQUIRE(n->get_data() == 99);
    }

    SECTION("variadic arguments are forwarded to payload") {
        // std::string(int count, char fill) — проверяем forwarding
        graph::orgraph_t<std::string, int> gs;
        auto n = gs.create_node(3, 'x');
        REQUIRE(n->get_data() == "xxx");
    }
}


// 2. Дуги: создание, регистрация у обоих концов, payload
TEST_CASE("Graph: edges", "[orgraph][edge]") {
    graph::orgraph_t<int, std::string> g;
    auto a = g.create_node(1);
    auto b = g.create_node(2);

    SECTION("new edge is registered at both endpoints") {
        auto e = g.create_edge(a, b, "a->b");

        REQUIRE(e->get_data() == "a->b");
        REQUIRE(e->get_from() == a);
        REQUIRE(e->get_to()   == b);

        REQUIRE(a->get_out_edges().size() == 1);
        REQUIRE(a->get_in_edges().empty());

        REQUIRE(b->get_in_edges().size() == 1);
        REQUIRE(b->get_out_edges().empty());

        REQUIRE(g.check());
    }

    SECTION("multiple out-edges from one node") {
        auto c = g.create_node(3);
        g.create_edge(a, b, "ab");
        g.create_edge(a, c, "ac");

        REQUIRE(a->get_out_edges().size() == 2);
        REQUIRE(b->get_in_edges().size()  == 1);
        REQUIRE(c->get_in_edges().size()  == 1);
        REQUIRE(g.check());
    }

    SECTION("multiple in-edges to one node") {
        auto c = g.create_node(3);
        g.create_edge(a, c, "ac");
        g.create_edge(b, c, "bc");

        REQUIRE(c->get_in_edges().size() == 2);
        REQUIRE(a->get_out_edges().size() == 1);
        REQUIRE(b->get_out_edges().size() == 1);
        REQUIRE(g.check());
    }

    SECTION("parallel edges between the same pair") {
        auto e1 = g.create_edge(a, b, "first");
        auto e2 = g.create_edge(a, b, "second");

        REQUIRE(e1 != e2);
        REQUIRE(a->get_out_edges().size() == 2);
        REQUIRE(b->get_in_edges().size()  == 2);
        REQUIRE(g.check());
    }

    SECTION("self-loop") {
        auto e = g.create_edge(a, a, "loop");

        REQUIRE(a->get_out_edges().size() == 1);
        REQUIRE(a->get_in_edges().size()  == 1);
        REQUIRE(e->get_from() == a);
        REQUIRE(e->get_to()   == a);
        REQUIRE(g.check());
    }
}


// 3. replace_prev_node / replace_next_node
TEST_CASE("Graph: replace endpoints", "[orgraph][replace]") {
    graph::orgraph_t<int, int> g;
    auto a = g.create_node(1);
    auto b = g.create_node(2);
    auto c = g.create_node(3);

    SECTION("replace_prev_node moves the arc") {
        auto e = g.create_edge(a, b, 0);
        g.replace_prev_node(e, c);

        REQUIRE(e->get_from() == c);
        REQUIRE(a->get_out_edges().empty());
        REQUIRE(c->get_out_edges().size() == 1);
        REQUIRE(g.check());
    }

    SECTION("replace_prev_node to the same source is a no-op") {
        auto e = g.create_edge(a, b, 0);
        g.replace_prev_node(e, a);  // уже a — ничего не меняется

        REQUIRE(e->get_from() == a);
        REQUIRE(a->get_out_edges().size() == 1);
        REQUIRE(g.check());
    }

    SECTION("replace_prev_node does not touch unrelated arcs") {
        auto e1 = g.create_edge(a, b, 0);
        auto e2 = g.create_edge(a, c, 1);
        g.replace_prev_node(e1, c);  // e1: a->b  становится  c->b

        // e2 по-прежнему a->c, значит у a остаётся одна out-дуга
        REQUIRE(a->get_out_edges().size() == 1);
        REQUIRE(e2->get_from() == a);
        REQUIRE(g.check());
    }

    SECTION("replace_next_node moves the arc") {
        auto e = g.create_edge(a, b, 0);
        g.replace_next_node(e, c);

        REQUIRE(e->get_to() == c);
        REQUIRE(b->get_in_edges().empty());
        REQUIRE(c->get_in_edges().size() == 1);
        REQUIRE(g.check());
    }

    SECTION("replace_next_node to the same target is a no-op") {
        auto e = g.create_edge(a, b, 0);
        g.replace_next_node(e, b);

        REQUIRE(e->get_to() == b);
        REQUIRE(b->get_in_edges().size() == 1);
        REQUIRE(g.check());
    }
}


// 4. Удаление

TEST_CASE("Graph: removal", "[orgraph][delete]") {
    graph::orgraph_t<int, int> g;
    auto a = g.create_node(1);
    auto b = g.create_node(2);
    auto c = g.create_node(3);

    auto e_ab = g.create_edge(a, b, 12);
    auto e_bc = g.create_edge(b, c, 23);
    auto e_ca = g.create_edge(c, a, 31);

    SECTION("delete_edge removes only that arc") {
        g.delete_edge(e_ab);

        REQUIRE(a->get_out_edges().empty());
        REQUIRE(b->get_in_edges().empty());

        // Остальные дуги целы
        REQUIRE(b->get_out_edges().size() == 1);
        REQUIRE(c->get_in_edges().size()  == 1);
        REQUIRE(c->get_out_edges().size() == 1);
        REQUIRE(a->get_in_edges().size()  == 1);

        REQUIRE(g.check());
    }

    SECTION("delete_node cascades all incident arcs") {
        g.delete_node(b);

        REQUIRE(g.get_nodes_iterator().size() == 2);
        REQUIRE(a->get_out_edges().empty());   // e_ab удалена
        REQUIRE(c->get_in_edges().empty());    // e_bc удалена

        // e_ca остаётся нетронутой
        REQUIRE(c->get_out_edges().size() == 1);
        REQUIRE(a->get_in_edges().size()  == 1);

        REQUIRE(g.check());
    }

    SECTION("delete_node with self-loop") {
        auto d = g.create_node(4);
        g.create_edge(d, d, 44);

        g.delete_node(d);
        REQUIRE(g.get_nodes_iterator().size() == 3);
        REQUIRE(g.check());
    }

    SECTION("delete_node with only in-edges") {
        // У c две входящие дуги (b->c и e_ca? нет, e_ca это c->a)
        // Проверим изолированный случай:
        auto x = g.create_node(10);
        auto y = g.create_node(11);
        auto z = g.create_node(12);
        g.create_edge(x, z, 0);
        g.create_edge(y, z, 1);
        g.delete_node(z);

        REQUIRE(x->get_out_edges().empty());
        REQUIRE(y->get_out_edges().empty());
        REQUIRE(g.check());
    }
}


// 5. Стабильность итераторов (требование №5 в Lab01)

TEST_CASE("Graph: iterator stability", "[orgraph][iterator]") {
    graph::orgraph_t<int, int> g;

    SECTION("creating more nodes/edges does not invalidate saved iterators") {
        auto a = g.create_node(1);
        auto b = g.create_node(2);
        auto e = g.create_edge(a, b, 0);

        // Создаём ещё много чего — сохранённые итераторы должны оставаться живыми
        for (int i = 0; i < 100; ++i) {
            auto extra = g.create_node(1000 + i);
            g.create_edge(a, extra, i);
        }

        REQUIRE(a->get_data() == 1);
        REQUIRE(b->get_data() == 2);
        REQUIRE(e->get_data() == 0);
        REQUIRE(g.check());
    }

    SECTION("deleting a node does not invalidate saved iterators to others") {
        auto a = g.create_node(1);
        auto b = g.create_node(2);
        auto c = g.create_node(3);

        auto e_ac = g.create_edge(a, c, 0);

        g.delete_node(b);

        // a, c и e_ac по-прежнему валидны
        REQUIRE(a->get_data() == 1);
        REQUIRE(c->get_data() == 3);
        REQUIRE(e_ac->get_data() == 0);
        REQUIRE(e_ac->get_from() == a);
        REQUIRE(e_ac->get_to()   == c);
        REQUIRE(g.check());
    }

    SECTION("deleting an edge does not invalidate saved edge iterators") {
        auto a = g.create_node(1);
        auto b = g.create_node(2);
        auto c = g.create_node(3);

        auto e_ab = g.create_edge(a, b, 0);
        auto e_ac = g.create_edge(a, c, 1);
        auto e_bc = g.create_edge(b, c, 2);

        g.delete_edge(e_ac);

        // e_ab и e_bc по-прежнему работают
        REQUIRE(e_ab->get_data() == 0);
        REQUIRE(e_bc->get_data() == 2);
        REQUIRE(g.check());
    }
}


// 6. get_nodes_iterator и обходы

TEST_CASE("Graph: node iterator enumeration", "[orgraph][iterator]") {
    graph::orgraph_t<int, int> g;

    SECTION("returns empty for empty graph") {
        REQUIRE(g.get_nodes_iterator().empty());
    }

    SECTION("returns all nodes, accessible for reading and writing") {
        auto a = g.create_node(1);
        auto b = g.create_node(2);
        auto c = g.create_node(3);

        auto nodes = g.get_nodes_iterator();
        REQUIRE(nodes.size() == 3);

        int sum = 0;
        for (auto it : nodes) sum += it->get_data();
        REQUIRE(sum == 6);

        // Через итераторы можно писать
        for (auto it : nodes) it->get_data() *= 10;
        REQUIRE(a->get_data() == 10);
        REQUIRE(b->get_data() == 20);
        REQUIRE(c->get_data() == 30);
    }

    SECTION("nodes can be traversed via out-edges (adjacency walk)") {
        auto a = g.create_node(1);
        auto b = g.create_node(2);
        auto c = g.create_node(3);

        g.create_edge(a, b, 12);
        g.create_edge(b, c, 23);

        // Простейший обход по out-edges от a
        int steps = 0;
        auto cur = a;
        while (!cur->get_out_edges().empty()) {
            cur = cur->get_out_edges().front()->get_to();
            ++steps;
        }
        REQUIRE(steps == 2);
        REQUIRE(cur == c);
    }
}


// 7. void-специализация

TEST_CASE("Graph: void payloads", "[orgraph][void]") {
    SECTION("void, void — nodes and edges can be created") {
        graph::orgraph_t<void, void> g;
        auto a = g.create_node();
        auto b = g.create_node();
        auto e = g.create_edge(a, b);

        REQUIRE(a->get_out_edges().size() == 1);
        REQUIRE(b->get_in_edges().size()  == 1);
        REQUIRE(e->get_from() == a);
        REQUIRE(e->get_to()   == b);
        REQUIRE(g.check());
    }

    SECTION("void payloads support full edit cycle") {
        graph::orgraph_t<void, void> g;
        auto a = g.create_node();
        auto b = g.create_node();
        auto c = g.create_node();

        auto e = g.create_edge(a, b);
        g.replace_prev_node(e, c);
        g.replace_next_node(e, c);
        REQUIRE(e->get_from() == c);
        REQUIRE(e->get_to()   == c);

        g.delete_node(c);
        REQUIRE(g.check());
    }
}


// 8. dump_dot

TEST_CASE("Graph: dump_dot", "[orgraph][export]") {
    SECTION("int/int — labels and arcs are present") {
        graph::orgraph_t<int, int> g;
        auto a = g.create_node(100);
        auto b = g.create_node(200);
        g.create_edge(a, b, 300);

        std::ostringstream oss;
        auto fmt = [](int v) {
            return "label=\"" + std::to_string(v) + "\"";
        };
        g.dump_dot(oss, fmt, fmt);

        const std::string text = oss.str();
        REQUIRE(text.find("digraph G {")      != std::string::npos);
        REQUIRE(text.find("label=\"100\"")    != std::string::npos);
        REQUIRE(text.find("label=\"200\"")    != std::string::npos);
        REQUIRE(text.find("label=\"300\"")    != std::string::npos);
        REQUIRE(text.find("->")               != std::string::npos);
        REQUIRE(text.find("}")                != std::string::npos);
    }

    SECTION("int/string — разные форматтеры для узлов и дуг") {
        graph::orgraph_t<int, std::string> g;
        auto a = g.create_node(1);
        auto b = g.create_node(2);
        g.create_edge(a, b, "arc_ab");

        std::ostringstream oss;
        auto nf = [](int v)               { return "label=\"" + std::to_string(v) + "\""; };
        auto ef = [](const std::string& s){ return "label=\"" + s + "\""; };
        g.dump_dot(oss, nf, ef);

        const std::string text = oss.str();
        REQUIRE(text.find("label=\"arc_ab\"") != std::string::npos);
        REQUIRE(text.find("label=\"1\"")      != std::string::npos);
    }

    SECTION("void/void — formatter accepts void_data_t") {
        graph::orgraph_t<void, void> g;
        auto a = g.create_node();
        auto b = g.create_node();
        g.create_edge(a, b);

        std::ostringstream oss;
        auto fmt = [](auto&) { return std::string("style=filled"); };
        g.dump_dot(oss, fmt, fmt);

        const std::string text = oss.str();
        REQUIRE(text.find("digraph G {")  != std::string::npos);
        REQUIRE(text.find("style=filled") != std::string::npos);
        REQUIRE(text.find("->")           != std::string::npos);
    }

    SECTION("empty graph — valid DOT shell") {
        graph::orgraph_t<int, int> g;
        std::ostringstream oss;
        auto fmt = [](int) { return std::string{}; };
        g.dump_dot(oss, fmt, fmt);

        REQUIRE(oss.str() == "digraph G {\n}\n");
    }
}


// 9. check() после каждой операции

TEST_CASE("Graph: check() reflects invariant", "[orgraph][check]") {
    graph::orgraph_t<int, int> g;
    REQUIRE(g.check());                             // пустой

    auto a = g.create_node(1);
    REQUIRE(g.check());                             // один узел

    auto b = g.create_node(2);
    auto c = g.create_node(3);
    REQUIRE(g.check());                             // три узла

    auto e1 = g.create_edge(a, b, 0);
    REQUIRE(g.check());

    auto e2 = g.create_edge(b, c, 1);
    auto e3 = g.create_edge(c, a, 2);
    REQUIRE(g.check());                             // цикл из трёх дуг — это валидный орграф

    g.replace_prev_node(e2, a);
    REQUIRE(g.check());

    g.replace_next_node(e3, b);
    REQUIRE(g.check());

    g.delete_edge(e1);
    REQUIRE(g.check());

    g.delete_node(c);
    REQUIRE(g.check());
}