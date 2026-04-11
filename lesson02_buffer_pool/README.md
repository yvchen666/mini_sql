# Lesson 02 — 缓冲池管理

## 学习目标

1. 理解为什么需要缓冲池（Buffer Pool）
2. 实现 LRU 替换算法（双向链表 + 哈希表）
3. 实现 BufferPoolManager：管理固定数量的内存帧
4. 理解 Pin/Unpin 机制和脏页管理

---

## 核心概念

### 为什么需要缓冲池？

磁盘 I/O 比内存访问慢约 **10万倍**。如果每次读取数据都直接访问磁盘，数据库会极慢。

缓冲池的思路：在内存中维护一个"页面缓存"，热点页面常驻内存，只有缓存满了才把最久未使用的页面换出到磁盘。

```
没有缓冲池：
  每次查询 → 磁盘读取 → 极慢

有缓冲池：
  第1次查询 → 磁盘读取 → 放入缓冲池
  第2次查询 → 缓冲池命中 → 极快（内存速度）
  第N次查询 → 缓冲池命中 → 极快
```

### 缓冲池架构

```
┌─────────────────────────────────────────────────────┐
│                  BufferPoolManager                   │
│                                                      │
│  frames[]:  内存中的页面槽位（固定数量，如 4 个）      │
│  ┌────────┬────────┬────────┬────────┐               │
│  │Frame 0 │Frame 1 │Frame 2 │Frame 3 │               │
│  │Page 5  │Page 2  │(空)    │Page 8  │               │
│  └────────┴────────┴────────┴────────┘               │
│                                                      │
│  page_table_: page_id → frame_id 的映射              │
│  { 5→0, 2→1, 8→3 }                                  │
│                                                      │
│  free_list_: 空闲帧列表 [2]                           │
│                                                      │
│  LRUReplacer: 记录哪些帧可以被淘汰                    │
│  LRU顺序: [Page 8] ← [Page 2] ← [Page 5]            │
│           (最久未用)              (最近使用)           │
└─────────────────────────────────────────────────────┘
```

### LRU 替换算法

LRU（Least Recently Used，最近最少使用）：当需要淘汰一个页面时，选择最久没有被访问的那个。

用**双向链表 + 哈希表**实现 O(1) 的插入、删除、查找：

```
哈希表：key → 链表节点指针（O(1) 查找）
双向链表：维护访问顺序（头部=最近使用，尾部=最久未用）

访问 Page A：将 A 移到链表头部
淘汰时：从链表尾部取出
```

### Pin/Unpin 机制

```
FetchPage(page_id):
  找到或加载页面 → pin_count++ → 返回页面指针
  （pin_count > 0 的页面不能被淘汰，因为有人正在用）

UnpinPage(page_id):
  pin_count-- → 如果 pin_count == 0，加入 LRU 候选列表
  （没人用了，可以被淘汰）
```

---

## 关键代码讲解

### LRU 替换器

```cpp
// 双向链表节点
struct LRUNode {
    frame_id_t frame_id;
    LRUNode* prev;
    LRUNode* next;
};

// 访问某帧：移到链表头部（最近使用）
void LRUReplacer::Pin(frame_id_t frame_id) {
    // 从链表中移除（不再是淘汰候选）
}

// 某帧可被淘汰：加入链表尾部
void LRUReplacer::Unpin(frame_id_t frame_id) {
    // 加入链表（成为淘汰候选）
}

// 淘汰最久未用的帧
bool LRUReplacer::Victim(frame_id_t* frame_id) {
    // 从链表头部取出（最久未用）
}
```

---

## 思考题

1. 为什么 pin_count > 0 的页面不能被淘汰？如果强制淘汰会发生什么？
2. LRU 算法在什么场景下效果不好？（提示：全表扫描）
3. 如果缓冲池中所有页面都被 pin 住了，FetchPage 应该怎么处理？

---

## 编译与运行

```bash
cd /home/aoi/AWorkSpace/sql_mvp/build
make lesson02
./lesson02_buffer_pool/lesson02
```
