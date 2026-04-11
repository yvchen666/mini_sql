#pragma once

#include <cstdint>
#include <cstring>

// ============================================================
// page.h — 数据库页面定义
//
// 页面（Page）是数据库 I/O 的最小单位。
// 数据库不会一次读写几个字节，而是整页读写，原因：
//   1. 与操作系统/磁盘的块大小对齐，减少 I/O 次数
//   2. 便于缓冲池以固定大小单元管理内存
//   3. 页面编号可以直接换算为文件偏移：offset = page_id * PAGE_SIZE
// ============================================================

// 页面 ID 类型：用 32 位整数表示，支持最大 2^32 个页面
using page_id_t = int32_t;

// 无效页面 ID，用于表示"空"或"未分配"
static constexpr page_id_t INVALID_PAGE_ID = -1;

// 页面大小：4096 字节（4 KB），与常见操作系统内存页大小相同
static constexpr size_t PAGE_SIZE = 4096;

// ============================================================
// Page 类：封装单个页面的数据和元信息
// ============================================================
class Page {
public:
    Page() {
        // 初始化：页面 ID 设为无效，数据区清零，脏标记清除
        page_id_ = INVALID_PAGE_ID;
        is_dirty_ = false;
        memset(data_, 0, PAGE_SIZE);
    }

    // 获取页面 ID
    page_id_t GetPageId() const { return page_id_; }

    // 设置页面 ID（由 DiskManager 分配后设置）
    void SetPageId(page_id_t page_id) { page_id_ = page_id; }

    // 获取数据区指针（可读写）
    char* GetData() { return data_; }

    // 获取数据区指针（只读）
    const char* GetData() const { return data_; }

    // 是否为脏页（内存中的数据比磁盘上的新）
    bool IsDirty() const { return is_dirty_; }

    // 设置脏页标记
    void SetDirty(bool dirty) { is_dirty_ = dirty; }

    // 重置页面状态（用于页面被淘汰后复用）
    void Reset() {
        page_id_ = INVALID_PAGE_ID;
        is_dirty_ = false;
        memset(data_, 0, PAGE_SIZE);
    }

private:
    page_id_t page_id_;    // 本页面的编号
    bool      is_dirty_;   // 脏页标记
    char      data_[PAGE_SIZE];  // 实际存储数据的字节数组，大小固定为 PAGE_SIZE
};
