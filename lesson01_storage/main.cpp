#include <iostream>
#include <cstring>
#include <string>
#include "page.h"
#include "disk_manager.h"

// ============================================================
// main.cpp — Lesson 01 演示
//
// 本演示展示：
//   1. 创建 DiskManager，打开/创建数据库文件
//   2. 分配页面，将数据写入页面，再写到磁盘
//   3. 从磁盘读回页面，验证数据正确性
// ============================================================

int main() {
    std::cout << "=== Lesson 01: 存储引擎基础 ===\n\n";

    // 创建磁盘管理器（会创建 lesson01.db 文件）
    DiskManager disk_mgr("lesson01.db");

    // --------------------------------------------------------
    // 演示 1：写入多个页面
    // --------------------------------------------------------
    std::cout << "\n--- 写入页面 ---\n";

    const int NUM_PAGES = 3;
    Page pages[NUM_PAGES];

    for (int i = 0; i < NUM_PAGES; i++) {
        // 分配一个新的页面 ID
        page_id_t pid = disk_mgr.AllocatePage();
        pages[i].SetPageId(pid);

        // 向页面数据区写入一些内容
        std::string content = "Hello, Database Page " + std::to_string(i) + "!";
        // 将内容复制到页面数据区（注意：不超过 PAGE_SIZE）
        memset(pages[i].GetData(), 0, PAGE_SIZE);
        memcpy(pages[i].GetData(), content.c_str(), content.size());
        pages[i].SetDirty(true);  // 标记为脏页（内存数据比磁盘新）

        std::cout << "写入页面 " << pid << "，内容: " << content << "\n";

        // 将页面写入磁盘
        disk_mgr.WritePage(pid, pages[i].GetData());
        pages[i].SetDirty(false);  // 写盘后清除脏标记
    }

    // --------------------------------------------------------
    // 演示 2：从磁盘读回页面，验证数据
    // --------------------------------------------------------
    std::cout << "\n--- 读取并验证页面 ---\n";

    bool all_ok = true;
    for (int i = 0; i < NUM_PAGES; i++) {
        Page read_page;
        // 从磁盘读取页面到新的 Page 对象
        disk_mgr.ReadPage(i, read_page.GetData());
        read_page.SetPageId(i);

        // 读出的内容
        std::string read_content(read_page.GetData());

        // 预期内容
        std::string expected = "Hello, Database Page " + std::to_string(i) + "!";

        if (read_content == expected) {
            std::cout << "读取页面 " << i << " 成功，内容: " << read_content << "\n";
        } else {
            std::cout << "错误！页面 " << i << " 内容不匹配！\n";
            std::cout << "  预期: " << expected << "\n";
            std::cout << "  实际: " << read_content << "\n";
            all_ok = false;
        }
    }

    // --------------------------------------------------------
    // 演示 3：显示页面元信息
    // --------------------------------------------------------
    std::cout << "\n--- 页面信息 ---\n";
    std::cout << "页面大小: " << PAGE_SIZE << " 字节\n";
    std::cout << "已分配页面数: " << disk_mgr.GetPageCount() << "\n";
    std::cout << "数据库文件: " << disk_mgr.GetFileName() << "\n";
    std::cout << "文件总大小: " << disk_mgr.GetPageCount() * PAGE_SIZE << " 字节\n";

    // --------------------------------------------------------
    // 总结
    // --------------------------------------------------------
    std::cout << "\n--- 结果 ---\n";
    if (all_ok) {
        std::cout << "验证成功！数据写入和读取正确。\n";
    } else {
        std::cout << "验证失败！请检查代码。\n";
        return 1;
    }

    std::cout << "\n=== Lesson 01 完成！===\n";
    std::cout << "下一步：学习 Lesson 02，了解如何用缓冲池减少磁盘 I/O。\n";

    return 0;
}
