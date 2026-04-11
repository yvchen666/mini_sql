#pragma once

#include "lru_replacer.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <fstream>
#include <list>
#include <unordered_map>
#include <vector>

using page_id_t = int32_t;
using frame_id_t = int32_t;

static constexpr page_id_t INVALID_PAGE_ID = -1;
static constexpr size_t PAGE_SIZE = 4096;

// ============================================================
// Page 类（含 pin_count，比 Lesson 01 多了引用计数）
// ============================================================
class Page {
public:
    Page() : page_id_(INVALID_PAGE_ID), is_dirty_(false), pin_count_(0) {
        memset(data_, 0, PAGE_SIZE);
    }

    page_id_t GetPageId() const { return page_id_; }
    void SetPageId(page_id_t id) { page_id_ = id; }
    char* GetData() { return data_; }
    const char* GetData() const { return data_; }
    bool IsDirty() const { return is_dirty_; }
    void SetDirty(bool d) { is_dirty_ = d; }
    int GetPinCount() const { return pin_count_; }
    void IncrPinCount() { pin_count_++; }
    void DecrPinCount() { if (pin_count_ > 0) pin_count_--; }
    void Reset() {
        page_id_ = INVALID_PAGE_ID;
        is_dirty_ = false;
        pin_count_ = 0;
        memset(data_, 0, PAGE_SIZE);
    }

private:
    page_id_t page_id_;
    bool      is_dirty_;
    int       pin_count_;
    char      data_[PAGE_SIZE];
};

// ============================================================
// BufferPoolManager — 缓冲池管理器
//
// 管理固定数量（pool_size）的内存帧（Frame）。
// 上层组件通过 FetchPage/NewPage/UnpinPage 操作页面，
// 不需要直接与磁盘打交道。
//
// 核心数据结构：
//   frames_[]     : 内存中的页面数组（固定大小）
//   page_table_   : page_id → frame_id 的映射（快速查找）
//   free_list_    : 空闲帧列表（还没有存放任何页面的帧）
//   replacer_     : LRU 替换器（决定淘汰哪个帧）
// ============================================================
class BufferPoolManager {
public:
    // pool_size：缓冲池中帧的数量（内存中最多同时存放多少页面）
    // db_file：数据库文件路径
    BufferPoolManager(size_t pool_size, const std::string& db_file);
    ~BufferPoolManager();

    // 获取指定页面（如果不在缓冲池中，从磁盘加载）
    // 返回 nullptr 表示缓冲池已满且所有帧都被 pin 住
    Page* FetchPage(page_id_t page_id);

    // 分配一个新页面（在磁盘上分配空间，并加载到缓冲池）
    // 返回 nullptr 表示缓冲池已满
    Page* NewPage(page_id_t* page_id);

    // 解除对页面的固定（pin_count--）
    // is_dirty：如果为 true，标记该页面为脏页
    bool UnpinPage(page_id_t page_id, bool is_dirty);

    // 将指定页面强制刷盘（写回磁盘）
    bool FlushPage(page_id_t page_id);

    // 将所有脏页刷盘
    void FlushAllPages();

    // 删除页面（从缓冲池和磁盘中移除）
    bool DeletePage(page_id_t page_id);

    // 获取缓冲池大小
    size_t GetPoolSize() const { return pool_size_; }

private:
    // 从磁盘读取页面到指定帧
    void LoadPageFromDisk(page_id_t page_id, frame_id_t frame_id);

    // 将指定帧的页面写回磁盘
    void WritePageToDisk(frame_id_t frame_id);

    // 找到一个可用的帧（优先用空闲帧，否则用 LRU 淘汰）
    // 返回 -1 表示没有可用帧
    frame_id_t GetAvailableFrame();

    size_t pool_size_;                                    // 缓冲池大小
    std::vector<Page> frames_;                            // 内存帧数组
    std::unordered_map<page_id_t, frame_id_t> page_table_; // page_id → frame_id
    std::list<frame_id_t> free_list_;                     // 空闲帧列表
    LRUReplacer replacer_;                                // LRU 替换器
    page_id_t next_page_id_;                              // 下一个可分配的页面 ID

    // 磁盘文件
    std::string db_file_name_;
    std::fstream db_file_;
};
