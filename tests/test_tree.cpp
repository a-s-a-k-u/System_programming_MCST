#include <catch2/catch_test_macros.hpp>

#include "../tools/tree.hpp"


// 1. Валидные деревья
TEST_CASE("Tree: valid topologies", "[tree][valid]") {
    tree::tree_t<int, int> t;

    SECTION("empty tree is valid, no root") {
        REQUIRE(t.check());
    }

    SECTION("single node is valid, is its own root") {
        auto root = t.create_node(10);
        REQUIRE(t.check());
        REQUIRE(t.get_root() == root);
        REQUIRE(t.get_root()->get_data() == 10);
    }

    SECTION("linear chain") {
        auto r = t.create_node(1);
        auto a = t.create_node(2);
        auto b = t.create_node(3);
        auto c = t.create_node(4);

        t.create_edge(r, a, 0);
        t.create_edge(a, b, 1);
        t.create_edge(b, c, 2);

        REQUIRE(t.check());
        REQUIRE(t.get_root() == r);
    }

    SECTION("branching tree") {
        auto r  = t.create_node(1);
        auto l  = t.create_node(2);
        auto rr = t.create_node(3);
        auto ll = t.create_node(4);
        auto lr = t.create_node(5);

        t.create_edge(r, l,  0);
        t.create_edge(r, rr, 1);
        t.create_edge(l, ll, 2);
        t.create_edge(l, lr, 3);

        REQUIRE(t.check());
        REQUIRE(t.get_root() == r);
    }

    SECTION("root is NOT the first created node") {
        // get_root() должен искать, а не возвращать первый узел.
        auto child = t.create_node(2);
        auto root  = t.create_node(1);
        t.create_edge(root, child, 0);

        REQUIRE(t.check());
        REQUIRE(t.get_root() == root);
        REQUIRE(t.get_root()->get_data() == 1);
    }
}


// 2. Невалидные топологии
TEST_CASE("Tree: structural rejections", "[tree][invalid]") {
    tree::tree_t<int, int> t;

    SECTION("two disjoint roots") {
        t.create_node(1);
        t.create_node(2);
        REQUIRE_FALSE(t.check());
    }

    SECTION("node with two distinct parents (diamond)") {
        auto r  = t.create_node(1);
        auto p1 = t.create_node(2);
        auto p2 = t.create_node(3);
        auto s  = t.create_node(4);

        t.create_edge(r, p1, 0);
        t.create_edge(r, p2, 1);
        t.create_edge(p1, s, 2);
        t.create_edge(p2, s, 3);

        REQUIRE_FALSE(t.check());
    }

    SECTION("node with two parallel in-edges from the same parent") {
        // Тоже два родителя — по счёту, хотя источник один.
        auto r = t.create_node(1);
        auto c = t.create_node(2);
        t.create_edge(r, c, 0);
        t.create_edge(r, c, 1);

        REQUIRE_FALSE(t.check());
    }
}


// 3. Циклы
TEST_CASE("Tree: cycle rejection", "[tree][cycle]") {
    SECTION("self-loop on the root") {
        tree::tree_t<int, int> t;
        auto a = t.create_node(1);
        t.create_edge(a, a, 0);
        REQUIRE_FALSE(t.check());
    }

    SECTION("self-loop on a non-root") {
        tree::tree_t<int, int> t;
        auto r = t.create_node(1);
        auto c = t.create_node(2);
        t.create_edge(r, c, 0);
        t.create_edge(c, c, 1);
        REQUIRE_FALSE(t.check());
    }

    SECTION("back edge to grandparent") {
        tree::tree_t<int, int> t;
        auto r = t.create_node(1);
        auto m = t.create_node(2);
        auto l = t.create_node(3);
        t.create_edge(r, m, 0);
        t.create_edge(m, l, 1);
        t.create_edge(l, r, 2);
        REQUIRE_FALSE(t.check());
    }

    SECTION("full ring (no roots)") {
        tree::tree_t<int, int> t;
        auto a = t.create_node(1);
        auto b = t.create_node(2);
        auto c = t.create_node(3);
        t.create_edge(a, b, 0);
        t.create_edge(b, c, 1);
        t.create_edge(c, a, 2);
        REQUIRE_FALSE(t.check());
    }


    SECTION("tree + isolated cycle") {
        tree::tree_t<int, int> t;

        auto r    = t.create_node(1);
        auto leaf = t.create_node(2);
        t.create_edge(r, leaf, 0);

        auto a = t.create_node(10);
        auto b = t.create_node(20);
        auto c = t.create_node(30);
        t.create_edge(a, b, 1);
        t.create_edge(b, c, 2);
        t.create_edge(c, a, 3);

        REQUIRE_FALSE(t.check());
    }
}


// 4. get_root

TEST_CASE("Tree: get_root behaviour", "[tree][root]") {
    SECTION("returns saved root iterator") {
        tree::tree_t<int, int> t;
        auto r = t.create_node(42);
        auto c = t.create_node(43);
        t.create_edge(r, c, 0);

        REQUIRE(t.get_root() == r);
        REQUIRE(t.get_root()->get_data() == 42);
    }

    SECTION("no root → default-constructed iterator returned") {
        // В графе без корней get_root() возвращает пустой итератор.
        // Проверяем только отсутствие валидного корня — не разыменовываем.
        tree::tree_t<int, int> t;
        auto a = t.create_node(1);
        auto b = t.create_node(2);
        t.create_edge(a, b, 0);
        t.create_edge(b, a, 1);

        REQUIRE_FALSE(t.check());
        (void)t.get_root();
    }
}


// 5. void-специализация
TEST_CASE("Tree: void payloads", "[tree][void]") {
    SECTION("valid void tree") {
        tree::tree_t<void, void> t;
        auto r = t.create_node();
        auto a = t.create_node();
        auto b = t.create_node();
        t.create_edge(r, a);
        t.create_edge(r, b);

        REQUIRE(t.check());
        REQUIRE(t.get_root() == r);
    }

    SECTION("void tree with a cycle is rejected") {
        tree::tree_t<void, void> t;
        auto r = t.create_node();
        auto c = t.create_node();
        t.create_edge(r, c);
        t.create_edge(c, r);

        REQUIRE_FALSE(t.check());
    }
}


// 6. Комбинированные сценарии (после изменений графа)
TEST_CASE("Tree: edits preserve validity", "[tree][edit]") {
    tree::tree_t<int, int> t;
    auto r  = t.create_node(1);
    auto a  = t.create_node(2);
    auto b  = t.create_node(3);

    auto e_ra = t.create_edge(r, a, 0);
    auto e_rb = t.create_edge(r, b, 1);
    REQUIRE(t.check());

    SECTION("deleting a leaf keeps tree valid") {
        t.delete_node(b);
        REQUIRE(t.check());
        REQUIRE(t.get_root() == r);
    }

    SECTION("deleting the root makes tree invalid (empty component with 0 roots? no — 2 disconnected roots)") {
        t.delete_node(r);
        // остались a и b без дуг — 2 корня
        REQUIRE_FALSE(t.check());
    }

    SECTION("replacing edge target keeps tree valid") {
        t.replace_next_node(e_rb, a);   // r -> b  становится  r -> a
        // a теперь имеет двух родителей: r (через e_ra) и r (через e_rb)
        // Значит у a два in-edge → невалидно
        REQUIRE_FALSE(t.check());
    }

    SECTION("deleting edge disconnects a node → invalid") {
        t.delete_edge(e_rb);
        // b теперь без родителей и без детей → второй корень
        REQUIRE_FALSE(t.check());
    }
}