#pragma once
#include "page.h"
#include "lru_replacer.h"
#include "disk_manager.h"
#include <string>
#include <list>
#include <unordered_map>
#include <vector>

class BufferPoolManager {
public:
    BufferPoolManager(size_t pool_size, DiskManager* disk_mgr);
    ~BufferPoolManager();
    Page* FetchPage(page_id_t page_id);
    Page* NewPage(page_id_t* page_id);
    bool UnpinPage(page_id_t page_id, bool is_dirty);
    bool FlushPage(page_id_t page_id);
    void FlushAllPages();
    bool DeletePage(page_id_t page_id);
private:
    frame_id_t GetAvailableFrame();
    size_t pool_size_;
    std::vector<Page> frames_;
    std::unordered_map<page_id_t, frame_id_t> page_table_;
    std::list<frame_id_t> free_list_;
    LRUReplacer replacer_;
    DiskManager* disk_mgr_;
};
