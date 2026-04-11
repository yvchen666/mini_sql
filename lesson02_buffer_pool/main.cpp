#include <iostream>
#include <cstring>
#include <string>
#include "buffer_pool_manager.h"

// ============================================================
// main.cpp — Lesson 02 演示
//
// 演示缓冲池的核心功能：
//   1. LRU 替换器的工作原理
//   2. 缓冲池的 FetchPage/NewPage/UnpinPage
//   3. 缓存命中 vs 磁盘 I/O
//   4. 页面淘汰（LRU eviction）
// ============================================================

// ---- 演示 LRU 替换器 ----
void demo_lru_replacer() {
    std::cout << "\n======= LRU 替换器演示 =======\n";
    LRUReplacer replacer(5);

    // Unpin 帧 1,2,3,4,5（加入淘汰候选列表）
    replacer.Unpin(1);
    replacer.Unpin(2);
    replacer.Unpin(3);
    replacer.Unpin(4);
    replacer.Unpin(5);
    std::cout << "Unpin 帧 1,2,3,4,5，可淘汰数量: " << replacer.Size() << "\n";
    // LRU 顺序（头部最近Unpin，尾部最久）：
    // Unpin 顺序 1,2,3,4,5 → 链表 [5→4→3→2→1]，尾部 1 最先被 Unpin

    // Pin 帧 1 和 3（正在使用，不能淘汰）
    replacer.Pin(1);
    replacer.Pin(3);
    std::cout << "Pin 帧 1,3 后，可淘汰数量: " << replacer.Size() << "\n";
    // 候选列表：[5→4→2]，尾部 2 是最久未使用的

    // 淘汰（从链表尾部取，即最久未用的帧）
    frame_id_t victim;
    replacer.Victim(&victim);
    std::cout << "淘汰帧: " << victim << "（应该是 2，最久未用）\n";

    replacer.Victim(&victim);
    std::cout << "淘汰帧: " << victim << "（应该是 4）\n";

    // 重新 Unpin 帧 3（使用完毕）
    replacer.Unpin(3);
    replacer.Victim(&victim);
    std::cout << "淘汰帧: " << victim << "（应该是 5）\n";
}

// ---- 演示缓冲池管理器 ----
void demo_buffer_pool() {
    std::cout << "\n======= 缓冲池管理器演示 =======\n";

    // 创建一个只有 3 个帧的小缓冲池（方便观察淘汰行为）
    BufferPoolManager bpm(3, "lesson02.db");

    // ---- 新建页面 ----
    std::cout << "\n--- 新建 4 个页面（缓冲池只有 3 帧）---\n";
    page_id_t pid0, pid1, pid2, pid3;

    Page* p0 = bpm.NewPage(&pid0);
    strcpy(p0->GetData(), "Page 0: First page");
    bpm.UnpinPage(pid0, true);  // 使用完毕，释放，标记为脏

    Page* p1 = bpm.NewPage(&pid1);
    strcpy(p1->GetData(), "Page 1: Second page");
    bpm.UnpinPage(pid1, true);

    Page* p2 = bpm.NewPage(&pid2);
    strcpy(p2->GetData(), "Page 2: Third page");
    bpm.UnpinPage(pid2, true);

    // 此时缓冲池已满（3帧都用了），再新建会触发淘汰
    Page* p3 = bpm.NewPage(&pid3);
    if (p3) {
        strcpy(p3->GetData(), "Page 3: Fourth page (caused eviction)");
        bpm.UnpinPage(pid3, true);
    }

    // ---- 验证缓存命中 ----
    std::cout << "\n--- 验证缓存命中 ---\n";
    Page* fetch3 = bpm.FetchPage(pid3);
    if (fetch3) {
        std::cout << "FetchPage(" << pid3 << ") 内容: " << fetch3->GetData() << "\n";
        bpm.UnpinPage(pid3, false);
    }

    // ---- 验证磁盘读取（page0 可能已被淘汰） ----
    std::cout << "\n--- 重新获取被淘汰的页面（从磁盘加载）---\n";
    Page* fetch0 = bpm.FetchPage(pid0);
    if (fetch0) {
        std::cout << "FetchPage(" << pid0 << ") 内容: " << fetch0->GetData() << "\n";
        bpm.UnpinPage(pid0, false);
    }

    // ---- 刷盘所有脏页 ----
    std::cout << "\n--- 将所有脏页刷盘 ---\n";
    bpm.FlushAllPages();
    std::cout << "所有脏页已写回磁盘。\n";
}

int main() {
    std::cout << "=== Lesson 02: 缓冲池管理 ===\n";

    demo_lru_replacer();
    demo_buffer_pool();

    std::cout << "\n=== Lesson 02 完成！===\n";
    std::cout << "下一步：学习 Lesson 03，了解如何用 B+树实现高效索引。\n";
    return 0;
}
