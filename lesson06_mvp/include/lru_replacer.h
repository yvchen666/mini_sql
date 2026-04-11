#pragma once
#include <cstdint>
#include <list>
#include <unordered_map>

using frame_id_t = int32_t;

class LRUReplacer {
public:
    explicit LRUReplacer(size_t pool_size);
    ~LRUReplacer() = default;
    bool Victim(frame_id_t* frame_id);
    void Pin(frame_id_t frame_id);
    void Unpin(frame_id_t frame_id);
    size_t Size() const;
private:
    size_t pool_size_;
    std::list<frame_id_t> lru_list_;
    std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> lru_map_;
};
