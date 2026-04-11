#include "lru_replacer.h"

LRUReplacer::LRUReplacer(size_t pool_size) : pool_size_(pool_size) {}

bool LRUReplacer::Victim(frame_id_t* frame_id) {
    if (lru_list_.empty()) return false;
    *frame_id = lru_list_.back();
    lru_map_.erase(*frame_id);
    lru_list_.pop_back();
    return true;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
    auto it = lru_map_.find(frame_id);
    if (it != lru_map_.end()) {
        lru_list_.erase(it->second);
        lru_map_.erase(it);
    }
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
    if (lru_map_.count(frame_id) > 0) return;
    lru_list_.push_front(frame_id);
    lru_map_[frame_id] = lru_list_.begin();
}

size_t LRUReplacer::Size() const { return lru_list_.size(); }
