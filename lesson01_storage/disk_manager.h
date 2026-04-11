#pragma once

#include "page.h"
#include <string>
#include <fstream>

// ============================================================
// disk_manager.h — 磁盘管理器
//
// DiskManager 是数据库与磁盘文件之间的唯一接口。
// 上层组件（缓冲池等）只通过 DiskManager 读写页面，
// 不直接操作文件，这样便于将来替换底层存储（如换成 SSD 优化版本）。
//
// 文件布局：
//   [Page 0][Page 1][Page 2]...[Page N]
//   每个页面占 PAGE_SIZE 字节，page_id 即为页面在文件中的索引。
//   Page N 的文件偏移 = N * PAGE_SIZE
// ============================================================

class DiskManager {
public:
    // 构造函数：打开（或创建）数据库文件
    // db_file：数据库文件路径，例如 "mydb.db"
    explicit DiskManager(const std::string& db_file);

    // 析构函数：关闭文件
    ~DiskManager();

    // 将 data 指向的 PAGE_SIZE 字节写入 page_id 对应的位置
    // 如果 page_id 超出文件末尾，文件会自动扩展
    void WritePage(page_id_t page_id, const char* data);

    // 从 page_id 对应的位置读取 PAGE_SIZE 字节到 data 指向的缓冲区
    // 调用者需保证 data 至少有 PAGE_SIZE 字节的空间
    void ReadPage(page_id_t page_id, char* data);

    // 分配一个新的页面 ID（简单地递增计数器）
    // 返回新分配的 page_id
    page_id_t AllocatePage();

    // 获取已分配的页面总数
    page_id_t GetPageCount() const { return next_page_id_; }

    // 获取数据库文件名
    const std::string& GetFileName() const { return db_file_name_; }

private:
    std::string  db_file_name_;   // 数据库文件路径
    std::fstream db_file_;        // 文件流（同时支持读和写）
    page_id_t    next_page_id_;   // 下一个可分配的页面 ID（从 0 开始）
};
