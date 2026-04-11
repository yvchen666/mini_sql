#include "lru_replacer.h"

// ============================================================
// lru_replacer.cpp — LRU 替换器实现
// ============================================================

LRUReplacer::LRUReplacer(size_t pool_size) : pool_size_(pool_size) {}

bool LRUReplacer::Victim(frame_id_t* frame_id) {
    if (lru_list_.empty()) {
        return false;  // 没有可淘汰的帧
    }

    // 链表尾部是最久未用的帧，选它作为淘汰对象
    *frame_id = lru_list_.back();
    lru_map_.erase(*frame_id);
    lru_list_.pop_back();
    return true;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
    // 该帧被 pin 住，从淘汰候选列表中移除
    auto it = lru_map_.find(frame_id);
    if (it != lru_map_.end()) {
        lru_list_.erase(it->second);  // 从链表中删除
        lru_map_.erase(it);           // 从哈希表中删除
    }
    // 如果不在列表中（已经被 pin 住），什么都不做
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
    // 如果已经在淘汰列表中，不重复添加
    if (lru_map_.count(frame_id) > 0) {
        return;
    }

    // 将帧加入链表头部（最近使用）
    lru_list_.push_front(frame_id);
    lru_map_[frame_id] = lru_list_.begin();
}

size_t LRUReplacer::Size() const {
    return lru_list_.size();
}
