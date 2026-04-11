#include <iostream>
#include <vector>
#include <cassert>
#include "btree.h"

// ============================================================
// main.cpp — Lesson 03 B+树演示
// ============================================================

int main() {
    std::cout << "=== Lesson 03: B+树索引 ===\n\n";

    BTree tree;

    // ---- 演示 1：插入 ----
    std::cout << "--- 插入键值对 ---\n";
    std::vector<std::pair<int, std::string>> data = {
        {10, "Alice"}, {20, "Bob"}, {5, "Charlie"},
        {15, "Dave"}, {25, "Eve"}, {30, "Frank"},
        {3, "Grace"}, {7, "Heidi"}, {12, "Ivan"},
        {18, "Judy"}, {22, "Karl"}, {28, "Laura"}
    };

    for (auto& [k, v] : data) {
        std::cout << "插入 " << k << " -> " << v << "\n";
        tree.Insert(k, v);
    }

    std::cout << "\n树高度: " << tree.Height() << "\n";
    std::cout << "\n";
    tree.Print();

    // ---- 演示 2：查找 ----
    std::cout << "\n--- 查找 ---\n";
    for (int key : {5, 15, 25, 99}) {
        auto result = tree.Search(key);
        if (result) {
            std::cout << "Search(" << key << ") = " << *result << "\n";
        } else {
            std::cout << "Search(" << key << ") = 不存在\n";
        }
    }

    // ---- 演示 3：范围查询 ----
    std::cout << "\n--- 范围查询 [10, 22] ---\n";
    auto range = tree.RangeSearch(10, 22);
    for (auto& [k, v] : range) {
        std::cout << "  " << k << " -> " << v << "\n";
    }

    // ---- 演示 4：更新 ----
    std::cout << "\n--- 更新键 10 的值 ---\n";
    tree.Insert(10, "Alice Updated");
    auto updated = tree.Search(10);
    std::cout << "Search(10) = " << (updated ? *updated : "不存在") << "\n";

    // ---- 演示 5：删除 ----
    std::cout << "\n--- 删除 ---\n";
    for (int key : {5, 15, 25}) {
        bool ok = tree.Delete(key);
        std::cout << "Delete(" << key << ") = " << (ok ? "成功" : "失败") << "\n";
    }

    std::cout << "\n删除后树结构：\n";
    tree.Print();

    // ---- 演示 6：验证删除后查找 ----
    std::cout << "\n--- 验证删除后查找 ---\n";
    for (int key : {5, 10, 15, 20, 25, 30}) {
        auto result = tree.Search(key);
        std::cout << "Search(" << key << ") = "
                  << (result ? *result : "不存在") << "\n";
    }

    // ---- 演示 7：正确性验证 ----
    std::cout << "\n--- 正确性验证 ---\n";
    assert(!tree.Search(5).has_value());
    assert(!tree.Search(15).has_value());
    assert(!tree.Search(25).has_value());
    assert(tree.Search(10).has_value());
    assert(tree.Search(20).has_value());
    assert(tree.Search(30).has_value());
    std::cout << "所有断言通过！B+树实现正确。\n";

    std::cout << "\n=== Lesson 03 完成！===\n";
    std::cout << "下一步：学习 Lesson 04，了解如何解析 SQL 语句。\n";
    return 0;
}
