#include "buffer_pool_manager.h"
#include <iostream>

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager* disk_mgr)
    : pool_size_(pool_size), frames_(pool_size), replacer_(pool_size), disk_mgr_(disk_mgr) {
    for (frame_id_t i = 0; i < static_cast<frame_id_t>(pool_size); i++) {
        free_list_.push_back(i);
    }
}

BufferPoolManager::~BufferPoolManager() { FlushAllPages(); }

frame_id_t BufferPoolManager::GetAvailableFrame() {
    if (!free_list_.empty()) {
        frame_id_t fid = free_list_.front();
        free_list_.pop_front();
        return fid;
    }
    frame_id_t victim;
    if (!replacer_.Victim(&victim)) return -1;
    Page& vp = frames_[victim];
    if (vp.IsDirty()) {
        disk_mgr_->WritePage(vp.GetPageId(), vp.GetData());
    }
    page_table_.erase(vp.GetPageId());
    return victim;
}

Page* BufferPoolManager::FetchPage(page_id_t page_id) {
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        frames_[it->second].IncrPinCount();
        replacer_.Pin(it->second);
        return &frames_[it->second];
    }
    frame_id_t fid = GetAvailableFrame();
    if (fid == -1) return nullptr;
    frames_[fid].Reset();
    disk_mgr_->ReadPage(page_id, frames_[fid].GetData());
    frames_[fid].SetPageId(page_id);
    frames_[fid].IncrPinCount();
    page_table_[page_id] = fid;
    replacer_.Pin(fid);
    return &frames_[fid];
}

Page* BufferPoolManager::NewPage(page_id_t* page_id) {
    frame_id_t fid = GetAvailableFrame();
    if (fid == -1) return nullptr;
    *page_id = disk_mgr_->AllocatePage();
    frames_[fid].Reset();
    frames_[fid].SetPageId(*page_id);
    frames_[fid].IncrPinCount();
    frames_[fid].SetDirty(true);
    page_table_[*page_id] = fid;
    replacer_.Pin(fid);
    return &frames_[fid];
}

bool BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) {
    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) return false;
    frame_id_t fid = it->second;
    if (frames_[fid].GetPinCount() <= 0) return false;
    frames_[fid].DecrPinCount();
    if (is_dirty) frames_[fid].SetDirty(true);
    if (frames_[fid].GetPinCount() == 0) replacer_.Unpin(fid);
    return true;
}

bool BufferPoolManager::FlushPage(page_id_t page_id) {
    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) return false;
    disk_mgr_->WritePage(page_id, frames_[it->second].GetData());
    frames_[it->second].SetDirty(false);
    return true;
}

void BufferPoolManager::FlushAllPages() {
    for (auto& [pid, fid] : page_table_) {
        if (frames_[fid].IsDirty()) {
            disk_mgr_->WritePage(pid, frames_[fid].GetData());
            frames_[fid].SetDirty(false);
        }
    }
}

bool BufferPoolManager::DeletePage(page_id_t page_id) {
    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) return true;
    frame_id_t fid = it->second;
    if (frames_[fid].GetPinCount() > 0) return false;
    replacer_.Pin(fid);
    page_table_.erase(it);
    frames_[fid].Reset();
    free_list_.push_back(fid);
    return true;
}
