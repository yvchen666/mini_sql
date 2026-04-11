#include "buffer_pool_manager.h"
#include <iostream>
#include <stdexcept>

// ============================================================
// buffer_pool_manager.cpp — 缓冲池管理器实现
// ============================================================

BufferPoolManager::BufferPoolManager(size_t pool_size, const std::string& db_file)
    : pool_size_(pool_size),
      frames_(pool_size),
      replacer_(pool_size),
      next_page_id_(0),
      db_file_name_(db_file) {

    // 初始化：所有帧都是空闲的
    for (frame_id_t i = 0; i < static_cast<frame_id_t>(pool_size); i++) {
        free_list_.push_back(i);
    }

    // 打开数据库文件
    db_file_.open(db_file, std::ios::in | std::ios::out | std::ios::binary);
    if (!db_file_.is_open()) {
        db_file_.open(db_file, std::ios::out | std::ios::binary);
        db_file_.close();
        db_file_.open(db_file, std::ios::in | std::ios::out | std::ios::binary);
    }

    // 恢复 next_page_id_
    db_file_.seekg(0, std::ios::end);
    std::streamsize file_size = db_file_.tellg();
    if (file_size > 0) {
        next_page_id_ = static_cast<page_id_t>(file_size / PAGE_SIZE);
    }

    std::cout << "[BufferPoolManager] 初始化，帧数=" << pool_size
              << "，数据库文件=" << db_file << "\n";
}

BufferPoolManager::~BufferPoolManager() {
    FlushAllPages();
    if (db_file_.is_open()) {
        db_file_.close();
    }
}

frame_id_t BufferPoolManager::GetAvailableFrame() {
    // 优先使用空闲帧
    if (!free_list_.empty()) {
        frame_id_t frame_id = free_list_.front();
        free_list_.pop_front();
        return frame_id;
    }

    // 没有空闲帧，用 LRU 淘汰一个
    frame_id_t victim_frame;
    if (!replacer_.Victim(&victim_frame)) {
        return -1;  // 所有帧都被 pin 住，无法淘汰
    }

    // 如果被淘汰的帧是脏页，先写回磁盘
    Page& victim_page = frames_[victim_frame];
    if (victim_page.IsDirty()) {
        WritePageToDisk(victim_frame);
        std::cout << "[BufferPoolManager] 淘汰脏页 " << victim_page.GetPageId()
                  << "，已写回磁盘\n";
    }

    // 从 page_table_ 中移除该页面的映射
    page_table_.erase(victim_page.GetPageId());

    return victim_frame;
}

void BufferPoolManager::LoadPageFromDisk(page_id_t page_id, frame_id_t frame_id) {
    size_t offset = static_cast<size_t>(page_id) * PAGE_SIZE;
    db_file_.seekg(offset, std::ios::beg);
    db_file_.read(frames_[frame_id].GetData(), PAGE_SIZE);
    // 如果读取失败（例如新页面），数据区保持为零
    if (db_file_.fail()) {
        db_file_.clear();
        memset(frames_[frame_id].GetData(), 0, PAGE_SIZE);
    }
}

void BufferPoolManager::WritePageToDisk(frame_id_t frame_id) {
    Page& page = frames_[frame_id];
    size_t offset = static_cast<size_t>(page.GetPageId()) * PAGE_SIZE;
    db_file_.seekp(offset, std::ios::beg);
    db_file_.write(page.GetData(), PAGE_SIZE);
    db_file_.flush();
    page.SetDirty(false);
}

Page* BufferPoolManager::FetchPage(page_id_t page_id) {
    // 1. 检查页面是否已在缓冲池中（缓存命中）
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        frame_id_t frame_id = it->second;
        frames_[frame_id].IncrPinCount();
        replacer_.Pin(frame_id);  // 从 LRU 列表中移除（正在使用）
        std::cout << "[BufferPoolManager] 缓存命中 page_id=" << page_id
                  << " frame_id=" << frame_id << "\n";
        return &frames_[frame_id];
    }

    // 2. 缓存未命中，需要从磁盘加载
    frame_id_t frame_id = GetAvailableFrame();
    if (frame_id == -1) {
        std::cout << "[BufferPoolManager] 错误：缓冲池已满，无法加载页面 " << page_id << "\n";
        return nullptr;
    }

    // 3. 从磁盘读取页面到帧
    frames_[frame_id].Reset();
    LoadPageFromDisk(page_id, frame_id);
    frames_[frame_id].SetPageId(page_id);
    frames_[frame_id].IncrPinCount();

    // 4. 更新 page_table_
    page_table_[page_id] = frame_id;
    replacer_.Pin(frame_id);

    std::cout << "[BufferPoolManager] 从磁盘加载 page_id=" << page_id
              << " → frame_id=" << frame_id << "\n";
    return &frames_[frame_id];
}

Page* BufferPoolManager::NewPage(page_id_t* page_id) {
    frame_id_t frame_id = GetAvailableFrame();
    if (frame_id == -1) {
        return nullptr;
    }

    // 分配新的页面 ID
    *page_id = next_page_id_++;

    // 初始化帧
    frames_[frame_id].Reset();
    frames_[frame_id].SetPageId(*page_id);
    frames_[frame_id].IncrPinCount();
    frames_[frame_id].SetDirty(true);  // 新页面需要写盘

    page_table_[*page_id] = frame_id;
    replacer_.Pin(frame_id);

    std::cout << "[BufferPoolManager] 新建页面 page_id=" << *page_id
              << " → frame_id=" << frame_id << "\n";
    return &frames_[frame_id];
}

bool BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) {
    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        return false;
    }

    frame_id_t frame_id = it->second;
    Page& page = frames_[frame_id];

    if (page.GetPinCount() <= 0) {
        return false;  // 已经是 0，不能再减
    }

    page.DecrPinCount();
    if (is_dirty) {
        page.SetDirty(true);
    }

    // pin_count 降为 0，加入 LRU 候选列表
    if (page.GetPinCount() == 0) {
        replacer_.Unpin(frame_id);
    }

    return true;
}

bool BufferPoolManager::FlushPage(page_id_t page_id) {
    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        return false;
    }
    WritePageToDisk(it->second);
    return true;
}

void BufferPoolManager::FlushAllPages() {
    for (auto& [page_id, frame_id] : page_table_) {
        if (frames_[frame_id].IsDirty()) {
            WritePageToDisk(frame_id);
        }
    }
}

bool BufferPoolManager::DeletePage(page_id_t page_id) {
    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        return true;  // 不在缓冲池中，视为成功
    }

    frame_id_t frame_id = it->second;
    if (frames_[frame_id].GetPinCount() > 0) {
        return false;  // 有人正在使用，不能删除
    }

    replacer_.Pin(frame_id);  // 从 LRU 列表移除
    page_table_.erase(it);
    frames_[frame_id].Reset();
    free_list_.push_back(frame_id);
    return true;
}
