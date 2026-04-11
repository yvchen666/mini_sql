#pragma once

#include <cstdint>
#include <list>
#include <unordered_map>

using frame_id_t = int32_t;

// ============================================================
// lru_replacer.h — LRU 替换器
//
// LRU（Least Recently Used，最近最少使用）替换算法：
// 当缓冲池满且需要腾出空间时，选择最久没有被使用的帧来淘汰。
//
// 实现方案：双向链表 + 哈希表
//   - 双向链表维护"可淘汰帧"的访问顺序
//     · 链表头部 = 最近使用的帧
//     · 链表尾部 = 最久未用的帧（淘汰候选）
//   - 哈希表 frame_id → 链表迭代器，实现 O(1) 查找和删除
//
// 注意：只有 Unpin（pin_count==0）的帧才在替换器中，
//       Pin 住的帧（pin_count>0）不参与淘汰。
// ============================================================

class LRUReplacer {
public:
    // pool_size：缓冲池中帧的总数（替换器最多跟踪这么多帧）
    explicit LRUReplacer(size_t pool_size);

    ~LRUReplacer() = default;

    // 选择并淘汰最久未用的帧
    // 成功返回 true 并将帧 ID 写入 *frame_id；
    // 如果没有可淘汰的帧，返回 false
    bool Victim(frame_id_t* frame_id);

    // 固定（Pin）某帧：该帧正在被使用，从淘汰列表中移除
    void Pin(frame_id_t frame_id);

    // 解固（Unpin）某帧：该帧不再被使用，加入淘汰候选列表
    void Unpin(frame_id_t frame_id);

    // 当前有多少帧可以被淘汰
    size_t Size() const;

private:
    size_t pool_size_;  // 缓冲池大小（上限）

    // 双向链表：存放可淘汰帧的 frame_id，头部=最近使用
    std::list<frame_id_t> lru_list_;

    // 哈希表：frame_id → 在链表中的迭代器，O(1) 删除
    std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> lru_map_;
};
